/*
 * archdep.h - definitions for portability
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

#ifndef ARCHDEP_H
#define ARCHDEP_H

#include "config.h"

#ifdef IS_WINDOWS
#define DIR_SEP "\\"
#define MODE_READ_TEXT "rt"
#define MODE_WRITE_TEXT "wt"
#else
#define DIR_SEP "/"
#define MODE_READ_TEXT "r"
#define MODE_WRITE_TEXT "w"
#endif

#endif
