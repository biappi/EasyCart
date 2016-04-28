/*
 * place.c - placement of data in Flash
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

#include "config.h"
#include "easyfs.h"
#include "flash.h"
#include "place.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

/* The amount of used bytes (== offset to first free byte) in bank. */
static bank_used_t flash_bank_used;

/* contains indices to main_flash_efs */
static int placement_order[EFS_ENTRIES_MAX + 1];

/* -------------------------------------------------------------------------- */

static int calc_top_space(int *bank_out, int *romlh_out)
{
    int bank, top_space, used, romlh = 0;

    for (bank = EASYFLASH_N_BANKS - 1; bank >= 0; --bank) {
        if (0
            || (flash_bank_used[0][bank])
            || (flash_bank_used[1][bank])) {
            ++bank;
            break;
        }
    }

    if ((bank > 0) && (bank <= EASYFLASH_N_BANKS)) {
        top_space = (EASYFLASH_N_BANKS - bank) * 0x4000;
        used = flash_bank_used[1][bank - 1];
        if (used < 0x2000) {
            --bank;
            top_space += (0x2000 - used);
            romlh = 1;
            if ((used == 0) && (flash_bank_used[0][bank] < 0x2000)) {
                top_space += (0x2000 - flash_bank_used[0][bank]);
                romlh = 0;
            }
        }
    } else {
        top_space = 0;
    }

    if (bank_out) {
        *bank_out = bank;
    }

    if (romlh_out) {
        *romlh_out = romlh;
    }

    return top_space;
}

static int calc_cart_space(int *roml_out, int *romh_out)
{
    int bank, rom16k = 0, rom8k = 0, romu8k = 0;

    for (bank = EASYFLASH_N_BANKS - 1; bank >= 0; --bank) {
        int roml = (flash_bank_used[0][bank] == 0);
        int romh = (flash_bank_used[1][bank] == 0);

        if (roml) {
            ++rom8k;
        }
        if (romh) {
            ++romu8k;
        }
        if (romh && roml) {
            ++rom16k;
        }
    }

    if (roml_out) {
        *roml_out = rom8k;
    }

    if (romh_out) {
        *romh_out = romu8k;
    }

    return rom16k;
}

static int calc_total_space(void)
{
    int bank, used, total_used = 0;

    for (bank = EASYFLASH_N_BANKS - 1; bank >= 0; --bank) {
        used = flash_bank_used[0][bank]
             + flash_bank_used[1][bank];

        total_used += used;
    }

    return EASYFLASH_SIZE - total_used;
}

static void update_space(void)
{
    main_flash_space.total = calc_total_space();
    main_flash_space.top = calc_top_space(NULL, NULL);
    main_flash_space.rom_16k = calc_cart_space(&main_flash_space.rom_8k, &main_flash_space.rom_u8k);
    memcpy(main_flash_space.bank_used, flash_bank_used, sizeof(flash_bank_used));
}

/* -------------------------------------------------------------------------- */

static int check_range_placed(int low, int high)
{
    int i;

    for (i = low; i < high; ++i) {
        if (!(main_flash_efs[i].flags & EFS_FLAG_PLACED)) {
            return 0;
        }
    }

    return 1;
}

static void shrink_range_placed(int *low_out, int *high_out)
{
    int low = *low_out;
    int high = *high_out;

    /* find first unplaced */
    while ((low < EFS_ENTRIES_MAX) && (main_flash_efs[placement_order[low]].flags & EFS_FLAG_PLACED)) {
        ++low;
    }

    /* find last unplaced */
    while ((high > 0) && (main_flash_efs[placement_order[high - 1]].flags & EFS_FLAG_PLACED)) {
        --high;
    }

    *low_out = low;
    *high_out = high;
}

/* -------------------------------------------------------------------------- */

static int find_free_banks(unsigned int bank_l_off, unsigned int bank_l_num, unsigned int bank_h_off, unsigned int bank_h_num, int step)
{
    int bank_l_top = bank_l_num + bank_l_off;
    int bank_h_top = bank_h_num + bank_h_off;
    int bank_top = (bank_l_top > bank_h_top) ? bank_l_top : bank_h_top;
    int i, j, empty;

    for (i = 0; i <= EASYFLASH_N_BANKS - bank_top; i += step) {
        empty = 1;
        for (j = 0; j < bank_top; ++j) {
            if (0
                || ((j >= bank_l_off) && (j < bank_l_top) && flash_bank_used[0][i + j])
                || ((j >= bank_h_off) && (j < bank_h_top) && flash_bank_used[1][i + j])) {
                empty = 0;
                if (step == 1) {
                    i += j;
                }
                break;
            }
        }
        if (empty) {
            break;
        }
    }

    if ((i + bank_top) >= EASYFLASH_N_BANKS) {
        return -1;
    }
    return i;
}

static int find_free_gap(int *gap_bank_out, int *gap_lh_out, int top_bank, unsigned int min_size)
{
    int bank = *gap_bank_out;
    int lh = *gap_lh_out;
    unsigned int size;

    int i, j;

    util_dbg("find gap bank %04x lh%i top %04x min %08x", bank, lh, top_bank, min_size);

    if (bank >= top_bank) {
        bank = 0;
        ++lh;
    }

    if (lh > 1) {
        return 0;
    }

    for (; lh < 2; ++lh) {
        for (i = bank; i < top_bank; ++i) {
            size = 0x2000 - flash_bank_used[lh][i];

            if (size == 0) {
                continue;
            }

            j = 1;

            while (((i + j) < top_bank) && flash_bank_used[lh][i + j] == 0) {
                size += 0x2000;
                ++j;
            }

            if (size >= min_size) {
                *gap_bank_out = i;
                *gap_lh_out = lh;
                return size;
            }
        }
        bank = 0;
    }

    *gap_bank_out = top_bank;
    *gap_lh_out = 1;
    return 0;
}

static void mark_used_banks(efs_entry_t *e, unsigned int bank_l, unsigned int bank_l_num, unsigned int bank_h, unsigned int bank_h_num)
{
    unsigned int i;

    if (e != NULL) {
        efs_entry_clear_bank_used(e);
    }

    for (i = bank_l; i < (bank_l + bank_l_num); ++i) {
        flash_bank_used[0][i] = 0x2000;
        if (e != NULL) {
            e->bank_used[0][i] = 0x2000;
        }
    }

    for (i = bank_h; i < (bank_h + bank_h_num); ++i) {
        flash_bank_used[1][i] = 0x2000;
        if (e != NULL) {
            e->bank_used[1][i] = 0x2000;
        }
    }
}

static void mark_used_prg(efs_entry_t *e, int bank, int size, int romlh, unsigned int len_in_bank, int toggle_romlh)
{
    if (e != NULL) {
        efs_entry_clear_bank_used(e);
    }

    while (size > 0) {
        if (len_in_bank > size) {
            len_in_bank = size;
        }

        flash_bank_used[romlh][bank] += len_in_bank;

        if (e != NULL) {
            e->bank_used[romlh][bank] = len_in_bank;
        }

        size -= len_in_bank;

        if (toggle_romlh) {
            romlh ^= 1;
            if (romlh == 0) {
                ++bank;
            }
        } else {
            ++bank;
        }

        len_in_bank = 0x2000 - flash_bank_used[romlh][bank];
    }
}

static void mark_used_prg_update(int prg_i, int bank, int size, int romlh, int toggle)
{
    unsigned int offset;

    offset = flash_bank_used[romlh][bank];
    main_flash_efs[prg_i].offset = offset + romlh * 0x2000;
    main_flash_efs[prg_i].bank = bank;

    if (toggle) {
        main_flash_efs[prg_i].type = EF_ENTRY_PRG;
    } else {
        main_flash_efs[prg_i].type = romlh ? EF_ENTRY_PRG_H : EF_ENTRY_PRG_L;
    }

    mark_used_prg(&(main_flash_efs[prg_i]), bank, size, romlh, 0x2000 - flash_bank_used[romlh][bank], toggle);
    main_flash_efs[prg_i].flags |= EFS_FLAG_PLACED;
}

static int place_prg_top(int prg_i)
{
    unsigned int size = main_flash_efs[prg_i].size;
    int bank = 0, romlh = 0;
    int top_space = calc_top_space(&bank, &romlh);

    util_dbg("prg %3i top size %06x space %06x", prg_i, size, top_space);

    if (size > top_space) {
        return -1;
    }

    mark_used_prg_update(prg_i, bank, size, romlh, 1);
    return 0;
}

static int place_prg_gap(int prg_i, int bank, int romlh)
{
    unsigned int size = main_flash_efs[prg_i].size;

    util_dbg("prg %3i gap size %06x bank %04x lh %i", prg_i, size, bank, romlh);

    mark_used_prg_update(prg_i, bank, size, romlh, 0);
    return 0;
}

static int place_crt(int i)
{
    efs_entry_type_t type;
    unsigned int bank_l_off, bank_l_num, bank_h_off, bank_h_num;
    int bank, step;

    if (main_flash_efs[i].flags & EFS_FLAG_PLACED) {
        return 0;
    }

    type = main_flash_efs[i].type;

    if (efs_entry_type_other_is_romh(type)) {
        bank_l_off = 0;
        bank_l_num = main_flash_efs[i].bank_num;
        bank_h_off = main_flash_efs[i].other_bank_off;
        bank_h_num = main_flash_efs[i].other_bank_num;
    } else {
        bank_h_off = 0;
        bank_h_num = main_flash_efs[i].bank_num;
        bank_l_off = main_flash_efs[i].other_bank_off;
        bank_l_num = main_flash_efs[i].other_bank_num;
    }

    if (main_flash_efs[i].flags & EFS_FLAG_ALIGN64K) {
        step = 0x10000 / 0x2000;
    } else {
        step = 0x2000 / 0x2000;
    }

    bank = find_free_banks(bank_l_off, bank_l_num, bank_h_off, bank_h_num, step);

    if (bank < 0) {
        return -1;
    }

    /* As Ocean carts are placed first, the previous call should have placed it at bank 0. */
    if (efs_entry_type_ocean(type) && (bank != 0)) {
        util_error("bug: bank %i for Ocean cart '%s'", bank, main_flash_efs[i].menuname);
        return -2;
    }

    main_flash_efs[i].bank = bank;

    util_dbg("crt %3i bank %04x l+%02x ln %02x h+%02x hn %02x", i, bank, bank_l_off, bank_l_num, bank_h_off, bank_h_num);

    mark_used_banks(&(main_flash_efs[i]), bank + bank_l_off, bank_l_num, bank + bank_h_off, bank_h_num);

    main_flash_efs[i].flags |= EFS_FLAG_PLACED;

    return 0;
}

static int place_all_prgs(int low, int high)
{
    int max_gap_bank = 0;
    int gap_bank = 0, gap_lh = 0;
    int i = 0;

    calc_top_space(&max_gap_bank, &i);

    shrink_range_placed(&low, &high);

    while (low < high) {
        int smallest_size;
        int gap_size;

        smallest_size = main_flash_efs[placement_order[high - 1]].size;
        gap_size = find_free_gap(&gap_bank, &gap_lh, max_gap_bank, smallest_size);

        if (gap_size == 0) {
            /* no gaps left to fit the smallest, just use the space at top */
            i = placement_order[low];

            if (place_prg_top(i)) {
                return -1;
            }
        } else {
            /* find the largest unplaced prg which fits the gap */
            int j;
            int size;

            for (j = low; j < high; ++j) {
                i = placement_order[j];
                size = main_flash_efs[i].size;

                if ((size <= gap_size) && ((main_flash_efs[i].flags & EFS_FLAG_PLACED) == 0)) {
                    place_prg_gap(i, gap_bank, gap_lh);
                    break;
                }
            }

            if (j == high) {
                util_error("bug: no prg fit gap size %06x! l %i h %i", gap_size, low, high);
                return -2;
            }
        }

        shrink_range_placed(&low, &high);
    }

    return 0;
}


/* -------------------------------------------------------------------------- */

/* comparison function for qsort */
static int weight_cmp(const void *p1, const void *p2)
{
    int i1 = *(int *)p1;
    int i2 = *(int *)p2;
    unsigned int w1 = main_flash_efs[i1].weight;
    unsigned int w2 = main_flash_efs[i2].weight;

    if (w1 < w2) {
        return 1;
    } else if (w1 > w2) {
        return -1;
    } else {
        return 0;
    }
}

/* Set weight for each entry. Larger weight -> placed earlier.
   Ocean carts have the strictest placement (must be bank 0),
   64k aligned xbank carts follow closely.
   PRG files have most flexibility and thus have lower weight than the carts. */
static void calc_weight(efs_entry_t *e)
{
    switch (e->type) {
        case EF_ENTRY_OCEAN:
        case EF_ENTRY_OCEAN_512:
            e->weight = (1 << 30);
            break;

        case EF_ENTRY_XBANK_8K:
        case EF_ENTRY_XBANK_16K:
        case EF_ENTRY_XBANK_ULT:
        case EF_ENTRY_8K:
        case EF_ENTRY_ULTIMAX_8K:
        case EF_ENTRY_ULTIMAX_16K:
        case EF_ENTRY_16K:
            {
                int other_bank_top = e->other_bank_off + e->other_bank_num;
                int bank_top = (e->bank_num > other_bank_top) ? e->bank_num : other_bank_top;

                e->weight = ((1 << 16) * bank_top) + e->other_bank_num;
            }

            if (e->flags & EFS_FLAG_ALIGN64K) {
                e->weight += (1 << 28);
            }
            break;


        case EF_ENTRY_PRG:
        case EF_ENTRY_PRG_L:
        case EF_ENTRY_PRG_H:
            e->weight = e->size;
            break;

        default:
            /* ? */
            e->weight = 0;
            break;
    }
}

static int order_entries(void)
{
    int i, num_crt = 0;

    for (i = 0; i < main_flash_efs_num; ++i) {
        calc_weight(&main_flash_efs[i]);
        placement_order[i] = i;
    }

    /* sort the (table of indices of) entries */
    qsort(placement_order, main_flash_efs_num, sizeof(int), weight_cmp);

    for (i = 0; i < main_flash_efs_num; ++i) {
        efs_entry_t *e;
        int j;

        j = placement_order[i];
        e = &main_flash_efs[j];

        if (!efs_entry_type_prg(e->type)) {
            ++num_crt;
        }

        util_dbg("%3i entry %3i w %08x size %06x '%s'", i, j, e->weight, e->size, e->menuname);
    }

    return num_crt;
}

/* -------------------------------------------------------------------------- */

int place_entries(void)
{
    int i, t_i, num_crt;
    int low = 0, high = main_flash_efs_num;

    if (check_range_placed(low, high)) {
        /* all placed, nothing to do */
        /* update space in case the last item was deleted */
        if (main_flash_efs_num == 0) {
            update_space();
        }
        return 0;
    }

    num_crt = order_entries();

    /* place carts */
    for (t_i = low; t_i < num_crt; ++t_i) {
        i = placement_order[t_i];

        if (main_flash_efs[i].flags & EFS_FLAG_PLACED) {
            if (t_i == low) {
                ++low;
            }
            if (t_i == (high - 1)) {
                --high;
            }
            continue;
        }

        if (place_crt(i) < 0) {
            util_error("cartridge '%s' didn't fit in!", main_flash_efs[i].menuname);
            return -3;
        }
    }

    low = t_i;

    if (place_all_prgs(low, high) < 0) {
        util_error("programs didn't fit in!");
        return -4;
    }

    update_space();

    return 0;
}

void mark_places_of_old_entries(void)
{
    int i;

    for (i = 0; i < main_flash_efs_num; ++i) {
        efs_entry_type_t type = main_flash_efs[i].type;

        if (efs_entry_type_prg(type)) {
            /* special case: PRG */
            int bank = main_flash_efs[i].bank;
            int offset = main_flash_efs[i].offset;
            int size = main_flash_efs[i].size;
            int toggle_romh = (type == EF_ENTRY_PRG) ? 1 : 0;
            int romh = (offset & 0x2000) ? 1 : 0;

            util_dbg("prg %3i bank %04x off %04x size %04x lh%i t%i", i, bank, offset, size, romh, toggle_romh);

            offset &= 0x1fff;

            mark_used_prg(&(main_flash_efs[i]), bank, size, romh, 0x2000 - offset, toggle_romh);
        } else {
            unsigned int bank_l, bank_l_num, bank_h, bank_h_num;

            if (efs_entry_type_other_is_romh(type)) {
                bank_l = main_flash_efs[i].bank;
                bank_l_num = main_flash_efs[i].bank_num;
                bank_h = main_flash_efs[i].bank + main_flash_efs[i].other_bank_off;
                bank_h_num = main_flash_efs[i].other_bank_num;
            } else {
                bank_h = main_flash_efs[i].bank;
                bank_h_num = main_flash_efs[i].bank_num;
                bank_l = main_flash_efs[i].bank + main_flash_efs[i].other_bank_off;
                bank_l_num = main_flash_efs[i].other_bank_num;
            }
            util_dbg("crt %3i bank l %04x ln %02x h %04x hn %02x", i, bank_l, bank_l_num, bank_h, bank_h_num);

            mark_used_banks(&(main_flash_efs[i]), bank_l, bank_l_num, bank_h, bank_h_num);
        }

        main_flash_efs[i].flags |= EFS_FLAG_PLACED;
    }

    update_space();
}

void clear_placements(void)
{
    int i;

    memset(flash_bank_used, 0, sizeof(flash_bank_used));

    /* reserve boot/efs/eapi bank */
    mark_used_banks(NULL, 0, 0, 0, 1);

    /* reserve loader bank */
    if (main_flash_state & MAIN_STATE_HAVE_OCEAN) {
        mark_used_banks(NULL, 0, 0, 1, 1);
    } else {
        mark_used_banks(NULL, 0, 1, 0, 0);
    }

    for (i = 0; i < main_flash_efs_num; ++i) {
        main_flash_efs[i].flags &= ~EFS_FLAG_PLACED;
    }
}

int clear_and_place_entries(void)
{
    clear_placements();
    return place_entries();
}
