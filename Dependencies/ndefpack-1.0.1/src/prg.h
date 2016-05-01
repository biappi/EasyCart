/*
 * prg.h - PRG file handling
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

#ifndef PRG_H
#define PRG_H

#include "flash.h"

struct efs_entry_s;
extern int prg_load(const char* filename, struct efs_entry_s *entry_ptr);
extern int prg_save(easyflash_cart_t * cart, const char* filename, struct efs_entry_s *entry_ptr);

extern int prg_inject(easyflash_cart_t * cart, struct efs_entry_s *entry_ptr);
extern int prg_extract(easyflash_cart_t * cart, struct efs_entry_s *entry_ptr);

#endif
