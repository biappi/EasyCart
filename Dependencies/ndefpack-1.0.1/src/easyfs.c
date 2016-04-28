/*
 * easyfs.c - EasyFS handling
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

#include "cart.h"
#include "config.h"
#include "easyfs.h"
#include "flash.h"
#include "lib.h"
#include "lst.h"
#include "petscii.h"
#include "prg.h"
#include "types.h"
#include "util.h"

/* ------------------------------------------------------------------------- */

#define EFS_OFFSET      0x80000

#define EFS_TYPE_MASK 0x1f
#define EFS_RESERVED_MASK 0x60
#define EFS_HIDDEN_MASK 0x80

#define EFS2_CRT_OFFS_MSB_MASK 0x8000
#define EFS2_CRT_OFFS_SUBDIR_MASK 0x4000
#define EFS2_CRT_OFFS_64K_MASK 0x2000
#define EFS2_CRT_OFFS_OPP_OFF_MASK 0x1f00
#define EFS2_CRT_OFFS_OPP_OFF_SHIFT 8
#define EFS2_CRT_OFFS_OPP_NUM_MASK 0x00ff

/* ------------------------------------------------------------------------- */

static const struct efs_entry_type_info_s {
    const char *string;
    unsigned char flags;
    unsigned char mode;
    int crt_id;
} efs_entry_type_info_tbl[EF_ENTRY_END + 1] = {
    { "(none)",         0x00, MODE_OFF, CARTRIDGE_NONE },
    { "PRG",            0x01, MODE_OFF, CARTRIDGE_NONE },
    { "PRG (ROML)",     0x02, MODE_OFF, CARTRIDGE_NONE },
    { "PRG (ROMH)",     0x03, MODE_OFF, CARTRIDGE_NONE },
    { "8k",             0x10, MODE_8K,  CARTRIDGE_CRT },
    { "16k",            0x11, MODE_16K, CARTRIDGE_CRT },
    { "Ultimax 16k",    0x12, MODE_ULT, CARTRIDGE_CRT },
    { "Ultimax 8k",     0x13, MODE_ULT, CARTRIDGE_CRT },
    { "Ocean 512k",     0x14, MODE_8K,  CARTRIDGE_OCEAN },
    { "Ocean",          0x15, MODE_16K, CARTRIDGE_OCEAN },
    { "(EasyFlash)",    0x1a, MODE_ULT, CARTRIDGE_EASYFLASH },
    { "xbank Ult.",     0x1e, MODE_ULT, CARTRIDGE_EASYFLASH_XBANK },
    { "xbank 8k",       0x1c, MODE_8K,  CARTRIDGE_EASYFLASH_XBANK },
    { "xbank 16k",      0x1d, MODE_16K, CARTRIDGE_EASYFLASH_XBANK },
    { "(end marker)",   0x1f, MODE_OFF, CARTRIDGE_NONE }
};

const char *efs_entry_type_string(efs_entry_type_t type)
{
    return efs_entry_type_info_tbl[type].string;
}

unsigned char efs_entry_type_flags(efs_entry_type_t type)
{
    return efs_entry_type_info_tbl[type].flags;
}

unsigned char efs_entry_type_mode(efs_entry_type_t type)
{
    return efs_entry_type_info_tbl[type].mode;
}

int efs_entry_type_crt_id(efs_entry_type_t type)
{
    return efs_entry_type_info_tbl[type].crt_id;
}

int efs_entry_type_ocean(efs_entry_type_t type)
{
    return (efs_entry_type_info_tbl[type].crt_id == CARTRIDGE_OCEAN) ? 1 : 0;
}

int efs_entry_type_ef(efs_entry_type_t type)
{
    int crt_id = efs_entry_type_info_tbl[type].crt_id;

    return ((crt_id == CARTRIDGE_EASYFLASH_XBANK) || (crt_id == CARTRIDGE_EASYFLASH)) ? 1 : 0;
}

int efs_entry_type_xbank(efs_entry_type_t type)
{
    return (efs_entry_type_info_tbl[type].crt_id == CARTRIDGE_EASYFLASH_XBANK) ? 1 : 0;
}

int efs_entry_type_prg(efs_entry_type_t type)
{
    return ((type >= EF_ENTRY_PRG) && (type <= EF_ENTRY_PRG_H)) ? 1 : 0;
}

int efs_entry_type_other_is_romh(efs_entry_type_t type)
{
    return (efs_entry_type_info_tbl[type].flags & 2) ? 0 : 1;
}

/* ------------------------------------------------------------------------- */

static void erase_banks(unsigned int bank_l, unsigned int bank_l_num, unsigned int bank_h, unsigned int bank_h_num)
{
    unsigned int i;

    for (i = bank_l; i < (bank_l + bank_l_num); ++i) {
        memset(&main_flash_data[i * 0x2000], 0xff, 0x2000);
    }

    for (i = bank_h; i < (bank_h + bank_h_num); ++i) {
        memset(&main_flash_data[i * 0x2000 + (1 << 19)], 0xff, 0x2000);
    }
}

/* ------------------------------------------------------------------------- */

static int efs2_entry_parse(unsigned char *data, efs_entry_t *entry_ptr, int do_extract)
{
    unsigned char flags = data[16];
    unsigned int bank = data[17] | data[18];
    unsigned int offs = data[19] | (data[20] << 8);
    unsigned int size = data[21] | (data[22] << 8) | (data[23] << 16);
    unsigned int bank_num = 0;
    unsigned int other_bank_num = 0;
    unsigned int other_bank_off = 0;
    efs_entry_type_t i = EF_ENTRY_NONE;

    if (flags & EFS_HIDDEN_MASK) {
        entry_ptr->flags = EFS_FLAG_HIDDEN;
    }

    for (i = EF_ENTRY_NONE; i < EF_ENTRY_END; ++i) {
        if ((flags & EFS_TYPE_MASK) == efs_entry_type_info_tbl[i].flags) {
            entry_ptr->type = i;
            break;
        }
    }

    if (i == EF_ENTRY_END) {
        return -1;
    }

    if (flags & 0x10) {
        if (offs & EFS2_CRT_OFFS_SUBDIR_MASK) {
            entry_ptr->flags |= EFS_FLAG_SUBDIR;
        }
        if (offs & EFS2_CRT_OFFS_64K_MASK) {
            entry_ptr->flags |= EFS_FLAG_ALIGN64K;
        }

        other_bank_num = offs & EFS2_CRT_OFFS_OPP_NUM_MASK;
        other_bank_off = (offs & EFS2_CRT_OFFS_OPP_OFF_MASK) >> EFS2_CRT_OFFS_OPP_OFF_SHIFT;

        offs = 0;
        bank_num = (size / 0x2000) - other_bank_num;
    }

    strncpy(entry_ptr->menuname, petscii_to_ascii((const char *)data), 16);

    entry_ptr->bank = bank;
    entry_ptr->bank_num = bank_num;
    entry_ptr->other_bank_num = other_bank_num;
    entry_ptr->other_bank_off = other_bank_off;
    entry_ptr->offset = offs;
    entry_ptr->size = size;

    util_dbg("parse type %02x bank %04x num %02x other %02x off %02x offset %04x size %06x '%s'",
             flags & EFS_TYPE_MASK, bank, bank_num, other_bank_num, other_bank_off, offs, size, entry_ptr->menuname
            );

    if (do_extract) {
        return efs_entry_extract(entry_ptr);
    }

    return 0;
}


static int efs_entry_parse_old(unsigned char *data, efs_entry_t *entry_ptr, int do_extract)
{
    unsigned char flags = data[16];
    unsigned int bank = data[17];
    unsigned int offs = data[19] | (data[20] << 8);
    unsigned int size = data[21] | (data[22] << 8) | (data[23] << 16);
    unsigned int bank_num = 0;
    unsigned int other_bank_num = 0;
    unsigned int other_bank_off = 0;

    if (flags & EFS_HIDDEN_MASK) {
        entry_ptr->flags = EFS_FLAG_HIDDEN;
    }

    switch (flags & EFS_TYPE_MASK) {
        /* end of directory */
        case 0x1f:
            entry_ptr->type = EF_ENTRY_END;
            return 1;

        /* "invalid" entry */
        case 0x00:
            entry_ptr->type = EF_ENTRY_NONE;
            break;

        /* prg */
        case 0x01:
            entry_ptr->type = EF_ENTRY_PRG;
            break;

        /* cart types */
        case 0x10:
            if (size == (8 << 10)) {
                entry_ptr->type = EF_ENTRY_8K;
            } else if ((size == (32 << 10)) && (bank == 0)) {
                entry_ptr->type = EF_ENTRY_OCEAN;
            } else if ((size == (128 << 10)) && (bank == 0)) {
                entry_ptr->type = EF_ENTRY_OCEAN;
            } else if ((size == (512 << 10)) && (bank == 0)) {
                entry_ptr->type = EF_ENTRY_OCEAN_512;
            } else {
                /* WARNING: assumes ROML only */
                entry_ptr->type = EF_ENTRY_XBANK_8K;
            }
            bank_num = size / 0x2000;
            break;

        case 0x11:
            bank_num = size / 0x4000;
            other_bank_num = bank_num;

            if (size == (16 << 10)) {
                entry_ptr->type = EF_ENTRY_16K;
            } else if ((size == (256 << 10)) && (bank == 0)) {
                entry_ptr->type = EF_ENTRY_OCEAN;
                other_bank_off = bank_num;
            } else {
                /* WARNING: assumes ROML & ROMH "square" */
                entry_ptr->type = EF_ENTRY_XBANK_16K;
            }
            break;

        case 0x12:
            bank_num = size / 0x4000;
            other_bank_num = bank_num;

            if (size == (16 << 10)) {
                entry_ptr->type = EF_ENTRY_ULTIMAX_16K;
            } else {
                /* WARNING: assumes ROML & ROMH "square" */
                entry_ptr->type = EF_ENTRY_XBANK_ULT;
            }
            break;

        case 0x13:
            bank_num = size / 0x2000;
            if (size == (8 << 10)) {
                entry_ptr->type = EF_ENTRY_ULTIMAX_8K;
            } else {
                /* WARNING: assumes ROMH only */
                entry_ptr->type = EF_ENTRY_XBANK_ULT;
            }
            break;

        default:
            return -1;

    }

    strncpy(entry_ptr->menuname, petscii_to_ascii((const char *)data), 16);

    entry_ptr->bank = bank;
    entry_ptr->bank_num = bank_num;
    entry_ptr->other_bank_num = other_bank_num;
    entry_ptr->other_bank_off = other_bank_off;
    entry_ptr->offset = offs;
    entry_ptr->size = size;

    if (do_extract) {
        return efs_entry_extract(entry_ptr);
    }

    return 0;
}

static int efs_check_entry(unsigned char *data, int was_old)
{
    unsigned char flags = data[16];
    unsigned int bank = data[17] | (data[18] << 8);
    unsigned int offs = data[19] | (data[20] << 8);
    unsigned int size = data[21] | (data[22] << 8) | (data[23] << 16);
    unsigned char type = flags & EFS_TYPE_MASK;

    int is_old = 0;

    if ((flags & EFS_RESERVED_MASK) != EFS_RESERVED_MASK) {
        if (was_old == 0) {
            util_error("EasyFS2 detected but reserved bits are wrong! ($%02x)", flags & EFS_RESERVED_MASK);
            return -1;
        } else if (was_old == -1) {
            util_warning("reserved bits are $%02x, expecting $%02x; assuming old EasyFS", flags & EFS_RESERVED_MASK, EFS_RESERVED_MASK);
        }
        is_old = 1;
    }

    /* end of directory */
    if (type == 0x1f) {
        return 2;
    }

    /* cart types */
    if (type & 0x10) {
        if ((offs != 0) && (is_old || (was_old == 1))) {
            util_error("old EasyFS detected but CRT offset is not zero!");
            return -1;
        }

        if (!is_old) {
            if (!(offs & EFS2_CRT_OFFS_MSB_MASK)) {
                util_error("EasyFS2 detected but CRT offset MSb is zero!");
                return -1;
            }
            if ((offs & EFS2_CRT_OFFS_64K_MASK) && (type < 0x1c)) {
                util_error("64k alignment for non-xbank cart!");
                return -1;
            }
            if ((offs & EFS2_CRT_OFFS_SUBDIR_MASK) && !((type == 0x1a) || (type >= 0x1c))) {
                util_error("subdir flag set for non-EF/xbank cart!");
                return -1;
            }
        }
    }

    if ((type & 0x10) && (size & 0x1fff)) {
        util_error("cart size $%06x is not a multiple of $2000!", size & 0xffffffu);
        return -1;
    }

    if (bank > 0x3f) {
        util_error("bank $%04x > $003f", bank & 0xffffu);
        return -1;
    }

    if (size > 0x7ffff) {
        util_error("size $%06x > $7ffff", size & 0xffffffu);
        return -1;
    }

    if (is_old && ((type > 0x13) || ((type > 0x01) && (type < 0x10)))) {
        util_error("old EasyFS detected but new type $%02x found!", type);
        return -1;
    }

    switch (type) {
        /* deleted entry */
        case 0x00:
            break;

        /* prg */
        case 0x01:
        case 0x02:
        case 0x03:
            if (offs > 0x3fff) {
                util_error("offset $%04x > $3fff", offs & 0xffffu);
                return -1;
            }
            if (size > 0xefff) {
                util_error("size $%06x > $efff", size & 0xffffffu);
                return -1;
            }
            break;

        /* cart types */
        case 0x10:
        case 0x13:
            if (!is_old && (size != 0x2000)) {
                util_error("EasyFS2 detected but 8k/Ult. CRT size $%06x != $2000!", size & 0xffffffu);
                return -1;
            }
            break;

        case 0x11:
        case 0x12:
            if (!is_old && (size != 0x4000)) {
                util_error("EasyFS2 detected but 16k/Ult. CRT size $%06x != $4000!", size & 0xffffffu);
                return -1;
            }
            break;

        /* EFS2 cart types */
        case 0x14:
            if (size != (512 << 10)) {
                util_error("Ocean 512k size $%06x != $%06x!", size & 0xffffffu, (512 << 10));
                return -1;
            }
            break;

        case 0x15:
            if (!((size == (32 << 10)) || (size == (128 << 10)) || (size == (256 << 10)))) {
                util_error("Ocean CRT size $%06x != 32/128/256kB!", size & 0xffffffu);
                return -1;
            }
            break;

        /* xbank */
        case 0x1c:
        case 0x1d:
        case 0x1e:
            break;

        /* EasyFlash */
        case 0x1a:
            util_error("unsupported entry type $%02x", type);
            return -1;

        default:
            util_error("unknown entry type $%02x", type);
            return -1;

    }

    return is_old;
}

/* ------------------------------------------------------------------------- */

void efs_entry_clear_bank_used(efs_entry_t *entry_ptr)
{
    memset(entry_ptr->bank_used, 0, sizeof(entry_ptr->bank_used));
}

/* ------------------------------------------------------------------------- */

void efs_entry_init(efs_entry_t *entry_ptr)
{
    entry_ptr->type = EF_ENTRY_NONE;
    entry_ptr->bank = 0;
    entry_ptr->offset = 0;
    entry_ptr->size = 0;
    entry_ptr->bank_num = 0;
    entry_ptr->other_bank_num = 0;
    entry_ptr->other_bank_off = 0;
    entry_ptr->flags = 0;
    entry_ptr->filename = NULL;
    memset(entry_ptr->menuname, 0, sizeof(entry_ptr->menuname));
    entry_ptr->data = NULL;
    efs_entry_clear_bank_used(entry_ptr);
}

void efs_entry_shutdown(efs_entry_t *entry_ptr)
{
    entry_ptr->type = EF_ENTRY_NONE;
    entry_ptr->bank = 0;
    entry_ptr->offset = 0;
    entry_ptr->size = 0;
    entry_ptr->bank_num = 0;
    entry_ptr->other_bank_num = 0;
    entry_ptr->other_bank_off = 0;
    entry_ptr->flags = 0;

    if (entry_ptr->filename != NULL) {
        lib_free(entry_ptr->filename);
        entry_ptr->filename = NULL;
    }

    if (entry_ptr->data != NULL) {
        lib_free(entry_ptr->data);
        entry_ptr->data = NULL;
    }
}

/* ------------------------------------------------------------------------- */

static int efs_entry_code(efs_entry_t *entry_ptr, int index)
{
    unsigned char *p;
    unsigned int offset;
    unsigned char flags;

    p = &main_flash_data[EFS_OFFSET + (index * EFS_ENTRY_LEN)];

    if (entry_ptr->type == EF_ENTRY_END) {
        memset(p, 0xff, EFS_ENTRY_LEN);
        return 0;
    }

    flags = efs_entry_type_flags(entry_ptr->type);

    memset(p, 0, EFS_NAME_LEN);
    strncpy((char *)p, ascii_to_petscii((const char *)entry_ptr->menuname), EFS_NAME_LEN);

    p[16] = flags
          | EFS_RESERVED_MASK
          | ((entry_ptr->flags & EFS_FLAG_HIDDEN) ? EFS_HIDDEN_MASK : 0)
          ;

    offset = entry_ptr->offset;

    if (flags & 0x10) {
        offset |= EFS2_CRT_OFFS_MSB_MASK;

        if (entry_ptr->flags & EFS_FLAG_SUBDIR) {
            offset |= EFS2_CRT_OFFS_SUBDIR_MASK;
        }

        if (entry_ptr->flags & EFS_FLAG_ALIGN64K) {
            offset |= EFS2_CRT_OFFS_64K_MASK;
        }

        offset |= (entry_ptr->other_bank_num) & EFS2_CRT_OFFS_OPP_NUM_MASK;
        offset |= ((entry_ptr->other_bank_off) << EFS2_CRT_OFFS_OPP_OFF_SHIFT) & EFS2_CRT_OFFS_OPP_OFF_MASK;
    }

    p[17] = (entry_ptr->bank & 0xff);
    p[18] = ((entry_ptr->bank >> 8) & 0xff);
    p[19] = (offset & 0xff);
    p[20] = ((offset >> 8) & 0xff);
    p[21] = (entry_ptr->size & 0xff);
    p[22] = ((entry_ptr->size >> 8) & 0xff);
    p[23] = ((entry_ptr->size >> 16) & 0xff);

    util_dbg("code %3i bank %04x offset %04x size %06x '%s'", index, entry_ptr->bank, offset, entry_ptr->size, entry_ptr->menuname);
    return 0;
}

int efs_entry_inject(efs_entry_t *entry_ptr, int index)
{
    int rc = 0;

    switch (entry_ptr->type) {
        case EF_ENTRY_END:
        case EF_ENTRY_NONE:
            break;

        case EF_ENTRY_PRG:
        case EF_ENTRY_PRG_L:
        case EF_ENTRY_PRG_H:
            rc = prg_inject(entry_ptr);
            break;

        default:
            rc = anycart_inject(entry_ptr);
            break;
    }

    if (rc < 0) {
        return rc;
    }

    return efs_entry_code(entry_ptr, index);
}

int efs_entry_extract(efs_entry_t *entry_ptr)
{
    switch (entry_ptr->type) {
        case EF_ENTRY_END:
        case EF_ENTRY_NONE:
            break;

        case EF_ENTRY_PRG:
        case EF_ENTRY_PRG_L:
        case EF_ENTRY_PRG_H:
            return prg_extract(entry_ptr);

        case EF_ENTRY_OCEAN:
        case EF_ENTRY_OCEAN_512:
        case EF_ENTRY_XBANK_8K:
        case EF_ENTRY_XBANK_16K:
        case EF_ENTRY_XBANK_ULT:
        case EF_ENTRY_8K:
        case EF_ENTRY_ULTIMAX_8K:
        case EF_ENTRY_ULTIMAX_16K:
        case EF_ENTRY_16K:
            return anycart_extract(entry_ptr);

        default:
            return -1;
    }

    return 0;
}

int efs_entry_save(const char *filename, efs_entry_t *entry_ptr)
{
    switch (entry_ptr->type) {
        case EF_ENTRY_END:
        case EF_ENTRY_NONE:
            break;

        case EF_ENTRY_PRG:
        case EF_ENTRY_PRG_L:
        case EF_ENTRY_PRG_H:
            return prg_save(filename, entry_ptr);

        case EF_ENTRY_OCEAN:
        case EF_ENTRY_OCEAN_512:
        case EF_ENTRY_XBANK_8K:
        case EF_ENTRY_XBANK_16K:
        case EF_ENTRY_XBANK_ULT:
        case EF_ENTRY_8K:
        case EF_ENTRY_ULTIMAX_8K:
        case EF_ENTRY_ULTIMAX_16K:
        case EF_ENTRY_16K:
            return anycart_save(filename, entry_ptr);

        default:
            return -1;
    }

    return 0;
}

void efs_entry_delete(efs_entry_t *entry_ptr)
{
    switch (entry_ptr->type) {
        case EF_ENTRY_NONE:
        case EF_ENTRY_END:
            break;

        case EF_ENTRY_OCEAN:
        case EF_ENTRY_OCEAN_512:
        case EF_ENTRY_XBANK_8K:
        case EF_ENTRY_XBANK_16K:
        case EF_ENTRY_XBANK_ULT:
        case EF_ENTRY_8K:
        case EF_ENTRY_ULTIMAX_8K:
        case EF_ENTRY_ULTIMAX_16K:
        case EF_ENTRY_16K:
            {
                unsigned int bank = entry_ptr->bank;
                unsigned int bank_num = entry_ptr->bank_num;
                unsigned int other_bank_num = entry_ptr->other_bank_num;
                unsigned int other_bank_off = entry_ptr->other_bank_off;
                unsigned int roml, roml_num;
                unsigned int romh, romh_num;

                if (efs_entry_type_other_is_romh(entry_ptr->type)) {
                    roml = bank;
                    roml_num = bank_num;
                    romh = bank + other_bank_off;
                    romh_num = other_bank_num;
                } else {
                    romh = bank;
                    romh_num = bank_num;
                    roml = bank + other_bank_off;
                    roml_num = other_bank_num;
                }
                erase_banks(roml, roml_num, romh, romh_num);
            }
            break;

        case EF_ENTRY_PRG:
        case EF_ENTRY_PRG_L:
        case EF_ENTRY_PRG_H:
            {
                int size = entry_ptr->size;
                if (entry_ptr->data == NULL) {
                    entry_ptr->data = lib_malloc(size);
                }

                memset(entry_ptr->data, 0xff, size);
                prg_inject(entry_ptr);
            }
            break;

        default:
            /* ? */
            break;
    }

    efs_entry_shutdown(entry_ptr);
}

/* ------------------------------------------------------------------------- */

static void efs_entry_display(efs_entry_t *e, int i, int show_banks)
{
    util_message("%-3i %c%c %-17s %-13s $%02x  $%04x $%06x  %s",
                  i,
                  (e->flags & EFS_FLAG_HIDDEN) ? '*' : ' ',
                  (e->flags & EFS_FLAG_ALIGN64K) ? 'X' : ' ',
                  e->menuname,
                  efs_entry_type_string(e->type),
                  e->bank,
                  e->offset,
                  e->size,
                  e->filename ? e->filename : "(none)"
                );

    if (show_banks) {
        util_display_bank_used(e->bank_used, 0);
    }
}

int efs_dump_all(int show_banks, int save_files, const char *prefix)
{
    int i;
    efs_entry_t *e;
    unsigned char *data;
    char *filename = NULL;

    data = &main_flash_data[EFS_OFFSET];
    e = &main_flash_efs[0];

    if (save_files) {
        filename = lib_malloc(strlen(prefix) + 10);
    }

    util_message("#   HA Name              Type          Bank Offs  Size     Filename");

    for (i = 0; i < main_flash_efs_num; ++i, data += EFS_ENTRY_LEN, ++e) {
        efs_entry_display(e, i, show_banks);
        if (save_files) {
            lst_type_t lst_type = LST_TYPE_NORMAL;

            sprintf(filename, "%s_%03i.%s", prefix, i, efs_entry_type_prg(e->type) ? "prg" : "crt");

            if (efs_entry_save(filename, e) < 0) {
                util_error("problems saving entry %i", i);
                lib_free(filename);
                return -1;
            }

            if (e->flags & EFS_FLAG_ALIGN64K) {
                lst_type = LST_TYPE_ALIGN64K;
            } else if (e->flags & EFS_FLAG_HIDDEN) {
                lst_type = LST_TYPE_HIDDEN;
            } else {
                lst_type = LST_TYPE_NORMAL;
            }

            if (lst_save_add(filename, e->menuname, lst_type, 0) < 0) {
                lib_free(filename);
                return -1;
            }
        }
    }

    lib_free(filename);
    return 0;
}

int efs_parse_all(void)
{
    int i, num = 0, res, is_old = -1;
    efs_entry_t *e = &main_flash_efs[0];
    unsigned char *data = &main_flash_data[EFS_OFFSET];

    for (i = 0; i <= EFS_ENTRIES_MAX; ++i, data += EFS_ENTRY_LEN) {
        res = efs_check_entry(data, is_old);
        if (res < 0) {
            util_error("entry %i has errors", i);
            return res;
        } else if (res == 2) {
            num = i;
            break;
        }
        is_old = res;
    }

    if (is_old) {
        if (num == 0) {
            util_error("no EasyFS detected!");
            return -1;
        } else {
            util_warning("old EasyFS detected, will convert to new");
        }
    }

    data = &main_flash_data[EFS_OFFSET];
    for (i = 0; i < num; ++i, data += EFS_ENTRY_LEN, ++e) {
        if (is_old) {
            res = efs_entry_parse_old(data, e, 1);
        } else {
            res = efs2_entry_parse(data, e, 1);
        }
        if (res < 0) {
            util_error("entry %i has errors", i);
            return res;
        }
    }
    e->type = EF_ENTRY_END;

    if (is_old) {
        main_flash_state |= MAIN_STATE_HAVE_OLDEFS;
    }

    return num;
}
