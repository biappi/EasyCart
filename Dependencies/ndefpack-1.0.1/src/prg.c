/*
 * prg.c - PRG file handling
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

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "easyfs.h"
#include "flash.h"
#include "lib.h"
#include "prg.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

#define PRG_MAX_SIZE 0xe000

int prg_load(const char* filename, struct efs_entry_s *entry_ptr)
{
    int prgsize;
    unsigned char buf[PRG_MAX_SIZE];

    util_message("Loading PRG '%s'...", filename);

    prgsize = util_prgfile_load(filename, buf, PRG_MAX_SIZE - 2, 0, NULL);

    if (prgsize < 1) {
        return -1;
    }

    entry_ptr->size = (unsigned int)prgsize;
    entry_ptr->data = lib_malloc(prgsize);
    memcpy(entry_ptr->data, buf, prgsize);
    entry_ptr->type = EF_ENTRY_PRG;

    return 0;
}

int prg_save(easyflash_cart_t * cart, const char *filename, struct efs_entry_s *entry_ptr)
{
    if (entry_ptr->data == NULL) {
        if (prg_extract(cart, entry_ptr) < 0) {
            return -1;
        }
    }

    util_message("Saving PRG '%s'...", filename);

    return util_prgfile_save(filename, entry_ptr->data, entry_ptr->size, NULL);
}

/* -------------------------------------------------------------------------- */

static int prg_copydata(easyflash_cart_t * cart, efs_entry_t *entry_ptr, const int inject)
{
    unsigned int addr, bytes_left;
    unsigned char *p;
    const int is_interleaved = (entry_ptr->type == EF_ENTRY_PRG) ? 1 : 0;

    addr = entry_ptr->bank * 0x2000
         + (entry_ptr->offset & 0x1fff)
         + ((entry_ptr->offset & 0x2000) ? (1 << 19) : 0);

    bytes_left = entry_ptr->size;
    p = entry_ptr->data;

    while (bytes_left > 0) {
        unsigned int len_in_bank = (addr | 0x1fff) - addr + 1;

        if (len_in_bank > bytes_left) {
            len_in_bank = bytes_left;
        }

        if (inject) {
            memcpy(&cart->main_flash_data[addr], p, len_in_bank);
        } else {
            memcpy(p, &cart->main_flash_data[addr], len_in_bank);
        }

        p += len_in_bank;
        bytes_left -= len_in_bank;

        addr &= ~0x1fff;

        if (is_interleaved) {
            if (addr & (1 << 19)) {
                addr = addr - (1 << 19) + 0x2000;
            } else {
                addr += (1 << 19);
            }
        } else {
            addr += 0x2000;
        }

        if (addr > EASYFLASH_SIZE) {
            return -1;
        }
    }

    return 0;
}

int prg_inject(easyflash_cart_t * cart, struct efs_entry_s *entry_ptr)
{
    return prg_copydata(cart, entry_ptr, 1);
}

int prg_extract(easyflash_cart_t * cart, struct efs_entry_s *entry_ptr)
{
    entry_ptr->data = lib_realloc(entry_ptr->data, entry_ptr->size);

    return prg_copydata(cart, entry_ptr, 0);
}
