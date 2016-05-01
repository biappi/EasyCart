//
//  NdefpackUiCallbacks.c
//  EasyCart
//
//  Created by Antonio Malara on 28/04/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

#include <stdio.h>
#include "types.h"
#include "flash.h"

void ui_message(const char *msg)
{
}

void ui_warning(const char *msg)
{
}

void ui_error(const char *msg)
{
}

void ui_display_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k)
{
}

void ui_display_bank_used(bank_used_t bank_used, int show_banknums)
{
}

void usage(int show_helptext)
{
}

/* - */
/*
efs_entry_t main_flash_efs_entry(int i) {
    if (i < EFS_ENTRIES_MAX + 1)
        return main_flash_efs[i];
    else
        return  (efs_entry_t) { 0 };
}

char * main_flash_efs_entry_menuname(int i) {
    if (i < EFS_ENTRIES_MAX + 1)
        return main_flash_efs[i].menuname;
    else
        return NULL;
}
*/