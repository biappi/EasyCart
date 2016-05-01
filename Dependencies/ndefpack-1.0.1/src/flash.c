/*
 * flash.c - main EasyFlash handling
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

#include "boot.h"
#include "cart.h"
#include "config.h"
#include "eapi.h"
#include "easyfs.h"
#include "efname.h"
#include "flash.h"
#include "loader.h"
#include "lib.h"
#include "lst.h"
#include "options.h"
#include "place.h"
#include "prg.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static void place_end_mark(easyflash_cart_t * cart)
{
    efs_entry_init(&cart->main_flash_efs[cart->main_flash_efs_num]);
    cart->main_flash_efs[cart->main_flash_efs_num].type = EF_ENTRY_END;
    efs_entry_inject(cart, &cart->main_flash_efs[cart->main_flash_efs_num], cart->main_flash_efs_num);
}

static void add_to_menu_last(easyflash_cart_t * cart, efs_entry_t *e)
{
    memcpy(&cart->main_flash_efs[cart->main_flash_efs_num], e, sizeof(efs_entry_t));
    ++cart->main_flash_efs_num;
    place_end_mark(cart);
}

static void del_from_menu_last(easyflash_cart_t * cart)
{
    --cart->main_flash_efs_num;
    efs_entry_delete(cart, &cart->main_flash_efs[cart->main_flash_efs_num]);
    place_end_mark(cart);
}

/* -------------------------------------------------------------------------- */

static int check_index(easyflash_cart_t * cart, int i)
{
    if ((i < 0) || (i >= cart->main_flash_efs_num)) {
        util_error("invalid index %i", i);
        return -1;
    }

    return 0;
}

static void swap_entry(easyflash_cart_t * cart, int i, int j)
{
    efs_entry_t e;

    memcpy(&e, &cart->main_flash_efs[i], sizeof(efs_entry_t));
    memcpy(&cart->main_flash_efs[i], &cart->main_flash_efs[j], sizeof(efs_entry_t));
    memcpy(&cart->main_flash_efs[j], &e, sizeof(efs_entry_t));
}

/* -------------------------------------------------------------------------- */

static void display_space(easyflash_cart_t * cart, int show_banknums)
{
    util_display_space(cart->main_flash_space.total,
                       cart->main_flash_space.top,
                       cart->main_flash_space.rom_16k,
                       cart->main_flash_space.rom_8k,
                       cart->main_flash_space.rom_u8k);

    util_display_bank_used(cart->main_flash_space.bank_used, show_banknums);
}

/* -------------------------------------------------------------------------- */

#define FILE_TYPE_PRG   0
#define FILE_TYPE_CRT   1

static int detect_file_type(const char *filename)
{
    int i;
    int len = (int)strlen(filename);

    i = len - 4;

    if (i < 0) {
        /* short filename without extension, assume PRG */
        return FILE_TYPE_PRG;
    }

    if (strncasecmp(&filename[i], ".crt", 4) == 0) {
        /* .crt extension found */
        return FILE_TYPE_CRT;
    }

    /* no .crt extension, assume PRG */
    return FILE_TYPE_PRG;
}

/* -------------------------------------------------------------------------- */

static void main_flash_data_init(easyflash_cart_t * cart)
{
    memset(cart->main_flash_data, 0xff, EASYFLASH_SIZE);
}

static void main_flash_data_shutdown(void)
{
}

static void main_flash_space_init(easyflash_cart_t * cart)
{
    memset(cart->main_flash_space.bank_used, 0, sizeof(cart->main_flash_space.bank_used));
    cart->main_flash_space.bank_used[0][0] = 0x2000;
    cart->main_flash_space.bank_used[1][0] = 0x2000;
    cart->main_flash_space.total = EASYFLASH_SIZE - (2 * 0x2000);
    cart->main_flash_space.top = EASYFLASH_SIZE - (2 * 0x2000);
    cart->main_flash_space.rom_16k = EASYFLASH_N_BANKS - 1;
    cart->main_flash_space.rom_8k = EASYFLASH_N_BANKS - 1;
    cart->main_flash_space.rom_u8k = EASYFLASH_N_BANKS - 1;
}

static void main_flash_space_shutdown(void)
{
}

/* -------------------------------------------------------------------------- */

void main_flash_init(easyflash_cart_t * cart)
{
    int i;

    main_flash_data_init(cart);
    main_flash_space_init(cart);

    for (i = 0; i <= EFS_ENTRIES_MAX; ++i) {
        efs_entry_init(&cart->main_flash_efs[i]);
    }

    cart->main_flash_efs_num = 0;
    cart->main_flash_state &= ~MAIN_STATE_HAVE_OCEAN;
    cart->main_flash_state &= ~MAIN_STATE_HAVE_OLDEFS;
    place_end_mark(cart);
    clear_placements(cart);
}

void main_flash_shutdown(easyflash_cart_t * cart)
{
    int i;

    main_flash_data_shutdown();
    main_flash_space_shutdown();

    for (i = 0; i <= EFS_ENTRIES_MAX; ++i) {
        efs_entry_shutdown(&cart->main_flash_efs[i]);
    }
}

/* -------------------------------------------------------------------------- */

int main_flash_load(easyflash_cart_t * cart, const char *filename)
{
    int i, res, have_ocean = 0;

    main_flash_shutdown(cart);
    main_flash_init(cart);

    if ((res = efcart_load(cart, filename)) < 0) {
        return res;
    }

    if ((res = efs_parse_all(cart)) < 0) {
        return res;
    }

    cart->main_flash_efs_num = res;
    place_end_mark(cart);

    /* detect if we have an Ocean cartridge */
    for (i = 0; i < res; ++i) {
        if (efs_entry_type_ocean(cart->main_flash_efs[i].type)) {
            have_ocean = 1;
            cart->main_flash_state |= MAIN_STATE_HAVE_OCEAN;
            break;
        }
    }

    clear_placements(cart);
    mark_places_of_old_entries(cart);

    if (eapi_detect(cart)) {
        cart->main_flash_state |= MAIN_STATE_HAVE_EAPI;
        eapi_extract(cart);
    }

    /* old EasyFS, do not extract obsolete boot & loader */
    if (cart->main_flash_state & MAIN_STATE_HAVE_OLDEFS) {
        return 0;
    }

    if (efname_extract(cart) < 0) {
        util_warning("cart has no EF-Name");
    }

    /* assume that we have boot & loader */
    if (have_ocean) {
        cart->main_flash_state |= MAIN_STATE_HAVE_BOOTO;
        cart->main_flash_state |= MAIN_STATE_HAVE_LOADERO;
    } else {
        cart->main_flash_state |= MAIN_STATE_HAVE_BOOTN;
        cart->main_flash_state |= MAIN_STATE_HAVE_LOADERN;
    }

    return boot_extract(cart, have_ocean) || loader_extract(cart, have_ocean);
}

int main_flash_save(easyflash_cart_t * cart, const char *filename)
{
    int i, res;
    int ocean = cart->main_flash_state & MAIN_STATE_HAVE_OCEAN;
    int custom_e;
    int custom_b;
    int custom_l;

    custom_e = cart->main_flash_state & MAIN_STATE_HAVE_EAPI;

    if (ocean) {
        custom_b = cart->main_flash_state & MAIN_STATE_HAVE_BOOTO;
        custom_l = cart->main_flash_state & MAIN_STATE_HAVE_LOADERO;
    } else {
        custom_b = cart->main_flash_state & MAIN_STATE_HAVE_BOOTN;
        custom_l = cart->main_flash_state & MAIN_STATE_HAVE_LOADERN;
    }

    if ((res = place_entries(cart)) < 0) {
        return res;
    }

    display_space(cart, 1);

    /* clear Flash data of any leftover cruft */
    main_flash_data_init(cart);

    if (add_eapi || custom_e) {
        eapi_inject(cart, custom_e);
    }

    if ((res = boot_inject(cart, ocean, custom_b)) < 0) {
        return res;
    }

    if ((res = loader_inject(cart, ocean, custom_l)) < 0) {
        return res;
    }

    cart->main_flash_state &= ~MAIN_STATE_HAVE_OLDEFS;

    if (outefname != NULL) {
        efname_set(outefname);
    }
    efname_inject(cart);

    for (i = 0; i <= cart->main_flash_efs_num; ++i) {
        if ((res = efs_entry_inject(cart, &cart->main_flash_efs[i], i)) < 0) {
            return res;
        }
    }

    if ((res = efcart_save(cart, filename)) < 0) {
        return res;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

/* comparison function for qsort */
static int efs_name_cmp(const void *p1, const void *p2)
{
    efs_entry_t *e1 = (efs_entry_t *)p1;
    efs_entry_t *e2 = (efs_entry_t *)p2;

    return strcmp(e1->menuname, e2->menuname);
}

/* -------------------------------------------------------------------------- */

int main_flash_entry_find(easyflash_cart_t * cart, const char *name)
{
    int i;

    if (strlen(name) > EFS_NAME_LEN) {
        util_warning("name longer than %i chars, truncating", EFS_NAME_LEN);
    }

    for (i = 0; i < cart->main_flash_efs_num; ++i) {
        if (strncmp(cart->main_flash_efs[i].menuname, name, EFS_NAME_LEN) == 0) {
            return i;
        }
    }

    return -1;
}

int main_flash_entry_sort(easyflash_cart_t * cart)
{
    if (cart->main_flash_efs_num < 2) {
        return 0;
    }

    util_message("Sorting entries...");

    /* sort the (table pointing to the) entries */
    qsort(cart->main_flash_efs, cart->main_flash_efs_num, sizeof(efs_entry_t), efs_name_cmp);

    return 0;
}

int main_flash_entry_swap(easyflash_cart_t * cart, int i, int j)
{
    if (check_index(cart, i) || check_index(cart, j)) {
        return -1;
    }

    if (i == j) {
        return 0;
    }

    util_message("Swapping entries %i and %i...", i, j);

    swap_entry(cart, i, j);

    return 0;
}

int main_flash_entry_name(easyflash_cart_t * cart, int i, const char *menuname)
{
    char buf[EFS_NAME_LEN + 1];

    if (check_index(cart, i)) {
        return -1;
    }

    if (strlen(menuname) > EFS_NAME_LEN) {
        util_warning("name longer than %i chars, truncating", EFS_NAME_LEN);
    }

    strncpy(buf, menuname, EFS_NAME_LEN);
    buf[EFS_NAME_LEN] = 0;

    util_message("Renaming entry %i to '%s'...", i, buf);
    strcpy(cart->main_flash_efs[i].menuname, buf);

    return 0;
}

int main_flash_entry_hide(easyflash_cart_t * cart, int i, int hidden)
{
    if (check_index(cart, i)) {
        return -1;
    }

    util_message("Setting entry %i '%s' hidden bit %s...", i, cart->main_flash_efs[i].menuname, hidden ? "on" : "off");

    if (hidden) {
        cart->main_flash_efs[i].flags |= EFS_FLAG_HIDDEN;
    } else {
        cart->main_flash_efs[i].flags &= ~EFS_FLAG_HIDDEN;
    }

    return 0;
}

int main_flash_entry_a64k(easyflash_cart_t * cart, int i, int align)
{
    int old_align;

    if (check_index(cart, i)) {
        return -1;
    }

    if (!efs_entry_type_xbank(cart->main_flash_efs[i].type)) {
        util_error("tried to set 64k alignment for non-xbank entry!");
        return -1;
    }

    util_message("Setting entry %i '%s' 64k alignment %s...", i, cart->main_flash_efs[i].menuname, align ? "on" : "off");

    old_align = (cart->main_flash_efs[i].flags & EFS_FLAG_ALIGN64K) ? 1 : 0;

    if (align) {
        cart->main_flash_efs[i].flags |= EFS_FLAG_ALIGN64K;
    } else {
        cart->main_flash_efs[i].flags &= ~EFS_FLAG_ALIGN64K;
    }

    if (align ^ old_align) {
        util_message("64k alignment changed, replacing...");

        if (clear_and_place_entries(cart) < 0) {
            util_error("placement failed, reverting 64k alignment!");

            /* revert to the old setting and place again */
            if (old_align) {
                cart->main_flash_efs[i].flags |= EFS_FLAG_ALIGN64K;
            } else {
                cart->main_flash_efs[i].flags &= ~EFS_FLAG_ALIGN64K;
            }

            if (clear_and_place_entries(cart) < 0) {
                util_error("reverting failed?!");
                /* should never happen */
                return -2;
            }

            return -1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int main_flash_add_file(easyflash_cart_t * cart, const char *filename, const char *menuname, int hidden, int force_align, int place_now)
{
    efs_entry_t e;
    int ftype, res = -1;

    if (cart->main_flash_efs_num >= EFS_ENTRIES_MAX) {
        util_error("all %i EasyFS entries already used!", EFS_ENTRIES_MAX);
        return -1;
    }

    efs_entry_init(&e);

    ftype = detect_file_type(filename);

    if (ftype == FILE_TYPE_CRT) {
        res = anycart_load(cart, filename, &e);
    } else if (ftype == FILE_TYPE_PRG) {
        res = prg_load(filename, &e);
    }

    if (res < 0) {
        goto fail;
    }

    if (force_align && !efs_entry_type_xbank(e.type)) {
        util_error("tried to force 64k align on non-xbank entry!");
        res = (res < 0) ? res : -5;
        goto fail;
    }

    e.filename = strdup(filename);

    if (menuname != NULL) {
        strncpy(e.menuname, menuname, EFS_NAME_LEN);
    } else if (e.menuname[0] == '\0') {
        char *new_menuname = make_menuname(filename);
        strncpy(e.menuname, new_menuname, EFS_NAME_LEN);
        lib_free(new_menuname);
    }
    e.menuname[EFS_NAME_LEN] = '\0';

    if (hidden) {
        e.flags |= EFS_FLAG_HIDDEN;
    }

    if (force_align) {
        e.flags |= EFS_FLAG_ALIGN64K;
    }

    if (e.size > cart->main_flash_space.total) {
        util_error("size $%06x exceeds total free space $%06x!", e.size, cart->main_flash_space.total);
        res = -6;
        goto fail;
    }

    switch (e.type) {
        case EF_ENTRY_PRG:
        case EF_ENTRY_PRG_L:
        case EF_ENTRY_PRG_H:
            break;

        case EF_ENTRY_OCEAN:
        case EF_ENTRY_OCEAN_512:
            if (cart->main_flash_state & MAIN_STATE_HAVE_OCEAN) {
                util_error("only one Ocean cart per EasyFlash .crt supported!");
                res = -2;
                goto fail;
            }

            cart->main_flash_state |= MAIN_STATE_HAVE_OCEAN;

            /* FALL THROUGH */

        case EF_ENTRY_XBANK_8K:
        case EF_ENTRY_8K:

        case EF_ENTRY_XBANK_ULT:
        case EF_ENTRY_ULTIMAX_8K:

        case EF_ENTRY_XBANK_16K:
        case EF_ENTRY_16K:
        case EF_ENTRY_ULTIMAX_16K:
            break;

        default:
            res = -1;
            goto fail;
    }

    add_to_menu_last(cart, &e);

    if (!place_now) {
        /* try to keep track of available space */
        cart->main_flash_space.total -= e.size;
        /* place later */
        return 0;
    }

    /* place on menu */
    if (clear_and_place_entries(cart) < 0) {
        del_from_menu_last(cart);
        return -1;
    }

    return 0;

fail:
    efs_entry_shutdown(&e);
    return res;
}

int main_flash_place_entries(easyflash_cart_t * cart)
{
    if (clear_and_place_entries(cart) < 0) {
        util_warning("placement failed, clearing all entries");
        while (cart->main_flash_efs_num > 0) {
            del_from_menu_last(cart);
        }
        return -1;
    }

    return 0;
}

int main_flash_del_entry(easyflash_cart_t * cart, int index)
{
    int ocean;

    if (check_index(cart, index)) {
        util_warning("(ignoring delete)");
        return 0;
    }

    util_message("Deleting entry %i '%s'...", index, cart->main_flash_efs[index].menuname);

    ocean = efs_entry_type_ocean(cart->main_flash_efs[index].type);

    for (; index < (cart->main_flash_efs_num - 1); ++index) {
        swap_entry(cart, index, index + 1);
    }

    del_from_menu_last(cart);

    if (ocean) {
        cart->main_flash_state &= ~MAIN_STATE_HAVE_OCEAN;
    }

    if (clear_and_place_entries(cart) < 0) {
        return -1;
    }

    return 0;
}

int main_flash_ext_entry(easyflash_cart_t * cart, int i, const char *filename)
{
    if (check_index(cart, i)) {
        return -1;
    }

    return efs_entry_save(cart, filename, &cart->main_flash_efs[i]);
}

/* -------------------------------------------------------------------------- */

void main_flash_display_space(easyflash_cart_t * cart)
{
    display_space(cart, 0);
}

int main_flash_dump_all(easyflash_cart_t * cart, int save_files, const char *prefix_in)
{
    const char *prefix = prefix_in ? prefix_in : "ndefdump";
    int ocean = cart->main_flash_state & MAIN_STATE_HAVE_OCEAN;
    int oldefs = cart->main_flash_state & MAIN_STATE_HAVE_OLDEFS;
    char *fname = NULL;

    util_message("%sing cart '%s'; mode %s, EAPI %s, %i entries.",
                 save_files ? "Dump" : "List",
                 efname_get(),
                 ocean ? "Ocean" : "normal",
                 (cart->main_flash_state & MAIN_STATE_HAVE_EAPI) ? eapi_name_get() : "(none)",
                 cart->main_flash_efs_num
                );

    if (save_files) {
        fname = lib_malloc((int)strlen(prefix) + 16);

        sprintf(fname, "%s.lst", prefix);
        if (lst_save_begin(fname) < 0) {
            return -1;
        }

        sprintf(fname, "%s_boot_%s.prg", prefix, ocean ? "ocm" : "nrm");
        if (0
            || (!oldefs && boot_extract(cart, ocean))
            || (!oldefs && boot_save(cart, fname, ocean))
            || lst_save_add(fname, "(boot)", ocean ? LST_TYPE_BOOTO : LST_TYPE_BOOTN, oldefs)) {
            return -1;
        }

        sprintf(fname, "%s_loader_%s.prg", prefix, ocean ? "ocm" : "nrm");
        if (0
            || (!oldefs && loader_extract(cart, ocean))
            || (!oldefs && loader_save(cart, fname, ocean))
            || lst_save_add(fname, "(loader)", ocean ? LST_TYPE_LOADERO : LST_TYPE_LOADERN, oldefs)) {
            return -1;
        }

        if (cart->main_flash_state & MAIN_STATE_HAVE_EAPI) {
            sprintf(fname, "%s_eapi.prg", prefix);
            if (0
                || eapi_extract(cart)
                || eapi_save(cart, fname)
                || lst_save_add(fname, "(eapi)", LST_TYPE_EAPI, 0)) {
                return -1;
            }
        }
    }

    if (efs_dump_all(cart, verbosity > 2, save_files, prefix) < 0) {
        goto fail;
    }

    if (save_files) {
        lst_save_finish();
    }

    display_space(cart, 1);

    lib_free(fname);
    return 0;

fail:
    if (save_files) {
        lst_save_finish();
    }
    lib_free(fname);
    return -1;
}
