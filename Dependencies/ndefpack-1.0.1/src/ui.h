/*
 * ui.h - UI API
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

#ifndef UI_H
#define UI_H

#include "types.h"

extern void ui_message(const char* msg);
extern void ui_warning(const char* msg);
extern void ui_error(const char* msg);

extern void ui_display_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k);
extern void ui_display_bank_used(bank_used_t bank_used, int show_banknums);

extern void usage(int show_helptext);

#endif
