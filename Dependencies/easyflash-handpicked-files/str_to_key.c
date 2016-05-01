
#include "str_to_key.h"

const str_to_key_t str_to_key[] =
{
        { "<CTRL-@>",        0 },
        { "<CTRL-A>",        1 },
        { "<CTRL-B>",        2 },
        { "<CTRL-C>",        3 },
        { "<STOP>",          3 },
        { "<RUN/STOP>",      3 },
        { "<CTRL-D>",        4 },
        { "<CTRL-E>",        5 },
        { "<CTRL-2>",        5 },
        { "<WHITE>",         5 },
        { "<CTRL-F>",        6 },
        { "<CTRL-G>",        7 },
        { "<CTRL-H>",        8 },
        { "<CTRL-I>",        9 },
        { "<CTRL-J>",       10 },
        { "<CTRL-K>",       11 },
        { "<CTRL-L>",       12 },
        { "<CTRL-M>",       13 },
        { "<RETURN>",       13 },
        { "<CTRL-N>",       14 },
        { "<CTRL-O>",       15 },
        { "<CTRL-P>",       16 },
        { "<CTRL-Q>",       17 },
        { "<DOWN>",         17 },
        { "<CTRL-R>",       18 },
        { "<CTRL-9>",       18 },
        { "<REVON>",        18 },
        { "<CTRL-S>",       19 },
        { "<HOME>",         19 },
        { "<CTRL-T>",       20 },
        { "<DEL>",          20 },
        { "<CTRL-U>",       21 },
        { "<CTRL-V>",       22 },
        { "<CTRL-W>",       23 },
        { "<CTRL-X>",       24 },
        { "<CTRL-Y>",       25 },
        { "<CTRL-Z>",       26 },
        { "<CTRL-:>",       27 },
        { "<CTRL-3>",       28 },
        { "<CTRL-POUND>",   28 },
        { "<RED>",          28 },
        { "<CTRL-;>",       29 },
        { "<RIGHT>",        29 },
        { "<CTRL-6>",       30 },
        { "<CTRL-^>",       30 },
        { "<GREEN>",        30 },
        { "<CTRL-7>",       31 },
        { "<CTRL-=>",       31 },
        { "<BLUE>",         31 },
        /* fertig bis hier */

        { "<<>",            60 }, /* alternative for '<' */

        { "<>>",            62 }, /* alternative for '>' */

        { "<POUND>",        92 },

        { "^",              94 }, /* arrow up */
        { "<<=>",           95 }, /* arrow left */

        { "<PI>",          126 },

        { "<F1>",          133 },
        { "<F3>",          134 },
        { "<F5>",          135 },
        { "<F7>",          136 },
        { "<F2>",          137 },
        { "<F4>",          138 },
        { "<F6>",          139 },
        { "<F8>",          140 },
        { "<SHIFT-RETURN>",141 },

        { "<BLACK>",       144 },
        { "<CTRL-1>",      144 },
        { "<UP>",          145 },
        { "<CTRL-0>",      146 },
        { "<REVOFF>",      146 },
        { "<CLEAR>",       147 },
        { "<INS>",         148 },
        { "<BROWN>",       149 },
        { "<C=-2>",        149 },

        { "<PINK>",        150 },
        { "<C=-3>",        150 },
        { "<DGREY>",       151 },
        { "<C=-4>",        151 },
        { "<GREY>",        152 },
        { "<C=-5>",        152 },
        { "<LGREEN>",      153 },
        { "<C=-6>",        153 },
        { "<LBLUE>",       154 },
        { "<C=-7>",        154 },
        { "<LGREY>",       155 },
        { "<C=-8>",        155 },
        { "<PURPLE>",      156 },
        { "<CTRL-5>",      156 },
        { "<LEFT>",        157 },
        { "<YELLOW>",      158 },
        { "<CTRL-8>",      158 },
        { "<CYAN>",        159 },
        { "<CTRL-4>",      159 },
        { "<SHIFT-SPACE>", 160 },

        { 0,                 0 }
};

