/*
 * loader.c - handling of EasyFlash loader code
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
#include "loader.h"
#include "flash.h"
#include "fw.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

#define LOADER_NRM_OFFSET   0x00000
#define LOADER_OCM_OFFSET   0x82000
#define LOADER_SIZE         0x2000
#define LOADER_NRM_ADDR     0x8000
#define LOADER_OCM_ADDR     0xa000

static unsigned char loader_data_nrm[LOADER_SIZE];
static unsigned char loader_data_ocm[LOADER_SIZE];

/* -------------------------------------------------------------------------- */

static int loader_check(int ocean)
{
    if (main_flash_state & (ocean ? MAIN_STATE_HAVE_LOADERO : MAIN_STATE_HAVE_LOADERN)) {
        return 0;
    }

    util_error("no loader for %s mode available", ocean ? "Ocean" : "normal");
    return -1;
}

/* -------------------------------------------------------------------------- */

int loader_load(const char* filename, int ocean)
{
    unsigned char buf[LOADER_SIZE + 2];
    unsigned int load_addr = 0;

    util_message("Loading loader '%s' for %s mode...", filename, ocean ? "Ocean" : "normal");

    if (util_prgfile_load(filename, buf, LOADER_SIZE, 0, &load_addr) < 0) {
        return -1;
    }

    memcpy(ocean ? loader_data_ocm : loader_data_nrm, buf, LOADER_SIZE);

    main_flash_state |= (ocean ? MAIN_STATE_HAVE_LOADERO : MAIN_STATE_HAVE_LOADERN);
    return 0;
}

int loader_save(const char *filename, int ocean)
{
    unsigned int load_addr = ocean ? LOADER_OCM_ADDR : LOADER_NRM_ADDR;

    if (loader_check(ocean)) {
        return -1;
    }

    util_message("Saving loader '%s' for %s mode...", filename, ocean ? "Ocean" : "normal");

    return util_prgfile_save(filename, ocean ? loader_data_ocm : loader_data_nrm, LOADER_SIZE, &load_addr);
}

/* -------------------------------------------------------------------------- */

int loader_inject(int ocean, int custom)
{
    const unsigned char *p;

    if (custom) {
        if (loader_check(ocean)) {
            return -1;
        }
        p = ocean ? loader_data_ocm : loader_data_nrm;
    } else {
        p = ocean ? &easyloader_ocm_prg[2] : &easyloader_nrm_prg[2];
    }

    memcpy(&main_flash_data[ocean ? LOADER_OCM_OFFSET : LOADER_NRM_OFFSET], p, LOADER_SIZE);
    return 0;
}

int loader_extract(int ocean)
{
    if (loader_check(ocean)) {
        return -1;
    }

    memcpy(ocean ? loader_data_ocm : loader_data_nrm, &main_flash_data[ocean ? LOADER_OCM_OFFSET : LOADER_NRM_OFFSET], LOADER_SIZE);
    return 0;
}
