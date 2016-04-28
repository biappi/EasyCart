/*
 * lib.c - memory handling helpers
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

#include <stdlib.h>

#include "config.h"
#include "lib.h"

void *lib_malloc(unsigned int size)
{
    void *ptr = malloc(size);

    if (ptr == NULL) {
        exit(EXIT_FAILURE);
    }

    return ptr;
}

void *lib_realloc(void *ptr, unsigned int size)
{
    ptr = realloc(ptr, size);

    if (ptr == NULL) {
        exit(EXIT_FAILURE);
    }

    return ptr;
}

void lib_free(void *ptr)
{
    free(ptr);
}
