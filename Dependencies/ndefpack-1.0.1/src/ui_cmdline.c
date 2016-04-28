/*
 * ui_cmdline.c - ndeft UI implementation
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cart.h"
#include "config.h"
#include "easyfs.h"
#include "flash.h"
#include "lst.h"
#include "options.h"
#include "types.h"
#include "ui.h"
#include "util.h"

/* ------------------------------------------------------------------------- */

static int build_lst_efcrt(int argc, char** argv)
{
    return lst_load(argv[0]);
}

static int list_efcrt(int argc, char** argv)
{
    return main_flash_dump_all(0, NULL);
}

static int dump_efcrt(int argc, char** argv)
{
    return main_flash_dump_all(1, (argc == 1) ? argv[0] : NULL);
}

static int ext_efcrt(int argc, char** argv)
{
    return main_flash_ext_entry(parse_initem(argv[0]), argv[1]);
}

static int add_entry(int argc, char** argv)
{
    int rc = 0;
    int i = 0;

    while (argc--) {
        rc = lst_parse_entry(argv[i++]);
        if (rc < 0) {
            return -1;
        }
    }
    return main_flash_place_entries();
}

static int del_entry(int argc, char** argv)
{
    return main_flash_del_entry(parse_initem(argv[0]));
}

static int rename_entry(int argc, char** argv)
{
    return main_flash_entry_name(parse_initem(argv[0]), argv[1]);
}

static int swap_entry(int argc, char** argv)
{
    return main_flash_entry_swap(parse_initem(argv[0]), parse_initem(argv[1]));
}

static int sort_efcrt(int argc, char** argv)
{
    return main_flash_entry_sort();
}

static int touch_efcrt(int argc, char** argv)
{
    return 0;
}

static int hide_entry(int argc, char** argv)
{
    return main_flash_entry_hide(parse_initem(argv[0]), 1);
}

static int unhide_entry(int argc, char** argv)
{
    return main_flash_entry_hide(parse_initem(argv[0]), 0);
}

static int align_entry(int argc, char** argv)
{
    return main_flash_entry_a64k(parse_initem(argv[0]), 1);
}

static int unalign_entry(int argc, char** argv)
{
    return main_flash_entry_a64k(parse_initem(argv[0]), 0);
}

static int dummy_handle(int argc, char **argv)
{
    int i;

    fprintf(stderr, "got %i params: ", argc);

    for (i = 0; i < argc; ++i) {
        fprintf(stderr, "%s ", argv[i]);
    }

    fprintf(stderr, "\n");

    return 0;
}

#define CMD_NEED_LOAD_EFCRT (1 << 0)
#define CMD_NEED_SAVE_EFCRT (1 << 1)

#define CMD_LIST_SEPARATOR(x) { "-", "", x, 0, 0, 0, dummy_handle }

typedef struct tool_commands_s {
    const char *name;
    const char *paramtext;
    const char *helptext;
    int min_argc;
    int max_argc;
    int need_inits;
    int (*handle)(int argc, char **argv);
} tool_commands_t;

static const tool_commands_t tool_commands[] = {
  CMD_LIST_SEPARATOR("Creating new EasyFlash .crt files"),

  { "make", "LST",
    "Makes a new EasyFlash .crt based on the given LST file.",
    1, 1, CMD_NEED_SAVE_EFCRT,
    build_lst_efcrt
  },

  { "build", "[INITEM]*",
    "Builds a new EasyFlash .crt from the given INITEMs.",
    1, 264, CMD_NEED_SAVE_EFCRT,
    add_entry
  },

  CMD_LIST_SEPARATOR("Operations on existing .crt files"),

  { "list", "INEFCRT",
    "List the EasyFlash .crt contents.",
    1, 1, CMD_NEED_LOAD_EFCRT,
    list_efcrt
  },
  { "dump", "INEFCRT [PREFIX]",
    "Dumps the EasyFlash .crt contents.\n"
    "Outputs PRG, CRT files and a LST file.\n"
    "Filenames start with PREFIX if given (also in the LST).",
    1, 2, CMD_NEED_LOAD_EFCRT,
    dump_efcrt
  },
  { "ext", "INEFCRT ID OUTFILE",
    "Dumps entry ID from the EasyFlash .crt.",
    3, 3, CMD_NEED_LOAD_EFCRT,
    ext_efcrt
  },
  { "add", "EFCRT [INITEM]*",
    "Add INITEMs to the EasyFlash .crt.",
    1, 264, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    add_entry
  },
  { "del", "EFCRT ID",
    "Delete the entry ID from the EasyFlash .crt.",
    2, 2, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    del_entry
  },
  { "ren", "EFCRT ID NAME",
    "Renames the entry ID in the EasyFlash .crt to NAME.",
    3, 3, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    rename_entry
  },
  { "hide", "EFCRT ID",
    "Hides the entry ID in the EasyFlash .crt.",
    2, 2, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    hide_entry
  },
  { "unhide", "EFCRT ID",
    "Unhides the entry ID in the EasyFlash .crt.",
    2, 2, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    unhide_entry
  },
  { "align", "EFCRT ID",
    "Aligns the entry ID in the EasyFlash .crt to 64k.\n"
    "Only valid for xbank entries.",
    2, 2, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    align_entry
  },
  { "unalign", "EFCRT ID",
    "Unaligns the entry ID in the EasyFlash .crt from 64k.\n"
    "Only valid for xbank entries.",
    2, 2, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    unalign_entry
  },
  { "swap", "EFCRT ID ID",
    "Swap the items with the given IDs.",
    3, 3, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    swap_entry
  },
  { "sort", "EFCRT",
    "Sorts the EasyFlash .crt menu.",
    1, 1, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    sort_efcrt
  },
  { "touch", "EFCRT",
    "Loads and saves the EasyFlash .crt.",
    1, 1, CMD_NEED_LOAD_EFCRT | CMD_NEED_SAVE_EFCRT,
    touch_efcrt
  },

  { NULL, NULL, NULL, 0, 0, 0, dummy_handle }
};

/* ------------------------------------------------------------------------- */

typedef struct tool_args_s {
    const char *name;
    const char *helptext;
} tool_args_t;

static const tool_args_t tool_args[] = {
  { "EFCRT",
    "EasyFlash .crt file. (CRT ID 32)"
  },
  { "LST",
    "A text file consisting of ITEMs. (see -h for more info)"
  },
  { "ITEM",
    "Input item description. Format:\n"
    "  FILE[" LST_SEP_CHAR "NAME[" LST_SEP_CHAR "TYPE]]\n"
    "    FILE - CRT or PRG file:\n"
    "      CRT\n"
    "        A cartridge .crt file.\n"
    "        The following cart types are supported:\n"
    "          * generic (CRT ID 0; 8k, 16k, Ultimax)\n"
    "          * Ocean (CRT ID 5)\n"
    "          * EasyFlash xbank (CRT ID 33)\n"
    "      PRG\n"
    "        Normal .prg file.\n"
    "    NAME - name on EasyFS, shown by loader (cleaned up FILE if omitted)\n"
    "    TYPE - type of entry; one of the following:\n"
    "      t - normal (default if omitted)\n"
    "      h - hidden, not shown by loader\n"
    "      a - force 64k alignment (only valid for xbank carts)\n"
    "      e - EAPI; file is 2+768 B, load address $C000 and \"eapi\" signature\n"
    "      o - Ocean boot code; file is <= 2+1256 B, load address >= $FB18\n"
    "      O - Ocean loader; file is 2+8192 B, load address = $A000\n"
    "      n - normal boot code; file is <= 2+1256 B, load address >= $FB18\n"
    "      N - normal loader; file is 2+8192 B, load address = $8000"
  },
  { "PREFIX",
    "A prefix to add to the output files. Defaults to \"ndefdump\" is not given."
  },
  { "ID",
    "Either the index number or the name of an entry."
  },

  { NULL, NULL }
};

/* ------------------------------------------------------------------------- */

void usage(int show_helptext)
{
    int i;

    util_message("Usage: ndeft [OPTION]* COMMAND ARGS\n\n"
                 "Available COMMANDs:");

    i = 0;
    while (tool_commands[i].name != NULL) {
        if (tool_commands[i].name[0] == '-') {
            if (show_helptext) {
                util_message("\n    %s\n", tool_commands[i].helptext);
            }
            ++i;
            continue;
        }

        util_message("  %s %s",
                    tool_commands[i].name,
                    tool_commands[i].paramtext ? tool_commands[i].paramtext : ""
                  );
        if (show_helptext) {
            util_message("%s\n", tool_commands[i].helptext);
        }
        ++i;
    }

    if (!show_helptext) {
        util_message("\nRun without ARGS for more help on the COMMAND");
    } else {
        util_message("Notes on ARGS:\n");
        i = 0;
        while (tool_args[i].name != NULL) {
            util_message(
                        "  %s\n%s\n",
                        tool_args[i].name,
                        tool_args[i].helptext
                        );
            ++i;
        }
    }

    usage_ef_options();
}

static void usage_cmd(int i)
{
    int j = 0;
    int firstarg = 1;

    util_message("ndeft %s %s\n\n%s",
            tool_commands[i].name,
            tool_commands[i].paramtext,
            tool_commands[i].helptext
           );

    while (tool_args[j].name != NULL) {
        if (strstr(tool_commands[i].paramtext, tool_args[j].name) != NULL) {
            if (firstarg) {
                util_message("\nNotes on ARGS:\n");
                firstarg = 0;
            }

            util_message(
                        "  %s\n%s\n",
                        tool_args[j].name,
                        tool_args[j].helptext
                        );
        }
        ++j;
    }
}

/* ------------------------------------------------------------------------- */

static int find_command(const char *cmdname)
{
    int i = 0;

    if (*cmdname == '-') {
        return -1;
    }

    while (tool_commands[i].name != NULL) {
        if (strcmp(tool_commands[i].name, cmdname) == 0) {
            break;
        }
        ++i;
    }

    if (tool_commands[i].name != NULL) {
        return i;
    } else {
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

void ui_message(const char *msg)
{
    if (verbosity >= 2) {
        fputs(msg, stdout);
        fputc('\n', stdout);
    }
}

void ui_warning(const char *msg)
{
    if (verbosity > 1) {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }
}

void ui_error(const char *msg)
{
    if (verbosity > 0) {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }
}

void ui_display_space(int top, int total, int rom_16k, int rom_8k, int rom_u8k)
{
    util_message("Free space: total = $%05x, top = $%05x. Banks: 16k = %i, L = %i, H = %i.",
                  total, top, rom_16k, rom_8k, rom_u8k);
}

void ui_display_bank_used(bank_used_t bank_used, int show_banknums)
{
    int lh, bank;
    char buf[EASYFLASH_N_BANKS + 1];
    char c;
    unsigned int size;

    buf[EASYFLASH_N_BANKS] = '\0';

    if (show_banknums) {
        for (lh = 1; lh >= 0; --lh) {
            for (bank = 0; bank < EASYFLASH_N_BANKS; ++bank) {
                buf[bank] = "0123456789abcdef"[(bank >> (lh << 2)) & 0xf];
            }
            util_message("      %s", buf);
        }
    }

    for (lh = 0; lh < 2; ++lh) {
        for (bank = 0; bank < EASYFLASH_N_BANKS; ++bank) {
            size = bank_used[lh][bank];

            if (size == 0x2000) {
                c = '*';
            } else if (size == 0) {
                c = '.';
            } else {
                c = 'o';
            }

            buf[bank] = c;
        }
        util_message("ROM%c: %s", "LH"[lh], buf);
    }
}

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    int i = 0;
    int cmd_i;
    int save_i = -1;
    int rc;

    if (argc < 2) {
        usage(0);
        return 0;
    }

    if (parse_options(&argc, argv, &i) < 0) {
        return -1;
    }

    util_message("ndeft v" PACKAGE_VERSION " - Not Done EasyFlash tool");

    if (argc < 1) {
        util_error("no COMMAND given!");
        return -1;
    }

    cmd_i = find_command(argv[i]);

    if (cmd_i < 0) {
        util_error("COMMAND '%s' not recognized!", argv[i]);
        return -1;
    }

    argc--;

    if (argc == 0) {
        usage_cmd(cmd_i);
        return 0;
    }

    if (argc < tool_commands[cmd_i].min_argc) {
        util_error("too few arguments for COMMAND '%s'!", argv[i]);
        return -1;
    }

    if (argc > tool_commands[cmd_i].max_argc) {
        util_error("too many arguments for COMMAND '%s'!", argv[i]);
        return -1;
    }

    ++i;

    main_flash_init();

    if (tool_commands[cmd_i].need_inits & CMD_NEED_LOAD_EFCRT) {
        if (main_flash_load(argv[i]) < 0) {
            main_flash_shutdown();
            return -1;
        }
        save_i = i;
    }

    if (tool_commands[cmd_i].need_inits & CMD_NEED_SAVE_EFCRT) {
        if (outefcrt_name == NULL) {
            /* no OUTEFCRT given, try to use INEFCRT for it */
            if (tool_commands[cmd_i].need_inits & CMD_NEED_LOAD_EFCRT) {
                outefcrt_name = argv[save_i];
            } else {
                /* optional INEFCRT; use it as output if given */
                if (argc > tool_commands[cmd_i].min_argc) {
                    outefcrt_name = argv[i];
                } else {
                    util_error("No output filename! Supply one with '-o'.");
                    return -1;
                }
            }
        }
    }

    if (tool_commands[cmd_i].need_inits & CMD_NEED_LOAD_EFCRT) {
        ++i;
        argc--;
    }

    rc = tool_commands[cmd_i].handle(argc, &(argv[i]));

    if ((rc >= 0) && (tool_commands[cmd_i].need_inits & CMD_NEED_SAVE_EFCRT)) {
        rc = main_flash_save(outefcrt_name);
    }

    util_message("All done%s Shutting down...", (rc < 0) ? ", errors encountered!" : ".");

    main_flash_shutdown();

    return rc;
}
