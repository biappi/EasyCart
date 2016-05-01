/*
 * easyfs.h - EasyFS handling
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

#ifndef EASYFS_H
#define EASYFS_H

#include "types.h"

/* The size of a single encoded (on-Flash) EasyFS entry. */
#define EFS_ENTRY_LEN   24
/* The length of a EasyFS entry name. */
#define EFS_NAME_LEN    16

/* The EasyFS entry types. */
typedef enum efs_entry_type_e {
    EF_ENTRY_NONE = 0,
    /* .prg file */
    EF_ENTRY_PRG,
    EF_ENTRY_PRG_L,
    EF_ENTRY_PRG_H,
    /* generic cart types */
    EF_ENTRY_8K,
    EF_ENTRY_16K,
    EF_ENTRY_ULTIMAX_16K,
    EF_ENTRY_ULTIMAX_8K,
    /* Ocean */
    EF_ENTRY_OCEAN_512,
    EF_ENTRY_OCEAN,
    /* EasyFlash (subdir) */
    EF_ENTRY_EASYFLASH,
    /* EasyFlash xbank */
    EF_ENTRY_XBANK_ULT,
    EF_ENTRY_XBANK_8K,
    EF_ENTRY_XBANK_16K,
    /* end marker */
    EF_ENTRY_END
} efs_entry_type_t;

typedef struct easyflash_cart_s easyflash_cart_t;

/* Returns the type as a string. */
extern const char *efs_entry_type_string(efs_entry_type_t type);
/* Returns the on-Flash "flags" field of the type. */
extern unsigned char efs_entry_type_flags(efs_entry_type_t type);
/* Returns the cartridge mode of the type. */
extern unsigned char efs_entry_type_mode(efs_entry_type_t type);
/* Returns the CRT ID of the type. */
extern int efs_entry_type_crt_id(efs_entry_type_t type);

/* Convenience functions for checking the general type. */
extern int efs_entry_type_ocean(efs_entry_type_t type);
extern int efs_entry_type_ef(efs_entry_type_t type);
extern int efs_entry_type_xbank(efs_entry_type_t type);
extern int efs_entry_type_prg(efs_entry_type_t type);

/* Returns 1 if the "other_bank" refers to ROMH, 0 if to ROML. */
extern int efs_entry_type_other_is_romh(efs_entry_type_t type);

#define EFS_FLAG_HIDDEN     (1 << 0)
#define EFS_FLAG_ALIGN64K   (1 << 1)
#define EFS_FLAG_SUBDIR     (1 << 2)
#define EFS_FLAG_PLACED     (1 << 3)

/* Parsed EasyFS entry, including the contents and metadata. */
typedef struct efs_entry_s {
    /* The type of entry. Encoded to the "flags" field (offs 16). */
    efs_entry_type_t type;

    /* Starting bank. Equal to the encoded form (offs 17-18). */
    unsigned int bank;
    /* Data (cart, PRG) size. Equal to the encoded form (offs 21-23). */
    unsigned int size;

    /* Offset in bank. Not used for carts.
       For PRG, equal to the encoded form (offs 19-20). */
    unsigned int offset;

    /* Number of banks and "other" banks. Not used for PRGs.
         8k/16k: bank <-> ROML, other <-> ROMH
        Ultimax: bank <-> ROMH, other <-> ROML
       other_bank_num encoded to (parts of) the CrtUsage field (offs 19-20),
       bank_num is derived from:
           size == (bank_num + other_bank_num) * 8kB */
    unsigned int bank_num;
    unsigned int other_bank_num;
    /* Offset (in banks) from the first bank to the "other" bank.
       Encoded to (parts of) the CrtUsage field (offs 19-20). */
    unsigned int other_bank_off;

    /* Hidden, align 64k and subdir flags.
       Encoded to either the type field (hidden)
       or the CrtUsage field (align 64k, subdir).
       Also used to store the "placed" flag for the placement code. */
    unsigned char flags;

    /* Name of the entry (offs 0-15). ASCII, encoded as PETSCII. */
    char menuname[EFS_NAME_LEN + 1];

    /* The entry (cart, PRG) data.
       For xbank carts, an interleaved array of 1MB size.
       For the rest, a linear array with size given by "size".
       PRG entries include the 2B load address. */
    unsigned char *data;

    /* The rest of the struct is not encoded. */

    /* Filename of the entry. */
    char *filename;
    /* Weight of the entry, for the placement code. */
    unsigned int weight;
    /* Bank usage of the entry. Valid only if the entry is placed. */
    bank_used_t bank_used;
} efs_entry_t;

/* Clears the bank usage array. */
extern void efs_entry_clear_bank_used(efs_entry_t *entry_ptr);

extern void efs_entry_init(efs_entry_t *entry_ptr);
extern void efs_entry_shutdown(efs_entry_t *entry_ptr);

extern int efs_entry_inject(easyflash_cart_t * cart, efs_entry_t *entry_ptr, int index);
extern int efs_entry_extract(easyflash_cart_t * cart, efs_entry_t *entry_ptr);
extern int efs_entry_save(easyflash_cart_t * cart, const char *filename, efs_entry_t *entry_ptr);
extern void efs_entry_delete(easyflash_cart_t * cart, efs_entry_t *entry_ptr);

extern int efs_dump_all(easyflash_cart_t * cart, int show_banks, int save_files, const char *prefix);
extern int efs_parse_all(easyflash_cart_t * cart);

#endif
