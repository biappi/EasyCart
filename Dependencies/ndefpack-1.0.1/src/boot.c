/*
 * boot.c - handling of EasyFlash boot code
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
#include "boot.h"
#include "flash.h"
#include "fw.h"
#include "lib.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

#define BOOT_OFFSET     0x81b18
#define BOOT_SIZE       0x4e8
#define BOOT_ADDR       0xfb18

static unsigned char boot_data_nrm[BOOT_SIZE];
static unsigned char boot_data_ocm[BOOT_SIZE];

/* -------------------------------------------------------------------------- */

static int boot_check(int ocean)
{
    if (main_flash_state & (ocean ? MAIN_STATE_HAVE_BOOTO : MAIN_STATE_HAVE_BOOTN)) {
        return 0;
    }

    util_error("no boot for %s mode available", ocean ? "Ocean" : "normal");
    return -1;
}

/* -------------------------------------------------------------------------- */

int boot_load(const char* filename, int ocean)
{
    unsigned char buf[BOOT_SIZE + 2];
    unsigned int load_addr = 0;

    util_message("Loading boot '%s' for %s mode...", filename, ocean ? "Ocean" : "normal");

    if (util_prgfile_load(filename, buf, BOOT_SIZE, BOOT_ADDR, &load_addr) < 0) {
        return -1;
    }

    memcpy(ocean ? boot_data_ocm : boot_data_nrm, buf, BOOT_SIZE);

    main_flash_state |= (ocean ? MAIN_STATE_HAVE_BOOTO : MAIN_STATE_HAVE_BOOTN);
    return 0;
}

int boot_save(const char *filename, int ocean)
{
    unsigned int load_addr = BOOT_ADDR;

    if (boot_check(ocean)) {
        return -1;
    }

    util_message("Saving boot '%s' for %s mode...", filename, ocean ? "Ocean" : "normal");

    return util_prgfile_save(filename, ocean ? boot_data_ocm : boot_data_nrm, BOOT_SIZE, &load_addr);
}

/* -------------------------------------------------------------------------- */

int boot_inject(int ocean, int custom)
{
    unsigned char *p;

    if (custom) {
        if (boot_check(ocean)) {
            return -1;
        }
        p = ocean ? boot_data_ocm : boot_data_nrm;
    } else {
        const unsigned char *q = ocean ? easyloader_launcher_ocm_prg : easyloader_launcher_nrm_prg;
        unsigned int start_addr;

        p = lib_malloc(BOOT_SIZE);
        memset(p, 0xff, BOOT_SIZE);

        start_addr = q[0] + (q[1] << 8);
        memcpy(p + start_addr - BOOT_ADDR, &q[2], 0xffff - start_addr + 1);
    }

    memcpy(&main_flash_data[BOOT_OFFSET], p, BOOT_SIZE);

    if (!custom) {
        lib_free(p);
    }

    return 0;
}

int boot_extract(int ocean)
{
    if (boot_check(ocean)) {
        return -1;
    }

    memcpy(ocean ? boot_data_ocm : boot_data_nrm, &main_flash_data[BOOT_OFFSET], BOOT_SIZE);
    return 0;
}
