/*
 * options.c - cmdline options
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "flash.h"
#include "options.h"
#include "ui.h"
#include "util.h"

/* ------------------------------------------------------------------------- */

/* options.h globals */

int verbosity = 2;
int add_eapi = 1;
const char *outefcrt_name = NULL;
const char *outefname = NULL;

/* ------------------------------------------------------------------------- */

int parse_initem(const char *s)
{
    char *e = NULL;
    unsigned long int v = strtoul(s, &e, 10);

    if (*e != '\0') {
        v = main_flash_entry_find(s);
    }

    return (int)v;
}

/* ------------------------------------------------------------------------- */

static int show_all_help(int argc, char **argv)
{
    usage(1);
    return -1;
}

static int set_outefcrt_name(int argc, char **argv)
{
    outefcrt_name = argv[0];
    util_message("Set output name to '%s'.", outefcrt_name);
    return 0;
}

static int set_outefname(int argc, char **argv)
{
    outefname = argv[0];
    util_message("Set EF-Name to '%s'.", outefname);
    return 0;
}

static int dont_add_eapi(int argc, char **argv)
{
    add_eapi = 0;
    return 0;
}

static int be_more_noisy(int argc, char **argv)
{
    ++verbosity;
    return 0;
}

static int be_more_quiet(int argc, char **argv)
{
    if ((--verbosity) < 0) {
        verbosity = 0;
    }
    return 0;
}

static int dummy_handleopt(int argc, char **argv)
{
    int i;

    fprintf(stderr, "opt got %i params: ", argc);

    for (i = 0; i < argc; ++i) {
        fprintf(stderr, "%s ", argv[i]);
    }

    fprintf(stderr, "\n");

    return 0;
}

typedef struct tool_options_s {
    const char name;
    const char *paramtext;
    const char *helptext;
    int num_argc;
    int (*handle)(int argc, char **argv);
} tool_options_t;

static const tool_options_t tool_options[] = {
  { 'h', NULL,
    "Show more help.",
    0,
    show_all_help
  },
  { 'o', "OUTEFCRT",
    "Save EasyFlash .crt to file OUTEFCRT. (default = use INEFCRT)",
    1,
    set_outefcrt_name
  },
  { 'e', "NAME    ",
    "Set EF-Name to NAME.",
    1,
    set_outefname
  },
  { 'd', NULL,
    "Don't add default EAPI.",
    0,
    dont_add_eapi
  },
  { 'q', NULL,
    "Be quieter.",
    0,
    be_more_quiet
  },
  { 'Q', NULL,
    "Be noisier.",
    0,
    be_more_noisy
  },
  { '\0', NULL, NULL, 0, dummy_handleopt }
};

static int find_option(const char *optname)
{
    int i = 0;

    if (0
        || (optname[0] != '-')
        || (optname[1] == '\0')
        || (optname[2] != '\0')) {
        return -1;
    }

    while (tool_options[i].name != '\0') {
        if ((tool_options[i].name == optname[1])) {
            break;
        }
        ++i;
    }

    if (tool_options[i].name != '\0') {
        return i;
    } else {
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

int parse_options(int *argcptr, char **argv, int *i_out)
{
    int i;
    int opt_i;
    int argc = *argcptr;

    argc--;

    i = 1;

    while ((argc > 0) && ((opt_i = find_option(argv[i])) >= 0)) {
        int opt_argc = tool_options[opt_i].num_argc;

        argc--;

        if (argc < opt_argc) {
            util_error("too few arguments for OPTION '%s'!", argv[i]);
            return -1;
        }

        ++i;

        if (tool_options[opt_i].handle(argc, &(argv[i])) < 0) {
            return -1;
        }

        i += opt_argc;
        argc -= opt_argc;
    }

    *argcptr = argc;
    *i_out = i;

    return 0;
}

void usage_ef_options(void)
{
    int i;

    util_message("\nAvailable OPTIONs:");

    i = 0;
    while (tool_options[i].name != '\0') {
        util_message(
                    "  -%c %s  %s",
                    tool_options[i].name,
                    tool_options[i].paramtext ? tool_options[i].paramtext : "        ",
                    tool_options[i].helptext
                    );
        ++i;
    }
}
