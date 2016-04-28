/*
 * util.h - misc. helper functions
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

#ifndef UTIL_H
#define UTIL_H

#include "types.h"

extern void util_message(const char* format, ...);
extern void util_warning(const char* format, ...);
extern void util_error(const char* format, ...);
extern void util_dbg(const char* format, ...);

extern void util_display_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k);
extern void util_display_bank_used(bank_used_t bank_used, int show_banknums);

extern int util_prgfile_load(const char *filename, unsigned char *data, unsigned int max_size, unsigned int align_data, unsigned int *load_addr);
extern int util_prgfile_save(const char *filename, unsigned char *data, unsigned int max_size, unsigned int *load_addr);

extern char *make_menuname(const char *filename);

#endif
