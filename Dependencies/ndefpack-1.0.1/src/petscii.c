/*
 * petscii.c - PETSCII conversion
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

#include "config.h"
#include "petscii.h"

const char *petscii_to_ascii(const char *txt)
{
    static char buf[16 + 1];

    char c;
    int i = 0;

    while ((i < 16) && (c = txt[i]) != '\0') {
        if (((unsigned char)c >= ('A' | 0x80)) && ((unsigned char)c <= ('Z' | 0x80))) {
            c &= 0x7f;
        } else if ((c & 0x80) || (c <= 0x1f)) {
            c = '.';
        } else if ((c >= 'a') && (c <= 'z')) {
            c = c - 'a' + 'A';
        } else if ((c >= 'A') && (c <= 'Z')) {
            c = c - 'A' + 'a';
        }

        buf[i++] = c;
    }

    buf[i] = '\0';

    return (const char *)buf;
}

const char *ascii_to_petscii(const char *txt)
{
    static char buf[16 + 1];

    char c;
    int i = 0;

    while ((i < 16) && (c = txt[i]) != '\0') {
        if ((c >= 'a') && (c <= 'z')) {
            c = c - 'a' + 'A';
        } else if ((c >= 'A') && (c <= 'Z')) {
            c = c - 'A' + 'a';
        }

        buf[i++] = c;
    }

    buf[i] = '\0';
    return (const char *)buf;
}
