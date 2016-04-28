/*
 * ui_fltk.c - ndefgui FTLK v1.[13].x GUI implementation
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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Hold_Browser.H>

extern "C" {
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} /* extern "C" */


/* ------------------------------------------------------------------------- */

#define UI_WIN_W 620
#define UI_WIN_H 520

/* Offset from efs_list_ index to actual EFS entry index.
   1 comes from 1-based indexing of the widget, the other
   1 comes from the "# H A ..." line. */
#define UI_EFS_LIST_OFFSET  2

class ndef_ui_window : public Fl_Double_Window {
    public:
        ndef_ui_window(int w, int h, const char *label);
        ~ndef_ui_window();

        void update_filename(char *fname);
        void update_efname(void);
        void update_efs_list(int selected);
        void update_fws(void);
        void update_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k);
        void update_bank_used(void);
        void add_to_log(const char *txt);
        int save_log(const char *file);
        int get_index(void);

    private:
        Fl_Text_Buffer *log_buf_;
        Fl_Text_Display *log_disp_;

        Fl_Box *cartfilename_;
        Fl_Box *cartefname_;
        Fl_Box *cartmode_;
        Fl_Box *carteapiname_;
        Fl_Box *cartspace_;
        Fl_Box *bank_used_[2][EASYFLASH_N_BANKS];

        Fl_Hold_Browser *efs_list_;

        Fl_Button *button_add_;
        Fl_Button *button_del_;
        Fl_Button *button_up_;
        Fl_Button *button_down_;
        Fl_Button *button_sort_;
        Fl_Button *button_ren_;
        Fl_Button *button_ext_;

        int log_lines_;
        char spacetxt_[255];
};

static ndef_ui_window *ui_win = NULL;

/* ------------------------------------------------------------------------- */

static char filename[1024] = "";

/* ------------------------------------------------------------------------- */

static void ui_load_ef_crt_file(char *newfile)
{
    strcpy(filename, "");

    if (main_flash_load(newfile) < 0) {
        fl_alert("Error reading from file \'%s\'.", newfile);
        return;
    }

    strcpy(filename, newfile);

    if (ui_win != NULL) {
        ui_win->update_filename(filename);
        ui_win->update_efs_list(0);
        ui_win->update_fws();
        ui_win->update_efname();
        main_flash_display_space();
    }
}

static void ui_save_ef_crt_file(char *newfile)
{
    if (main_flash_save(newfile) < 0) {
        fl_alert("Error saving to file \'%s\'.", newfile);
    } else {
        strcpy(filename, newfile);
    }
}

/* ------------------------------------------------------------------------- */

void new_cb(Fl_Widget *, void *)
{
    filename[0] = '\0';
    main_flash_shutdown();
    main_flash_init();
    ui_win->update_filename(filename);
    ui_win->update_efs_list(0);
    ui_win->update_fws();
    ui_win->update_efname();
    main_flash_display_space();
}

void open_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Open EasyFlash file", "*.{crt,CRT}", filename);

    if (newfile != NULL) {
        ui_load_ef_crt_file(newfile);
    }
}

void saveas_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Save EasyFlash file as?", "*.crt", filename);

    if (newfile != NULL) {
        ui_save_ef_crt_file(newfile);
        strcpy(filename, newfile);
    }
}

void save_cb(Fl_Widget*, void*)
{
    if (filename[0] == '\0') {
        /* No filename - get one! */
        saveas_cb(NULL, NULL);
        return;
    } else {
        ui_save_ef_crt_file(filename);
    }
}

void savelog_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Save log as?", "*.txt", "log.txt");

    if (newfile == NULL) {
        return;
    }

    if (ui_win->save_log((const char *)newfile) != 0) {
        fl_alert("Problems saving log");
    }
}

void quit_cb(Fl_Widget *w, void *p)
{
    exit(0);
}

/* ------------------------------------------------------------------------- */

void inject_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Open PRG/CRT file", "*.{prg,PRG,crt,CRT}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (main_flash_add_file(newfile, NULL, 0, 0, 1) < 0) {
        fl_alert("Error adding file!");
    }

    ui_win->update_efs_list(main_flash_efs_num + UI_EFS_LIST_OFFSET - 1);
    ui_win->update_fws();
    main_flash_display_space();
}

void extract_cb(Fl_Widget*, void*)
{
    int i = ui_win->get_index();

    if (i < 0) {
        fl_alert("Nothing selected!");
        return;
    }

    char *newfile = fl_file_chooser("Save PRG/CRT file as?", "*.{prg,PRG,crt,CRT}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (main_flash_ext_entry(i, newfile) < 0) {
        fl_alert("Error extracting file!");
    }
}

void move_item_cb(Fl_Widget*, void *dir_v)
{
    int i = ui_win->get_index();

    if (i < 0) {
        fl_alert("Nothing selected!");
        return;
    }

    int dir = (int)(long)(dir_v) - 1;
    int j = i + dir;

    if ((j < 0) || (j >= main_flash_efs_num)) {
        return;
    }

    main_flash_entry_swap(i, j);
    ui_win->update_efs_list(j + UI_EFS_LIST_OFFSET);
}

void sort_items_cb(Fl_Widget*, void *)
{
    main_flash_entry_sort();
    ui_win->update_efs_list(0);
}

void rename_cb(Fl_Widget*, void*)
{
    int i = ui_win->get_index();

    if (i < -1) {
        fl_alert("Nothing selected!");
        return;
    }

    if (i >= 0) {
        const char *oldname = (const char *)main_flash_efs[i].menuname;
        const char *newname = fl_input("Enter name for item.", oldname);

        if (newname != NULL) {
            main_flash_entry_name(i, newname);
            ui_win->update_efs_list(i + UI_EFS_LIST_OFFSET);
        }
    } else {
        const char *oldname = efname_get();
        const char *newname = fl_input("Enter new EF-Name.", oldname);

        if (newname != NULL) {
            efname_set(newname);
            ui_win->update_efname();
        }
    }
}

void delete_cb(Fl_Widget*, void*)
{
    int i = ui_win->get_index();

    if (i < 0) {
        fl_alert("Nothing selected!");
        return;
    }

    if (main_flash_del_entry(i) < 0) {
        fl_alert("Error deleting item!");
    }
    ui_win->update_efs_list(i + UI_EFS_LIST_OFFSET);
    ui_win->update_fws();
    main_flash_display_space();
}

void hide_item_cb(Fl_Widget*, void *h_v)
{
    int i = ui_win->get_index();

    if (i < 0) {
        fl_alert("Nothing selected!");
        return;
    }

    int hidden = (int)(long)(h_v);

    main_flash_entry_hide(i, hidden);
    ui_win->update_efs_list(i + UI_EFS_LIST_OFFSET);
}

void align_item_cb(Fl_Widget*, void *a_v)
{
    int i = ui_win->get_index();

    if (i < 0) {
        fl_alert("Nothing selected!");
        return;
    }

    int align = (int)(long)(a_v);

    if (main_flash_entry_a64k(i, align) < 0) {
        fl_alert("Error aligning item!");
    }

    ui_win->update_efs_list(i + UI_EFS_LIST_OFFSET);
    main_flash_display_space();
}

/* ------------------------------------------------------------------------- */

void select_item_cb(Fl_Widget*, void *)
{
    ui_win->update_bank_used();
}

/* ------------------------------------------------------------------------- */

void inject_boot_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Open boot file", "*.{prg,PRG,bin,BIN}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (boot_load(newfile, main_flash_state & MAIN_STATE_HAVE_OCEAN) < 0) {
        fl_alert("Error adding file!");
        return;
    }
}

void extract_boot_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Save boot PRG file as?", "*.{prg,PRG}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (boot_save(newfile, main_flash_state & MAIN_STATE_HAVE_OCEAN) < 0) {
        fl_alert("Error extracting file!");
    }
}

void inject_loader_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Open loader file", "*.{prg,PRG}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (loader_load(newfile, main_flash_state & MAIN_STATE_HAVE_OCEAN) < 0) {
        fl_alert("Error adding file!");
        return;
    }
}

void extract_loader_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Save loader PRG file as?", "*.{prg,PRG}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (loader_save(newfile, main_flash_state & MAIN_STATE_HAVE_OCEAN) < 0) {
        fl_alert("Error extracting file!");
    }
}

void inject_eapi_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Open EAPI file", "*eapi*", NULL);

    if (newfile == NULL) {
        return;
    }

    if (eapi_load(newfile) < 0) {
        fl_alert("Error adding file!");
        return;
    }

    ui_win->update_fws();
}

void extract_eapi_cb(Fl_Widget*, void*)
{
    char *newfile = fl_file_chooser("Save EAPI PRG file as?", "*.{prg,PRG}", NULL);

    if (newfile == NULL) {
        return;
    }

    if (eapi_save(newfile) < 0) {
        fl_alert("Error extracting file!");
    }
}

/* ------------------------------------------------------------------------- */

void show_info_cb(Fl_Widget*, void*)
{
    main_flash_dump_all(0, NULL);
}

void dump_cb(Fl_Widget*, void*)
{
    main_flash_dump_all(1, NULL);
}

/* ------------------------------------------------------------------------- */

Fl_Menu_Item menuitems[] = {
    { "&File",                  0, NULL, NULL, FL_SUBMENU },
        { "&New image file",        0, (Fl_Callback *)new_cb },
        { "&Open image file...",    FL_CTRL + 'o', (Fl_Callback *)open_cb },
        { "&Save image file",       FL_CTRL + 's', (Fl_Callback *)save_cb },
        { "Save image file &as...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, NULL, FL_MENU_DIVIDER },
        { "E&xit",                  FL_CTRL + 'q', (Fl_Callback *)quit_cb, NULL },
        { NULL },

    { "&Image",                 0, NULL, NULL, FL_SUBMENU },
        { "&Add file...",       FL_ALT + 'a', (Fl_Callback *)inject_cb },
        { "&Extract file...",   FL_ALT + 'e', (Fl_Callback *)extract_cb, NULL, FL_MENU_DIVIDER },
        { "Move item &up",      FL_ALT + 'w', (Fl_Callback *)move_item_cb, (void *)0 },
        { "Move item &down",    FL_ALT + 's', (Fl_Callback *)move_item_cb, (void *)2 },
        { "&Sort items",        FL_ALT + 'g', (Fl_Callback *)sort_items_cb, NULL, FL_MENU_DIVIDER },
        { "&Rename item",       FL_ALT + 'r', (Fl_Callback *)rename_cb },
        { "&Delete item",       FL_ALT + 'd', (Fl_Callback *)delete_cb },
        { "&Hide item",         FL_ALT + 'h', (Fl_Callback *)hide_item_cb, (void *)1 },
        { "&Unhide item",       FL_ALT + 'u', (Fl_Callback *)hide_item_cb, (void *)0 },
        { "Align item",         FL_ALT + '6', (Fl_Callback *)align_item_cb, (void *)1 },
        { "Unalign item",       FL_ALT + '1', (Fl_Callback *)align_item_cb, (void *)0 },
        { NULL },

    { "&Firmware",              0, NULL, NULL, FL_SUBMENU },
        { "&Add EAPI...",       0, (Fl_Callback *)inject_eapi_cb },
        { "&Extract EAPI...",   0, (Fl_Callback *)extract_eapi_cb, NULL, FL_MENU_DIVIDER },
        { "&Add boot...",       0, (Fl_Callback *)inject_boot_cb },
        { "&Extract boot...",   0, (Fl_Callback *)extract_boot_cb, NULL, FL_MENU_DIVIDER },
        { "&Add loader...",     0, (Fl_Callback *)inject_loader_cb },
        { "&Extract loader...", 0, (Fl_Callback *)extract_loader_cb },
        { NULL },

    { "&Misc",                  0, NULL, NULL, FL_SUBMENU },
        { "&Dump all files",    0, (Fl_Callback *)dump_cb },
        { "&Show info",         0, (Fl_Callback *)show_info_cb },
        { "Save log &as...",    0, (Fl_Callback *)savelog_cb },
        { NULL },

    { NULL }
};

ndef_ui_window::ndef_ui_window(int w, int h, const char* label) : Fl_Double_Window(w, h, label)
{
    log_lines_ = 0;

    int x = 5;
    int y = 0;

    {
        Fl_Menu_Bar *m = new Fl_Menu_Bar(0, 0, w, 30);
        m->copy(menuitems);
        y += 30;
    }

    /* bottom: log */
    int yo = y + 370 + 25;
    {
        log_buf_ = new Fl_Text_Buffer();
        log_disp_ = new Fl_Text_Display(x, yo, w - x - 5, h - yo, "Log");
        log_disp_->align(FL_ALIGN_TOP_LEFT);
        log_disp_->buffer(log_buf_);
        log_disp_->hide_cursor();
        log_disp_->textfont(FL_COURIER);
        log_disp_->textsize(12);
    }

    /* bottom: bank map */
    int wb = 1 + (w - (2 * 5)) / EASYFLASH_N_BANKS;
    int hb = wb;
    yo -= hb + 30;
    {
        int yb = yo;
        int xb = 5;

        for (int lh = 0; lh < 2; ++lh, yb += (hb - 1), xb = 5) {
            for (int bank = 0; bank < EASYFLASH_N_BANKS; ++bank, xb += (wb - 1)) {
                bank_used_[lh][bank] = new Fl_Box(FL_BORDER_BOX, xb, yb, wb, hb, NULL);
            }
        }
    }

    /* top/center: cart image */
    int wt = 495;
    {
        y += 5;
        cartfilename_ = new Fl_Box(FL_NO_BOX, x, y, wt - x - 5 - 200, 20, "...");
        update_filename(filename);
        cartefname_ = new Fl_Box(FL_NO_BOX, x + wt - 5 - 200, y, 200, 20, "...");
        update_efname();
        cartmode_ = new Fl_Box(FL_NO_BOX, x + wt + 5, y, 80, 20, "...");
        y += 25;
    }
    {
        static const int cw[] = { 32, 16, 16, 150, 100, 44, 44, 60, 0 };

        efs_list_ = new Fl_Hold_Browser(x, y, wt - x - 5, yo - y - 5);

        efs_list_->column_char('\t');
        efs_list_->column_widths(cw);
        efs_list_->callback((Fl_Callback *)select_item_cb, 0);
    }

    /* right: buttons */
    x += wt + 5;
    wb = 32;
    hb = 32;
    {
        Fl_Button *b = new Fl_Button(x + wb, y, wb, hb, "@2<-");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)move_item_cb, (void *)0);
        b->tooltip("Move up");
        button_up_ = b;
    }
    {
        Fl_Button *b = new Fl_Button(x, y + hb, wb, hb, "@+");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)inject_cb, 0);
        b->tooltip("Add file");
        button_add_ = b;
    }
    {
        Fl_Button *b = new Fl_Button(x + wb, y + hb, wb, hb, "A-Z");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)sort_items_cb, 0);
        b->tooltip("Sort list");
        button_sort_ = b;
    }
    {
        Fl_Button *b = new Fl_Button(x + wb * 2, y + hb, wb, hb, "@line");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)delete_cb, 0);
        b->tooltip("Delete");
        button_del_ = b;
    }
    {
        Fl_Button *b = new Fl_Button(x, y + hb * 2, wb, hb, "Ren");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)rename_cb, 0);
        b->tooltip("Rename");
        button_ren_ = b;
    }
    {
        Fl_Button *b = new Fl_Button(x + wb, y + hb * 2, wb, hb, "@2->");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)move_item_cb, (void *)2);
        b->tooltip("Move down");
        button_down_ = b;
    }
    {
        Fl_Button *b = new Fl_Button(x + wb * 2, y + hb * 2, wb, hb, "@circle");
        b->type(FL_NORMAL_BUTTON);
        b->callback((Fl_Callback *)extract_cb, 0);
        b->tooltip("Extract item");
        button_ext_ = b;
    }

    /* right/bottom: info */
    wb *= 3;
    wb += 35;
    x -= 15;
    y += hb * 3 + 20;
    hb = 45;
    {
        carteapiname_ = new Fl_Box(FL_NO_BOX, x, y, wb, hb, "...");
    }

    y += hb + 10;
    hb = 140;
    {
        cartspace_ = new Fl_Box(FL_NO_BOX, x, y, wb, hb, "...");
    }

    update_efs_list(0);
    update_fws();
}

ndef_ui_window::~ndef_ui_window()
{
}

void ndef_ui_window::add_to_log(const char *txt)
{
    int i = 0;

    for (i = 0; (i < 1024) && (txt[i] != '\0'); ++i) {
        if (txt[i] == '\n') {
            log_lines_++;
        }
    }

    log_lines_++;

    log_disp_->insert(txt);
    log_disp_->insert("\n");

    if (log_lines_ > 3) {
        log_disp_->scroll(log_lines_ - 3, 0);
    }
}

int ndef_ui_window::save_log(const char *filename)
{
    return log_buf_->savefile(filename);
}

void ndef_ui_window::update_filename(char *fname)
{
    cartfilename_->label((*fname != '\0') ? fl_filename_name(fname) : "(no file)");
}

void ndef_ui_window::update_efname(void)
{
    cartefname_->label(efname_get());
}

int ndef_ui_window::get_index(void)
{
    return efs_list_->value() - UI_EFS_LIST_OFFSET;
}

void ndef_ui_window::update_efs_list(int selected = 0)
{
    efs_list_->clear();
    efs_list_->add("#\tH\tA\tName\tType\tBank\tOffs\tSize");

    for (int i = 0; i < main_flash_efs_num; ++i) {
        char buf[256];
        efs_entry_t *e = &main_flash_efs[i];

        sprintf(buf, "%i\t%c\t%c\t%s\t%s\t%02x\t%04x\t%06x",
                i,
                (e->flags & EFS_FLAG_HIDDEN) ? '*' : ' ',
                (e->flags & EFS_FLAG_ALIGN64K) ? 'X' : ' ',
                e->menuname,
                efs_entry_type_string(e->type),
                e->bank,
                e->offset,
                e->size
                );

        efs_list_->add(buf);
    }

    efs_list_->select(selected);
}

void ndef_ui_window::update_fws(void)
{
    bool ocean = (main_flash_state & MAIN_STATE_HAVE_OCEAN);

    cartmode_->label(ocean ? "Ocean mode" : "Normal mode");
    cartmode_->redraw();

    bool have_eapi = main_flash_state & MAIN_STATE_HAVE_EAPI;

    carteapiname_->label(have_eapi ? eapi_name_get() : "(no EAPI)");
    carteapiname_->redraw();
}

static Fl_Color colorbank(WORD size, int lh, int bank, bool selected)
{
    if (((main_flash_state & MAIN_STATE_HAVE_OCEAN) && lh == 1 && bank < 2) ||
        (!(main_flash_state & MAIN_STATE_HAVE_OCEAN) && bank < 1)) {
        return FL_MAGENTA;
    } else if (size == 0x2000) {
        return selected ? FL_BLUE : FL_GREEN;
    } else if (size > 0) {
        return selected ? FL_DARK_BLUE : FL_YELLOW;
    }

    return FL_BACKGROUND_COLOR;
}

void ndef_ui_window::update_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k)
{
    sprintf(spacetxt_, "Free space:\nTotal = %i kB\nTop = %i kB\nBanks:\n16k = %i\nL = %i\nH = %i",
                         top / (1 << 10), total / (1 << 10), rom_16k, rom_8k, rom_u8k);
    cartspace_->label(spacetxt_);
    cartspace_->redraw();
}

void ndef_ui_window::update_bank_used(void)
{
    int lh, bank;
    WORD size;
    const int i = get_index();
    const bool any_selected = (i >= 0) && (i < main_flash_efs_num);
    bool selected = false;

    for (lh = 0; lh < 2; ++lh) {
        for (bank = 0; bank < EASYFLASH_N_BANKS; ++bank) {
            if (any_selected && ((size = main_flash_efs[i].bank_used[lh][bank]) > 0)) {
                selected = true;
            } else {
                selected = false;
                size = main_flash_space.bank_used[lh][bank];
            }
            bank_used_[lh][bank]->color(colorbank(size, lh, bank, selected));
            bank_used_[lh][bank]->redraw_label();
        }
    }
}

/* ------------------------------------------------------------------------- */

static void ui_abort(const char *, ...)
{
    exit(0);
}

static void ndefgui_shutdown(void)
{
    fprintf(stdout, "ndefgui shutting down.\n");

    delete ui_win;
    ui_win = NULL;

    main_flash_shutdown();
}

/* ------------------------------------------------------------------------- */

extern "C" {

void ui_message(const char *msg)
{
    if (ui_win != NULL) {
        ui_win->add_to_log(msg);
    } else {
        fputs(msg, stdout);
        fputc('\n', stdout);
    }
}

void ui_warning(const char *msg)
{
    if (ui_win != NULL) {
        ui_win->add_to_log(msg);
    } else {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }
}

void ui_error(const char *msg)
{
    if (ui_win != NULL) {
        ui_win->add_to_log(msg);
    } else {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }
}

void ui_display_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k)
{
    if (ui_win != NULL) {
        ui_win->update_space(top, total, rom_16k, rom_8k, rom_u8k);
    }
}

void ui_display_bank_used(bank_used_t bank_used, int show_banknums)
{
    if (ui_win != NULL) {
        ui_win->update_bank_used();
    }
}

void usage(int show_helptext)
{
    util_message("Usage: ndefgui [EFCRT]");
    usage_ef_options();
}

} /* extern "C" */

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    int i = 0;

    fprintf(stdout, "ndefgui v" VERSION "\n");

#if defined(__APPLE__) || defined(__MACH_O__)
    /* launched by Finder? */
    if (argc >= 2 && strncmp(argv[1], "-psn_", 5) == 0) {
        argc = 1;
    }
#endif

    if (parse_options(&argc, argv, &i) < 0) {
        return -1;
    }

    atexit(ndefgui_shutdown);

    main_flash_init();

    Fl::set_abort(ui_abort);

    ui_win = new ndef_ui_window(UI_WIN_W, UI_WIN_H, "ndefgui v" VERSION);

    Fl::visual(FL_DOUBLE|FL_INDEX);
    ui_win->show();

    ui_message("ndefgui v" VERSION " - Not Done EasyFlash GUI\nRead the README file(s) for help.\n"
              "Tip: rename the first line (# H A ...) to set the EF-Name."
              );

    if (argc > 0) {
        ui_load_ef_crt_file(argv[i]);
    } else {
        /* make sure to display proper free space */
        main_flash_display_space();
    }

    return Fl::run();
}
