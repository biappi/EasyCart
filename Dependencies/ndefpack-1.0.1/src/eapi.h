/*
 * eapi.h - EAPI handling
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

#ifndef EAPI_H
#define EAPI_H

#include "flash.h"

extern int eapi_detect(easyflash_cart_t * cart);
extern const char *eapi_name_get(void);

extern int eapi_inject(easyflash_cart_t * cart, int custom);
extern int eapi_extract(easyflash_cart_t * cart);

extern int eapi_load(easyflash_cart_t * cart, const char *filename);
extern int eapi_save(easyflash_cart_t * cart, const char *filename);

#endif
