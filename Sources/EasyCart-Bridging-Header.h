//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#include "boot.h"
#include "cart.h"
#include "config.h"
#include "eapi.h"
#include "easyfs.h"
#include "efname.h"
#include "flash.h"
#include "loader.h"
#include "options.h"
#include "types.h"
#include "ui.h"
#include "util.h"

/* - */

efs_entry_t main_flash_efs_entry(int i);
char * main_flash_efs_entry_menuname(int i);