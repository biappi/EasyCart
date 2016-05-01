/*
 * cart.h - crt load/save and cart inject/extract
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

#ifndef CART_H
#define CART_H

/* CRT IDs */
#define CARTRIDGE_NONE                 -1
#define CARTRIDGE_CRT                   0
#define CARTRIDGE_OCEAN                 5
#define CARTRIDGE_EASYFLASH            32
#define CARTRIDGE_EASYFLASH_XBANK      33

/* CARTRIDGE_CRT subtypes  */
#define CARTRIDGE_ULTIMAX              -6
#define CARTRIDGE_GENERIC_8KB          -3
#define CARTRIDGE_GENERIC_16KB         -2

/* cartridge modes */
#define MODE_16K    0
#define MODE_ULT    1
#define MODE_8K     2
#define MODE_OFF    3

#include "flash.h"

extern int efcart_load(easyflash_cart_t * cart, const char *filename);
extern int efcart_save(easyflash_cart_t * cart, const char *filename);

extern int detect_cart_type(const char *filename, int *crt_id_out, const char **cart_name);

struct efs_entry_s;
extern int anycart_load(easyflash_cart_t * cart, const char *filename, struct efs_entry_s *entry_ptr);
extern int anycart_save(const char *filename, struct efs_entry_s *entry_ptr);

extern int anycart_inject(easyflash_cart_t * cart, struct efs_entry_s *entry_ptr);
extern int anycart_extract(easyflash_cart_t * cart, struct efs_entry_s *entry_ptr);

#endif
