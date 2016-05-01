/*
 * options.h - cmdline options
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "flash.h"

extern int parse_initem(easyflash_cart_t * cart, const char *s);
extern int parse_options(int *argcptr, char **argv, int *i_out);
extern void usage_ef_options(void);

extern int verbosity;
extern int add_eapi;
extern const char *outefcrt_name;
extern const char *outefname;

#endif
