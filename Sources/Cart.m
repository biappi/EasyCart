//
//  Cart.m
//  EasyCart
//
//  Created by Antonio Malara on 01/05/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

#import "Cart.h"
#import "efname.h"

@implementation Cart
{
    easyflash_cart_t cart;
}

- (id)init;
{
    if ((self = [super init]) == nil)
        return nil;
    
    main_flash_init(&cart);
    
    return self;
}

- (NSInteger)save:(const char *)filepath;
{
    return main_flash_save(&cart, filepath);
}

- (NSInteger)load:(const char *)filepath;
{
    return main_flash_load(&cart, filepath);
}

- (NSInteger)entryCount;
{
    return cart.main_flash_efs_num;
}

- (efs_entry_t)entryAt:(NSInteger)i;
{
    return cart.main_flash_efs[i];
}

- (char *)menuNameAt:(NSInteger)i;
{
    return cart.main_flash_efs[i].menuname;
}

- (const char *)typeAt:(NSInteger)i;
{
    return efs_entry_type_string(cart.main_flash_efs[i].type);
}

- (main_flash_space_t)main_flash_space;
{
    return cart.main_flash_space;
}

- (NSInteger)addFile:(const char *)filepath;
{
    return main_flash_add_file(&cart, filepath, NULL, 0, 0, 1);
}

- (void)removeEntryAt:(NSInteger)i;
{
    main_flash_del_entry(&cart, (int)i);
}

- (void)swapEntriesAt:(NSInteger)a with:(NSInteger)b;
{
    main_flash_entry_swap(&cart, (int)a, (int)b);
}

- (char *)efname;
{
    return (char *)efname_get(&cart);
}

- (void)setEfname:(char *)efname;
{
    efname_set(&cart, efname);
}

- (BOOL)canRunEntryAt:(NSInteger)i;
{
    const char * type = [self typeAt:i];
    const size_t len  = strlen(type);
    return strncmp(type, "PRG", len) == 0;
}

@end
