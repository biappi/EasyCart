/*
 * lst.c - cartridge content list file handling
 *
 * Written by
 *  Hannu Nuotio <nojoopa@users.sf.net>
 *
 * This file is part of ndefpack.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "boot.h"
#include "config.h"
#include "eapi.h"
#include "flash.h"
#include "loader.h"
#include "lst.h"
#include "util.h"

/* ------------------------------------------------------------------------- */

static const char lst_type_char[LST_TYPE_NUM] = {
    't',
    'h',
    'a',
    'e',
    'o',
    'O',
    'n',
    'N'
};

static FILE *fd_o = NULL;

/* ------------------------------------------------------------------------- */

static int lst_check_write_error(void)
{
    if (ferror(fd_o)) {
        util_error("problems writing to file!");
        fclose(fd_o);
        fd_o = NULL;
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int lst_parse_entry(easyflash_cart_t * cart, const char *buf_in)
{
    int res = -1;
    char *filename = NULL;
    char *menuname = NULL;
    char *typestr = NULL;
    lst_type_t type = LST_TYPE_NORMAL;
    char *buf = strdup(buf_in);

    filename = strtok(buf, LST_SEP_CHAR "\r\n");
    menuname = strtok(NULL, LST_SEP_CHAR "\r\n");

    if (menuname != NULL) {
        typestr = strtok(NULL, "\r\n");
        if ((typestr != NULL) && (typestr[1] != 0)) {
            util_error("too long type!");
            goto fail;
        }
    }

    if (typestr != NULL) {
        for (type = LST_TYPE_NORMAL; type < LST_TYPE_NUM; ++type) {
            if (lst_type_char[type] == typestr[0]) {
                break;
            }
        }

        if (type == LST_TYPE_NUM) {
            util_error("invalid type '%c'!", typestr[0]);
            goto fail;
        }
    }

    switch (type) {
        case LST_TYPE_NORMAL:
        case LST_TYPE_HIDDEN:
        case LST_TYPE_ALIGN64K:
            res = main_flash_add_file(cart, filename, menuname, type == LST_TYPE_HIDDEN, type == LST_TYPE_ALIGN64K, 0);
            break;

        case LST_TYPE_EAPI:
            res = eapi_load(cart, filename) || eapi_inject(cart, 1);
            break;

        case LST_TYPE_BOOTO:
        case LST_TYPE_BOOTN:
            res = boot_load(cart, filename, type == LST_TYPE_BOOTO);
            break;

        case LST_TYPE_LOADERO:
        case LST_TYPE_LOADERN:
            res = loader_load(cart, filename, type == LST_TYPE_LOADERO);
            break;

        default:
            break;
    }

    free(buf);
    return res;

fail:
    free(buf);
    return -1;
}

#define BUFSIZE     2048

int lst_load(easyflash_cart_t * cart, const char *filename)
{
    FILE *fd = NULL;
    char buf[BUFSIZE];

    fd = fopen(filename, MODE_READ_TEXT);

    if (fd == NULL) {
        util_error("problems opening file '%s'!", filename);
        return -1;
    }

    do {
        buf[0] = 0;

        if (fgets(buf, BUFSIZE - 1, fd)) {
            char *p;
            size_t len = strlen(buf);

            if (len == 0) {
                break;
            }

            /* remove newline */
            buf[len - 1] = 0;

            /* remove comments */
            if ((p = strchr(buf, '#'))) {
                *p = 0;
            }

            if (*buf != 0) {
                if (lst_parse_entry(cart, buf) < 0) {
                    goto fail;
                }
            }
        }
    } while (!feof(fd));

    fclose(fd);
    return main_flash_place_entries(cart);

fail:
    fclose(fd);
    return -1;
}

/* ------------------------------------------------------------------------- */

int lst_save_begin(const char *filename)
{
    if (fd_o != NULL) {
        util_error("already open!", filename);
        return -1;
    }

    fd_o = fopen(filename, MODE_WRITE_TEXT);

    if (fd_o == NULL) {
        util_error("problems creating file '%s'!", filename);
        return -1;
    }

    fprintf(fd_o,
            "# ndefpack v" PACKAGE_VERSION " LST file\n"
            "#\n"
            "# Format: (INITEM)\n"
            "#   FILE[" LST_SEP_CHAR "NAME[" LST_SEP_CHAR "TYPE]]\n"
            "#     FILE - CRT or PRG file:\n"
            "#       CRT\n"
            "#         A cartridge .crt file.\n"
            "#         The following cart types are supported:\n"
            "#           * generic (CRT ID 0; 8k, 16k, Ultimax)\n"
            "#           * Ocean (CRT ID 5)\n"
            "#           * EasyFlash xbank (CRT ID 33)\n"
            "#       PRG\n"
            "#         Normal .prg file.\n"
            "#     NAME - name on EasyFS, shown by loader (cleaned up FILE if omitted)\n"
            "#     TYPE - type of entry; one of the following:\n"
            "#       t - normal (default if omitted)\n"
            "#       h - hidden, not shown by loader\n"
            "#       a - force 64k alignment (only valid for xbank carts)\n"
            "#       e - EAPI; file is 2+768 B, load address $C000 and \"eapi\" signature\n"
            "#       o - Ocean boot code; file is <= 2+1256 B, load address >= $FB18\n"
            "#       O - Ocean loader; file is 2+8192 B, load address = $A000\n"
            "#       n - normal boot code; file is <= 2+1256 B, load address >= $FB18\n"
            "#       N - normal loader; file is 2+8192 B, load address = $8000\n"
            "#\n"
            "\n"
           );

    return lst_check_write_error();
}

int lst_save_add(const char *filename, const char *menuname, lst_type_t type, int comment_only)
{
    char typesuffix[3] = { '\0', '\0', '\0' };

    if (type != LST_TYPE_NORMAL) {
        typesuffix[0] = LST_SEP_CHAR[0];
        typesuffix[1] = lst_type_char[type];
    }

    fprintf(fd_o, "%s%s%c%s%s\n", comment_only ? "# " : "", filename, LST_SEP_CHAR[0], menuname ? menuname : "(none)", typesuffix);

    return lst_check_write_error();
}

int lst_save_finish(void)
{
    int rc;

    fprintf(fd_o, "\n# end\n\n");

    if ((rc = lst_check_write_error())) {
        return rc;
    }

    rc = fclose(fd_o);
    fd_o = NULL;

    return rc;
}
