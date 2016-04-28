/*
 * util.c - misc. helper functions
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

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "config.h"
#include "options.h"
#include "lib.h"
#include "types.h"
#include "ui.h"
#include "util.h"

/* ------------------------------------------------------------------------- */

#define MAX_MSG_LEN (1024 * 2)

static char msgbuf[MAX_MSG_LEN] = "";

void util_message(const char* format, ...)
{
    va_list ap;
    int len;

    va_start(ap, format);
    len = vsprintf(msgbuf, format, ap);
    va_end(ap);

    ui_message(msgbuf);
}

void util_warning(const char* format, ...)
{
    va_list ap;
    int len;

    len = sprintf(msgbuf, "Warning - ");

    va_start(ap, format);
    len = vsprintf(&msgbuf[len], format, ap);
    va_end(ap);

    ui_warning(msgbuf);
}

void util_error(const char* format, ...)
{
    va_list ap;
    int len;

    len = sprintf(msgbuf, "Error - ");

    va_start(ap, format);
    len = vsprintf(&msgbuf[len], format, ap);
    va_end(ap);

    ui_error(msgbuf);
}

void util_dbg(const char* format, ...)
{
    va_list ap;
    int len;

    if (verbosity < 3) {
        return;
    }

    len = sprintf(msgbuf, "debug: ");

    va_start(ap, format);
    len = vsprintf(&msgbuf[len], format, ap);
    va_end(ap);

    ui_message(msgbuf);
}

void util_display_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k)
{
    ui_display_space(top, total, rom_16k, rom_8k, rom_u8k);
}

void util_display_bank_used(bank_used_t bank_used, int show_banknums)
{
    ui_display_bank_used(bank_used, show_banknums);
}

/* ------------------------------------------------------------------------- */

int util_prgfile_load(const char *filename, unsigned char *data, unsigned int max_size, unsigned int align_data, unsigned int *load_addr_out)
{
    FILE *fd = NULL;
    unsigned char *buf;
    unsigned int load_addr;
    int prgsize = 0;
    unsigned int i = 0;
    unsigned int dest_i = 0;

    buf = lib_malloc(max_size + 3);
    memset(buf, 0xff, max_size + 2);

    fd = fopen(filename, "rb");

    if (fd == NULL) {
        util_error("problems opening file '%s'!", filename);
        goto fail;
    }

    prgsize = fread(buf, 1, max_size + 3, fd);

    if (ferror(fd)) {
        util_error("problems reading file '%s'!", filename);
        goto fail;
    }

    if (!feof(fd)) {
        util_error("file '%s' too long!", filename);
        goto fail;
    }

    fclose(fd);
    fd = NULL;

    load_addr = buf[0] | (buf[1] << 8);

    if (align_data > load_addr) {
        util_error("file '%s' load address $%04x is below alignment $%04x!", filename, load_addr, align_data);
        goto fail;
    }

    if (load_addr_out) {
        *load_addr_out = load_addr;

        /* skip load addr in actual data */
        i = 2;
        prgsize -= 2;
    }

    if (align_data > 0) {
        dest_i = load_addr - align_data;

        if ((dest_i + prgsize) > max_size) {
            util_error("file '%s' is too long after alignment!", filename);
            goto fail;
        }

        memset(data, 0xff, max_size);
    }

    memcpy(&data[dest_i], &buf[i], prgsize);

    lib_free(buf);
    return prgsize;

fail:
    lib_free(buf);
    fclose(fd);
    return -1;
}

int util_prgfile_save(const char *filename, unsigned char *data, unsigned int max_size, unsigned int *load_addr_in)
{
    FILE *fd = NULL;
    unsigned char buf[2];
    int add_addr = 0;

    if (load_addr_in) {
        add_addr = 1;
        buf[0] = (*load_addr_in) & 0xff;
        buf[1] = ((*load_addr_in) >> 8) & 0xff;
    }

    fd = fopen(filename, "wb");

    if (fd == NULL) {
        util_error("problems creating file '%s'!", filename);
        return -1;
    }

    if (0
        || (add_addr && (fwrite(buf, 2, 1, fd) != 1))
        || (fwrite(data, max_size, 1, fd) != 1)) {
        util_error("problems writing to file '%s'!", filename);
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;

}

/* ------------------------------------------------------------------------- */

char *make_menuname(const char *filename)
{
    char *p;
    char *menuname;

    p = strrchr(filename, DIR_SEP[0]);

#ifdef IS_WINDOWS
    if (p == NULL) {
        /* Windows allows also '/' as dir. separator */
        p = strrchr(filename, '/');
    }
#endif

    if (p != NULL) {
        menuname = strdup(p + 1);
    } else {
        menuname = strdup(filename);
    }

    p = strrchr(menuname, '.');

    if (p != NULL) {
        *p = '\0';
    }

    /* convert '_' to ' ' */
    p = menuname;
    while ((p = strchr(p, '_')) != NULL) {
        *p = ' ';
        ++p;
    }

    return menuname;
}
