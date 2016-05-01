//
//  Cart.h
//  EasyCart
//
//  Created by Antonio Malara on 01/05/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "flash.h"

/*
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

// - //

efs_entry_t main_flash_efs_entry(int i);
char * main_flash_efs_entry_menuname(int i);
 
*/

@interface Cart : NSObject

- (NSInteger)save:(const char *)filepath;
- (NSInteger)load:(const char *)filepath;

- (NSInteger)entryCount;
- (efs_entry_t)entryAt:(NSInteger)i;
- (char *)menuNameAt:(NSInteger)i;
- (const char *)typeAt:(NSInteger)i;

@property(readonly) main_flash_space_t main_flash_space;

- (NSInteger)addFile:(const char *)filepath;
- (void)removeEntryAt:(NSInteger)i;
- (void)swapEntriesAt:(NSInteger)a with:(NSInteger)b;

@end
