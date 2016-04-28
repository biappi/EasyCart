/*
 * cart.c - crt load/save and cart inject/extract
 *
 * Written by
 *  Hannu Nuotio <nojoopa@users.sf.net>
 *
 * Based on {crt, easyflash, generic, ocean}.c from VICE, written by
 *  Andreas Boose <viceteam@t-online.de>
 *  ALeX Kazik <alx@kazik.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "util.h"

/* -------------------------------------------------------------------------- */

static char default_cartname[32 + 1] = PACKAGE_STRING;

static const char CRT_HEADER[] = "C64 CARTRIDGE   ";
//static const char CHIP_HEADER[] = "CHIP";

typedef struct crt_info_s {
    /* CRT ID (or subtype if negative). */
    int id;
    /* Name of the cartridge, stored in the CRT header. */
    char name[32 + 1];
    /* Cartridge mode (16k/Ult/8k/Off) as stored in the CRT header. */
    unsigned char mode;
    /* Detected cartridge mode.
       May differ with the above, used to detect broken .crt files. */
    unsigned char mode_d;
    /* Size of the cartridge. Multiple of 8kB. */
    unsigned int size;
    /* Offset and number of banks in ROML/H. */
    unsigned int bank_l;
    unsigned int bank_l_num;
    unsigned int bank_h;
    unsigned int bank_h_num;
} crt_info_t;

/* -------------------------------------------------------------------------- */

static void crt_info_init(crt_info_t *crt_ptr)
{
    crt_ptr->id = CARTRIDGE_NONE;
    memset(crt_ptr->name, 0, 32 + 1);
    crt_ptr->mode = MODE_OFF;
    crt_ptr->mode_d = MODE_OFF;
    crt_ptr->size = 0;
    crt_ptr->bank_l = 0;
    crt_ptr->bank_l_num = 0;
    crt_ptr->bank_h = 0;
    crt_ptr->bank_h_num = 0;
}

static void crt_info_make(crt_info_t *crt_ptr, efs_entry_t *entry_ptr)
{
    crt_ptr->id = efs_entry_type_crt_id(entry_ptr->type);
    memset(crt_ptr->name, 0, 32 + 1);
    strncpy(crt_ptr->name, entry_ptr->menuname, 16);
    crt_ptr->mode = efs_entry_type_mode(entry_ptr->type);
    crt_ptr->mode_d = MODE_OFF;
    crt_ptr->size = entry_ptr->size;

    if (efs_entry_type_other_is_romh(entry_ptr->type)) {
        crt_ptr->bank_l = 0;
        crt_ptr->bank_l_num = entry_ptr->bank_num;
        crt_ptr->bank_h = entry_ptr->other_bank_off;
        crt_ptr->bank_h_num = entry_ptr->other_bank_num;
    } else {
        crt_ptr->bank_h = 0;
        crt_ptr->bank_h_num = entry_ptr->bank_num;
        crt_ptr->bank_l = entry_ptr->other_bank_off;
        crt_ptr->bank_l_num = entry_ptr->other_bank_num;
    }
}

/* -------------------------------------------------------------------------- */

static int write_chip(FILE *fd, const unsigned char *data, unsigned int bank, unsigned int addr, unsigned int size, unsigned char type)
{
    unsigned char chipheader[0x10] = {
        'C', 'H', 'I', 'P',
        0, 0, ((size + 0x10) >> 8) & 0xff, (size + 0x10) & 0xff,
        0, type,
        (bank >> 8) & 0xff, bank & 0xff,
        (addr >> 8) & 0xff, addr & 0xff,
        (size >> 8) & 0xff, size & 0xff
    };

    if (fwrite(chipheader, 0x10, 1, fd) != 1) {
        return -1;
    }

    if (fwrite(data, size, 1, fd) != 1) {
        return -1;
    }

    return 0;
}

static int write_chip_if_not_empty(FILE *fd, const unsigned char *data, unsigned int bank, unsigned int addr)
{
    int i;

    for (i = 0; i < 0x2000; i++) {
        if (data[i] != 0xff) {
            return write_chip(fd, data, bank, addr, 0x2000, 2);
        }
    }

    return 0;
}

static int read_chip_hdr(FILE *fd, unsigned int *bank, unsigned int *offset, unsigned int *length)
{
    unsigned char chipheader[0x10];

    if (fread(chipheader, 0x10, 1, fd) < 1) {
        return -1;
    }

    *bank = (chipheader[0xa] << 8) | chipheader[0xb];
    *offset = (chipheader[0xc] << 8) | chipheader[0xd];
    *length = (chipheader[0xe] << 8) | chipheader[0xf];

    return 1;
}

/* -------------------------------------------------------------------------- */

static FILE *crt_load(const char* filename, crt_info_t *crt_ptr, int want_crt_id)
{
    unsigned char header[0x40];
    FILE *fd = NULL;

    fd = fopen(filename, "rb");

    if (fd == NULL) {
        util_error("problems opening file '%s'!", filename);
        return NULL;
    }

    if (fread(header, 0x40, 1, fd) < 1) {
        util_error("problems reading file '%s'!", filename);
        fclose(fd);
        return NULL;
    }

    if (strncmp((char*)header, CRT_HEADER, 16)) {
        util_error("file '%s' doesn't have CRT header!", filename);
        fclose(fd);
        return NULL;
    }

    crt_ptr->id = (header[0x17] + (header[0x16] * 256));
    crt_ptr->mode = ((header[0x18] & 1) | ((header[0x19] & 1) << 1));

    strncpy(crt_ptr->name, (const char *)&header[0x20], 32);
    crt_ptr->name[32] = '\0';

    if ((want_crt_id != CARTRIDGE_NONE) && (crt_ptr->id != want_crt_id)) {
        util_error("file '%s' has CRT ID %i instead of %i!", filename, crt_ptr->id, want_crt_id);
        fclose(fd);
        return NULL;
    }

    return fd;
}

static FILE *crt_save(const char* filename, crt_info_t *crt_ptr)
{
    unsigned char header[0x40];
    FILE *fd = NULL;

    if (filename == NULL) {
        return NULL;
    }

    fd = fopen(filename, "wb");

    if (fd == NULL) {
        util_error("problems creating file '%s'!", filename);
        return NULL;
    }

    memset(header, 0x0, 0x40);
    strcpy((char *)header, CRT_HEADER);

    header[0x13] = 0x40;
    header[0x14] = 0x01;
    header[0x17] = (unsigned char)(crt_ptr->id & 0xffu);
    header[0x18] = (unsigned char)(crt_ptr->mode & 1u);
    header[0x19] = (unsigned char)((crt_ptr->mode & 2u) >> 1);

    strncpy((char *)&header[0x20], (crt_ptr->name[0] != '\0') ? crt_ptr->name : default_cartname, 32);

    if (fwrite(header, 0x40, 1, fd) != 1) {
        util_error("problems writing to file '%s'!", filename);
        fclose(fd);
        return NULL;
    }

    return fd;
}

static int get_cart_params(int crt_id, const char **cart_name)
{
    switch (crt_id) {
        case CARTRIDGE_CRT:
            *cart_name = "generic";
            break;
        case CARTRIDGE_OCEAN:
            *cart_name = "Ocean";
            break;
        case CARTRIDGE_EASYFLASH:
            *cart_name = "EasyFlash";
            break;
        case CARTRIDGE_EASYFLASH_XBANK:
            *cart_name = "EasyFlash xbank";
            break;
        default:
            /* util_error("Unsupported CRT ID %i.", crt_id); */
            return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

static int efcart_load_crt(FILE *fd, unsigned char *data, crt_info_t *crt_ptr)
{
    unsigned int bank, offset, length, total = 0;
    unsigned int min_l = EASYFLASH_N_BANKS;
    unsigned int min_h = EASYFLASH_N_BANKS;
    unsigned int max_l = 0;
    unsigned int max_h = 0;
    int is_roml, is_romh;
    int has_roml = 0, has_romh = 0;

    memset(data, 0xff, EASYFLASH_SIZE);

    while (1) {
        if (read_chip_hdr(fd, &bank, &offset, &length) < 1) {
            break;
        }

        is_roml = (offset == 0x8000);
        is_romh = (offset == 0xa000) || (offset == 0xe000);

        if (bank >= EASYFLASH_N_BANKS || !(is_roml || is_romh)) {
            return -1;
        }

        if (length == 0x2000) {
            if (fread(&data[(bank << 13) | (is_roml ? 0 : (1 << 19))], 0x2000, 1, fd) < 1) {
                return -1;
            }
        } else if (length == 0x4000) {
            if (!is_roml) {
                return -1;
            }
            if (fread(&data[(bank << 13) | (0 << 19)], 0x2000, 1, fd) < 1) {
                return -1;
            }
            if (fread(&data[(bank << 13) | (1 << 19)], 0x2000, 1, fd) < 1) {
                return -1;
            }
            is_romh = 1;
        } else {
            return -1;
        }

        if (is_roml) {
            if (bank < min_l) {
                min_l = bank;
            }
            if (bank >= max_l) {
                max_l = bank + 1;
            }
            has_roml = 1;
        }
        if (is_romh) {
            if (bank < min_h) {
                min_h = bank;
            }
            if (bank >= max_h) {
                max_h = bank + 1;
            }
            has_romh = 1;
         }

        total += length;
    }

    crt_ptr->bank_l = has_roml ? min_l : 0;
    crt_ptr->bank_l_num = has_roml ? max_l - min_l : 0;
    crt_ptr->bank_h = has_romh ? min_h : 0;
    crt_ptr->bank_h_num = has_romh ? max_h - min_h : 0;
    crt_ptr->size = (crt_ptr->bank_l_num + crt_ptr->bank_h_num) * 0x2000;

    if (has_roml && has_romh) {
        crt_ptr->mode_d = MODE_16K;
    } else if (has_roml) {
        crt_ptr->mode_d = MODE_8K;
    } else {
        crt_ptr->mode_d = MODE_ULT;
    }

    return 0;
}

static int efcart_save_crt(const char *filename, const unsigned char *flash_data)
{
    FILE *fd;
    const unsigned char *data;
    crt_info_t crt_info;
    int bank;

    crt_info_init(&crt_info);
    crt_info.id = CARTRIDGE_EASYFLASH;
    crt_info.mode = MODE_ULT;

    fd = crt_save(filename, &crt_info);

    if (fd == NULL) {
        return -1;
    }

    for (bank = 0; bank < EASYFLASH_N_BANKS; bank++) {
        data = flash_data + bank * 0x2000;

        if (write_chip_if_not_empty(fd, data, bank, 0x8000) != 0) {
            fclose(fd);
            return -1;
        }

        data = flash_data + (1 << 19) + bank * 0x2000;

        if (write_chip_if_not_empty(fd, data, bank, 0xa000) != 0) {
            fclose(fd);
            return -1;
        }
    }
    fclose(fd);
    return 0;
}

static int efcart_save_xbank_crt(const char *filename, const unsigned char *flash_data, crt_info_t *crt_ptr)
{
    FILE *fd;
    const unsigned char *data;
    int bank;
    int bank_max;

    fd = crt_save(filename, crt_ptr);

    if (fd == NULL) {
        return -1;
    }

    bank_max = crt_ptr->bank_l + crt_ptr->bank_l_num;
    for (bank = crt_ptr->bank_l; bank < bank_max; bank++) {
        data = flash_data + bank * 0x2000;

        if (write_chip(fd, data, bank, 0x8000, 0x2000, 2) != 0) {
            fclose(fd);
            return -1;
        }
    }

    bank_max = crt_ptr->bank_h + crt_ptr->bank_h_num;
    for (bank = crt_ptr->bank_h; bank < bank_max; bank++) {
        data = flash_data + (1 << 19) + bank * 0x2000;

        if (write_chip(fd, data, bank, 0xa000, 0x2000, 2) != 0) {
            fclose(fd);
            return -1;
        }
    }

    fclose(fd);
    return 0;
}

/* -------------------------------------------------------------------------- */

static int cart_generic_load_crt(FILE *fd, unsigned char *data, crt_info_t *crt_ptr)
{
    unsigned int bank, offset, length;

    if (read_chip_hdr(fd, &bank, &offset, &length) < 1) {
        return -1;
    }

    if (length < 0x100) {
        return -1;
    }

    crt_ptr->size = 0;

    if ((offset == 0x8000) && (length <= 0x4000)) {
        if (fread(data, length, 1, fd) < 1) {
            return -1;
        }
        crt_ptr->id = (length <= 0x2000) ? CARTRIDGE_GENERIC_8KB : CARTRIDGE_GENERIC_16KB;
        crt_ptr->mode_d = (crt_ptr->id == CARTRIDGE_GENERIC_8KB) ? MODE_8K : MODE_16K;
        crt_ptr->size = length;
        crt_ptr->bank_l_num = 1;
        crt_ptr->bank_h_num = (crt_ptr->id == CARTRIDGE_GENERIC_16KB) ? 1 : 0;

        /* try to read next CHIP header in case of 16k Ultimax cart */
        if (read_chip_hdr(fd, &bank, &offset, &length) < 1) {
            return 0;
        }

        data += length;
    }

    if ((offset >= 0xe000) && ((offset + length) == 0x10000)) {
        if (fread(data + (offset & 0x1fff), length, 1, fd) < 1) {
            return -1;
        }
        crt_ptr->id = CARTRIDGE_ULTIMAX;
        crt_ptr->mode_d = MODE_ULT;
        crt_ptr->size += length;
        crt_ptr->bank_h_num = 1;
        return 0;
    }

    return -1;
}

static int cart_generic_save_crt(const char *filename, unsigned char *data, crt_info_t *crt_ptr)
{
    FILE *fd;
    unsigned int addr = 0x8000;
    unsigned int size = crt_ptr->size;

    fd = crt_save(filename, crt_ptr);

    if (fd == NULL) {
        return -1;
    }

    if (crt_ptr->mode == MODE_ULT) {
        addr = (size == 0x4000) ? 0x8000 : 0xe000;
        if (size < 0x2000) {
            size = 0x2000;
        }
    }

    if (write_chip(fd, data, 0, addr, size, 0) != 0) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

/* -------------------------------------------------------------------------- */

static int cart_ocean_load_crt(FILE *fd, unsigned char *data, crt_info_t *crt_ptr)
{
    unsigned int bank, offset, length;
    unsigned int banks_l = 0, banks_h = 0;
    int have_romh = 0;

    while (1) {
        if (read_chip_hdr(fd, &bank, &offset, &length) < 1) {
            break;
        }

        if ((bank > 0x3f) || (length != 0x2000) || ((offset != 0x8000) && (offset != 0xa000))) {
            return -1;
        }

        if (fread(&data[bank * 0x2000], 0x2000, 1, fd) < 1) {
            return -1;
        }

        if (offset == 0xa000) {
            have_romh = 1;
            ++banks_h;
        } else {
            ++banks_l;
        }
    }

    crt_ptr->size = (banks_l + banks_h) * 0x2000;
    crt_ptr->mode_d = (crt_ptr->size == (512 << 10)) ? MODE_8K : MODE_16K;

    crt_ptr->bank_l_num = banks_l;

    if (have_romh) {
        crt_ptr->bank_h = banks_l;
        crt_ptr->bank_h_num = banks_h;
    }

    return 0;
}

static int cart_ocean_save_crt(const char *filename, unsigned char *data, crt_info_t *crt_ptr)
{
    FILE *fd;
    unsigned int bank, bank_max;

    fd = crt_save(filename, crt_ptr);

    if (fd == NULL) {
        return -1;
    }

    bank_max = crt_ptr->bank_l_num;

    for (bank = 0; bank < bank_max; ++bank, data += 0x2000) {
        if (write_chip(fd, data, bank, 0x8000, 0x2000, 0) != 0) {
            fclose(fd);
            return -1;
        }

    }

    bank_max = crt_ptr->bank_h + crt_ptr->bank_h_num;

    for (bank = crt_ptr->bank_h; bank < bank_max; ++bank, data += 0x2000) {
        if (write_chip(fd, data, bank, 0xa000, 0x2000, 0) != 0) {
            fclose(fd);
            return -1;
        }
    }

    fclose(fd);
    return 0;
}

/* -------------------------------------------------------------------------- */

int efcart_load(const char *filename)
{
    FILE *fd = NULL;
    crt_info_t crt_info;

    crt_info_init(&crt_info);

    util_message("Loading EasyFlash cart '%s'...", filename);

    fd = crt_load(filename, &crt_info, CARTRIDGE_EASYFLASH);

    if (fd == NULL) {
        return -1;
    }

    if (efcart_load_crt(fd, main_flash_data, &crt_info) < 0) {
        util_error("problems reading file '%s'!", filename);
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

int efcart_save(const char *filename)
{
    util_message("Saving EasyFlash cart '%s'...", filename);

    if (efcart_save_crt(filename, main_flash_data) < 0) {
        util_error("problems saving file '%s'!", filename);
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int detect_cart_type(const char *filename, int *crt_id_out, const char **cart_name)
{
    FILE *fd = NULL;
    crt_info_t crt_info;

    crt_info_init(&crt_info);

    fd = crt_load(filename, &crt_info, CARTRIDGE_NONE);

    if (fd == NULL) {
        util_error("problems loading file '%s'!", filename);
        return -1;
    }

    fclose(fd);

    *crt_id_out = crt_info.id;

    return get_cart_params(*crt_id_out, cart_name);
}

/* -------------------------------------------------------------------------- */

int anycart_load(const char *filename, struct efs_entry_s *entry_ptr)
{
    FILE *fd = NULL;
    const char *cart_name = "???";
    int rc = -1;
    int resize = 1;
    unsigned int base_bank = 0;
    crt_info_t crt_info;

    crt_info_init(&crt_info);

    util_message("Loading unknown cart '%s'...", filename);

    fd = crt_load(filename, &crt_info, CARTRIDGE_NONE);

    if (fd == NULL) {
        return -1;
    }

    if (get_cart_params(crt_info.id, &cart_name) != 0) {
        util_error("Unsupported CRT ID %i.", crt_info.id);
        fclose(fd);
        return -1;
    }
    strncpy(entry_ptr->menuname, crt_info.name, 16);

    util_message("Loading %s cart '%s' (%s)...", cart_name, filename, crt_info.name);

    entry_ptr->data = lib_malloc(EASYFLASH_SIZE);

    switch (crt_info.id) {
        case CARTRIDGE_CRT:
            rc = cart_generic_load_crt(fd, entry_ptr->data, &crt_info);
            break;
        case CARTRIDGE_OCEAN:
            rc = cart_ocean_load_crt(fd, entry_ptr->data, &crt_info);
            break;
        case CARTRIDGE_EASYFLASH_XBANK:
            rc = efcart_load_crt(fd, entry_ptr->data, &crt_info);
            break;
        default:
            rc = -1;
            break;
    }

    fclose(fd);

    if (rc < 0) {
        util_error("problems loading file '%s'!", filename);
        goto fail;
    }

    switch (crt_info.id) {
        case CARTRIDGE_ULTIMAX:
            entry_ptr->type = (crt_info.bank_l_num == 0) ? EF_ENTRY_ULTIMAX_8K : EF_ENTRY_ULTIMAX_16K;
            break;
        case CARTRIDGE_GENERIC_8KB:
            entry_ptr->type = EF_ENTRY_8K;
            break;
        case CARTRIDGE_GENERIC_16KB:
            entry_ptr->type = EF_ENTRY_16K;
            break;

        case CARTRIDGE_OCEAN:
            switch (crt_info.size >> 10) {
                case 32:
                case 128:
                case 256:
                    entry_ptr->type = EF_ENTRY_OCEAN;
                    break;
                case 512:
                    entry_ptr->type = EF_ENTRY_OCEAN_512;
                    break;
                default:
                    util_error("invalid Ocean size %u kB", crt_info.size >> 10);
                    rc = -2;
                    goto fail;
            }
            break;

        case CARTRIDGE_EASYFLASH_XBANK:
            switch (crt_info.mode) {
                case MODE_ULT:
                    entry_ptr->type = EF_ENTRY_XBANK_ULT;
                    break;
                case MODE_8K:
                    entry_ptr->type = EF_ENTRY_XBANK_8K;
                    break;
                case MODE_16K:
                    entry_ptr->type = EF_ENTRY_XBANK_16K;
                    break;
                default:
                    util_error("xbank: cart with invalid mode in header!");
                    rc = -3;
                    goto fail;
            }

            /* keep the full 1MB cartridge image */
            resize = 0;
            break;
    }

    if (efs_entry_type_other_is_romh(entry_ptr->type)) {
        base_bank = crt_info.bank_l;
        entry_ptr->bank_num = crt_info.bank_l_num;
        entry_ptr->other_bank_num = crt_info.bank_h_num;
        entry_ptr->other_bank_off = crt_info.bank_h;
    } else {
        base_bank = crt_info.bank_h;
        entry_ptr->bank_num = crt_info.bank_h_num;
        entry_ptr->other_bank_num = crt_info.bank_l_num;
        entry_ptr->other_bank_off = crt_info.bank_l;
    }

    util_message("Cart ID %i, type %s, size $%06x, mode %i (%i in header), L %i, H %i",
                 crt_info.id,
                 efs_entry_type_string(entry_ptr->type),
                 crt_info.size,
                 crt_info.mode_d,
                 crt_info.mode,
                 crt_info.bank_l_num,
                 crt_info.bank_h_num
                );

    /* do some sanity checks */
    switch (entry_ptr->type) {
        case EF_ENTRY_8K:
        case EF_ENTRY_16K:
        case EF_ENTRY_ULTIMAX_8K:
        case EF_ENTRY_ULTIMAX_16K:
            if (crt_info.mode_d != crt_info.mode) {
                util_warning("detected mode %i doesn't match header mode %i, file possibly broken. Using detected mode.", crt_info.mode_d, crt_info.mode);
            }
            break;

        case EF_ENTRY_XBANK_8K:
        case EF_ENTRY_XBANK_16K:
        case EF_ENTRY_XBANK_ULT:
            if (crt_info.mode_d == MODE_OFF) {
                util_error("xbank: empty cart, refusing!");
                rc = -4;
                goto fail;
            }
            if (base_bank != 0) {
                util_error("xbank: nonzero base bank %u, cart unbootable!", base_bank);
                rc = -5;
                goto fail;
            }
            if (entry_ptr->bank_num == 0) {
                util_error("xbank: no banks on ROM%c for type %s, cart unbootable!", efs_entry_type_other_is_romh(entry_ptr->type) ? 'L' : 'H', efs_entry_type_string(entry_ptr->type));
                rc = -6;
                goto fail;
            }
            if (crt_info.mode_d != crt_info.mode) {
                util_warning("detected mode %i doesn't match header mode %i. Using header mode.", crt_info.mode_d, crt_info.mode);
            }
            break;

        default:
            break;
    }


    if (resize) {
        entry_ptr->data = lib_realloc(entry_ptr->data, crt_info.size);
    }
    entry_ptr->size = crt_info.size;

    return 0;

fail:
    lib_free(entry_ptr->data);
    entry_ptr->data = NULL;
    return rc;
}

int anycart_save(const char *filename, struct efs_entry_s *entry_ptr)
{
    int rc = -1;
    crt_info_t crt_info;

    crt_info_make(&crt_info, entry_ptr);

    util_message("Saving cart '%s'...", filename);
    util_dbg("Cart ID %i, type %s, size $%06x, mode %i, L %i, H %i",
              crt_info.id,
              efs_entry_type_string(entry_ptr->type),
              crt_info.size,
              crt_info.mode,
              crt_info.bank_l_num,
              crt_info.bank_h_num
             );

    switch (crt_info.id) {
        case CARTRIDGE_CRT:
            rc = cart_generic_save_crt(filename, entry_ptr->data, &crt_info);
            break;
        case CARTRIDGE_OCEAN:
            rc = cart_ocean_save_crt(filename, entry_ptr->data, &crt_info);
            break;
        case CARTRIDGE_EASYFLASH_XBANK:
            rc = efcart_save_xbank_crt(filename, entry_ptr->data, &crt_info);
            break;
        default:
            return -2;
    }

    if (rc < 0) {
        util_error("problems saving file '%s'!", filename);
    }

    return rc;
}

/* -------------------------------------------------------------------------- */

static void cart_copydata(struct efs_entry_s *entry_ptr, const int inject)
{
    unsigned int bank;
    unsigned int bank_l, bank_l_top, bank_l_off, bank_h, bank_h_top, bank_h_off;
    unsigned char *data_p;
    unsigned char *cart_p;
    int is_interleaved = efs_entry_type_ef(entry_ptr->type);

    if (efs_entry_type_other_is_romh(entry_ptr->type)) {
        bank_l = entry_ptr->bank;
        bank_l_top = entry_ptr->bank + entry_ptr->bank_num;
        bank_l_off = 0;
        bank_h = entry_ptr->bank + entry_ptr->other_bank_off;
        bank_h_top = entry_ptr->bank + entry_ptr->other_bank_off + entry_ptr->other_bank_num;
        bank_h_off = entry_ptr->other_bank_off;
    } else {
        bank_h = entry_ptr->bank;
        bank_h_top = entry_ptr->bank + entry_ptr->bank_num;
        bank_h_off = 0;
        bank_l = entry_ptr->bank + entry_ptr->other_bank_off;
        bank_l_top = entry_ptr->bank + entry_ptr->other_bank_off + entry_ptr->other_bank_num;
        bank_l_off = entry_ptr->other_bank_off;
    }

    cart_p = &main_flash_data[bank_l * 0x2000];
    data_p = &entry_ptr->data[bank_l_off * 0x2000];

    for (bank = bank_l; bank < bank_l_top; ++bank, cart_p += 0x2000, data_p += 0x2000) {
        if (inject) {
            memcpy(cart_p, data_p, 0x2000);
        } else {
            memcpy(data_p, cart_p, 0x2000);
        }
    }

    cart_p = &main_flash_data[bank_h * 0x2000 + (1 << 19)];

    if (is_interleaved) {
        data_p = &(entry_ptr->data[bank_h_off * 0x2000 + (1 << 19)]);
    }

    for (bank = bank_h; bank < bank_h_top; ++bank, cart_p += 0x2000, data_p += 0x2000) {
        if (inject) {
            memcpy(cart_p, data_p, 0x2000);
        } else {
            memcpy(data_p, cart_p, 0x2000);
        }
    }
}

int anycart_inject(struct efs_entry_s *entry_ptr)
{
    cart_copydata(entry_ptr, 1);
    return 0;
}

int anycart_extract(struct efs_entry_s *entry_ptr)
{
    unsigned int size;

    if (efs_entry_type_ef(entry_ptr->type)) {
        size = EASYFLASH_SIZE;
    } else {
        size = entry_ptr->size;
    }

    entry_ptr->data = lib_realloc(entry_ptr->data, size);
    memset(entry_ptr->data, 0xff, size);

    cart_copydata(entry_ptr, 0);
    return 0;
}
