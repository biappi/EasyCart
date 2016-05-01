/*
 * eapi.c - EAPI handling
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

#include <string.h>

#include "config.h"
#include "eapi.h"
#include "flash.h"
#include "fw.h"
#include "petscii.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

#define EAPI_OFFSET     0x81800
#define EAPI_SIZE       0x300

#define EAPI_NAME_OFFSET (EAPI_OFFSET + 4)

#define EAPI_FILE_LOAD_ADDR 0xc000

static unsigned char eapi_data[EAPI_SIZE] = { 0 };

static char eapi_name_buf[16 + 1] = "(no EAPI)";

/* -------------------------------------------------------------------------- */

static int check_header(unsigned char *data)
{
    return strncmp((char *)data, "eapi", 4);
}

/* -------------------------------------------------------------------------- */

int eapi_detect(easyflash_cart_t * cart)
{
    return (check_header(&cart->main_flash_data[EAPI_OFFSET]) == 0) ? 1 : 0;
}

const char *eapi_name_get(void)
{
    return (const char *)eapi_name_buf;
}

/* -------------------------------------------------------------------------- */

int eapi_load(easyflash_cart_t * cart, const char* filename)
{
    unsigned char buf[EAPI_SIZE + 2];
    unsigned int load_addr = 0;

    util_message("Loading EAPI '%s'...", filename);

    if (util_prgfile_load(filename, buf, EAPI_SIZE, 0, &load_addr) < 0) {
        return -1;
    }

    if (load_addr != EAPI_FILE_LOAD_ADDR) {
        util_error("load address $%04x, expecting $%04x!", load_addr, EAPI_FILE_LOAD_ADDR);
        return -1;
    }

    if (check_header(buf) != 0) {
        util_error("file '%s' doesn't have EAPI header!", filename);
        return -1;
    }

    memcpy(eapi_data, buf, EAPI_SIZE);

    cart->main_flash_state |= MAIN_STATE_HAVE_EAPI;
    return 0;
}

int eapi_save(easyflash_cart_t * cart, const char *filename)
{
    unsigned int load_addr = EAPI_FILE_LOAD_ADDR;

    if (check_header(eapi_data) != 0) {
        if (eapi_extract(cart) < 0) {
            return -1;
        }
    }

    util_message("Saving EAPI '%s'...", filename);

    return util_prgfile_save(filename, eapi_data, EAPI_SIZE, &load_addr);
}

/* -------------------------------------------------------------------------- */

int eapi_inject(easyflash_cart_t * cart, int custom)
{
    const unsigned char *p;

    if (custom) {
        if (check_header(eapi_data) != 0) {
            return -1;
        }
        p = eapi_data;
    } else {
        p = &eapi_am29f040_prg[2];
    }

    memcpy(&cart->main_flash_data[EAPI_OFFSET], p, EAPI_SIZE);
    strncpy(eapi_name_buf, petscii_to_ascii((const char *)&cart->main_flash_data[EAPI_NAME_OFFSET]), 16);
    return 0;
}

int eapi_extract(easyflash_cart_t * cart)
{
    if (check_header(&cart->main_flash_data[EAPI_OFFSET]) != 0) {
        return -1;
    }

    memcpy(eapi_data, &cart->main_flash_data[EAPI_OFFSET], EAPI_SIZE);
    strncpy(eapi_name_buf, petscii_to_ascii((const char *)&cart->main_flash_data[EAPI_NAME_OFFSET]), 16);
    return 0;
}
