/*
 * lst.h - cartridge content list file handling
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

#ifndef LST_H
#define LST_H

enum lst_type_e {
    LST_TYPE_NORMAL = 0,
    LST_TYPE_HIDDEN,
    LST_TYPE_ALIGN64K,
    LST_TYPE_EAPI,
    LST_TYPE_BOOTO,
    LST_TYPE_LOADERO,
    LST_TYPE_BOOTN,
    LST_TYPE_LOADERN,
    LST_TYPE_NUM
};
typedef enum lst_type_e lst_type_t;

#define LST_SEP_CHAR    ","

extern int lst_parse_entry(const char *buf);
extern int lst_load(const char *filename);

extern int lst_save_begin(const char *filename);
extern int lst_save_add(const char *filename, const char *menuname, lst_type_t type, int comment_only);
extern int lst_save_finish(void);


#endif
