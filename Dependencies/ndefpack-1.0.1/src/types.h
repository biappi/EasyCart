/*
 * types.h - common types and consts.
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

#ifndef TYPES_H
#define TYPES_H

#include "config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#define BYTE uint8_t
#define WORD uint16_t
#else /* !HAVE_STDINT_H */
#define BYTE unsigned char
#define WORD unsigned short
#endif /* HAVE_STDINT_H */

/* EasyFlash constants */
#define EASYFLASH_SIZE (1024 * 1024)
#define EASYFLASH_N_BANK_BITS 6
#define EASYFLASH_N_BANKS     (1 << (EASYFLASH_N_BANK_BITS))
#define EASYFLASH_BANK_MASK   ((EASYFLASH_N_BANKS) - 1)
#define EFS_ENTRIES_MAX 255

/* The amount of used bytes in bank.
   First index is for ROML/H (<-> 0/1). */
typedef WORD bank_used_t[2][EASYFLASH_N_BANKS];

#endif
