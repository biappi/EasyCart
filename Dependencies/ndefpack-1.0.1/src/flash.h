/*
 * flash.h - main EasyFlash handling
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

#ifndef FLASH_H
#define FLASH_H

#include "easyfs.h"
#include "types.h"

/* Raw 1MB Flash image. */
extern unsigned char main_flash_data[EASYFLASH_SIZE];

/* Table of EasyFS entries. */
extern efs_entry_t main_flash_efs[EFS_ENTRIES_MAX + 1];
/* Number of entries in the above table. */
extern int main_flash_efs_num;

/* Free space on Flash. */
typedef struct main_flash_space_s {
    /* Total amount. */
    int total;
    /* Continuous space on the top on Flash. */
    int top;
    /* Amount of free slots for 16k, 8k and Ultimax 8k carts. */
    int rom_16k;
    int rom_8k;
    int rom_u8k;
    /* Used bytes per bank & ROML/H. */
    bank_used_t bank_used;
} main_flash_space_t;

extern main_flash_space_t main_flash_space;

#define MAIN_STATE_HAVE_BOOTN   (1 << 0)
#define MAIN_STATE_HAVE_BOOTO   (1 << 1)
#define MAIN_STATE_HAVE_LOADERN (1 << 2)
#define MAIN_STATE_HAVE_LOADERO (1 << 3)
#define MAIN_STATE_HAVE_EAPI    (1 << 4)
#define MAIN_STATE_HAVE_OCEAN   (1 << 5)
#define MAIN_STATE_HAVE_OLDEFS  (1 << 6)

extern int main_flash_state;

extern void main_flash_init(void);
extern void main_flash_shutdown(void);

extern int main_flash_load(const char *filename);
extern int main_flash_save(const char *filename);

extern int main_flash_dump_all(int save_files, const char *prefix);
extern void main_flash_display_space(void);

extern int main_flash_add_file(const char *filename, const char *menuname, int hidden, int force_align, int place_now);
extern int main_flash_place_entries(void);
extern int main_flash_del_entry(int index);
extern int main_flash_ext_entry(int index, const char *filename);

extern int main_flash_entry_find(const char *name);
extern int main_flash_entry_swap(int i, int j);
extern int main_flash_entry_sort(void);
extern int main_flash_entry_name(int i, const char *menuname);
extern int main_flash_entry_hide(int i, int hidden);
extern int main_flash_entry_a64k(int i, int align);

#endif
