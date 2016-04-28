/*
 * efname.c - EF-Name handling
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

#include <string.h>

#include "config.h"
#include "efname.h"
#include "flash.h"
#include "petscii.h"

/* -------------------------------------------------------------------------- */

#define EFNAME_MAGIC_OFFSET 0x81b00
#define EFNAME_OFFSET       0x81b08

static const char efname_magic[] = "ef-nAME:";

static char efname_buf[16 + 1] = "(no EF-Name)";

/* -------------------------------------------------------------------------- */

static int efname_detect(void)
{
    return (memcmp((char *)&main_flash_data[EFNAME_MAGIC_OFFSET], efname_magic, 8) == 0);
}

/* -------------------------------------------------------------------------- */

void efname_set(const char *name)
{
    strncpy(efname_buf, name, 16);
}

const char *efname_get(void)
{
    return (const char *)efname_buf;
}

int efname_inject(void)
{
    memcpy((char *)&main_flash_data[EFNAME_MAGIC_OFFSET], efname_magic, 8);
    strncpy((char *)&main_flash_data[EFNAME_OFFSET], ascii_to_petscii(efname_buf), 16);
    return 0;
}

int efname_extract(void)
{
    if (!efname_detect()) {
        return -1;
    }

    strncpy(efname_buf, petscii_to_ascii((const char *)&main_flash_data[EFNAME_OFFSET]), 16);
    return 0;
}
