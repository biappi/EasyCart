AC_INIT([ndefpack], [1.0.1])

AC_ARG_WITH(gui, AS_HELP_STRING([--without-gui], [do not build the GUI]))
AC_ARG_WITH(firmware, AS_HELP_STRING([--without-firmware], [do not try to build the firmware]))

AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/cart.c])
AM_SILENT_RULES

user_cflags=$CFLAGS

AC_PROG_CC
AC_PROG_CPP
AC_CHECK_TOOL(AR, ar, :)
AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_CHECK_TOOL(STRIP, strip, :)
AC_PROG_INSTALL
AC_PROG_LN_S


dnl - add some warnings

if test -z "$user_cflags" ; then

    dnl Check for the type of compiler first.
    if test x"$GCC" = "xyes" ; then

        warnings_cxx="-Wall -Wno-inline"
        warnings_c="-Wstrict-prototypes $warnings_cxx"

        dnl Set appropriate optimization options (better than the default -g -O)
        dnl if using GCC.
        dnl If the user has specified her own `CFLAGS', we do not override them.

        optimization_c="-O2"
        optimization_cxx="-fno-exceptions $optimization_c"
        dnl Use -g if available.
        if test x"$ac_cv_prog_cc_g" = "xyes" ; then
            CFLAGS="-g $optimization_c $warnings_c"
            CXXFLAGS="-g $optimization_cxx $warnings_cxx"
        else
            CFLAGS="$optimization_c $warnings_c"
            CXXFLAGS="$optimization_cxx $warnings_cxx"
        fi

        dnl Check whether the options are OK.
        AC_PROG_CC

    fi

    dnl (If not on GCC, just keep the defaults, which are very conservative).

fi


dnl - check for win32 headers

AC_CHECK_HEADERS([wtypes.h winnt.h], [is_win32=yes; AC_DEFINE(IS_WINDOWS,, [Compiling for Windows])], [is_win32=no],
[[#ifdef HAVE_WTYPES_H
#include <wtypes.h>
#endif
#ifdef HAVE_WINNT_H
#include <winnt.h>
#endif
]])
AM_CONDITIONAL(BUILD_WIN32, test x"$is_win32" = "xyes")


dnl - check for Mac OS X platform

AC_EGREP_CPP(yes,
[
#if defined(__APPLE__) || defined(__MACH_O__)
  yes
#endif
], is_macosx=yes, is_macosx=no)
AM_CONDITIONAL(BUILD_MACOSX, test x"$is_macosx" = "xyes")


dnl - check for FLTK-1.1.x GUI

UI_FLTK_LIBS=
AM_CONDITIONAL(SUPPORT_UI_FLTK, false)

dnl we need a C++ compiler
if test -z "$CXX"; then
    AC_CHECK_PROGS(cxx, $CCC c++ g++ gcc CC cxx cc++, no)
    if test x"$cxx" = "xno"; then
        AC_MSG_WARN([C++ compiler missing, GUI will not be configured])
        with_gui=no
    fi
fi

if test x"$with_gui" != "xno"; then
    old_cppflags=$CPPFLAGS

    dnl check for fltk-config
    AC_CHECK_TOOLS(fltk_config, fltk-config, no)

    dnl if no fltk-config was found, use FLTK_INCLUDES/LIBS
    if test x"$fltk_config" = "xno"; then
        AC_MSG_WARN([fltk-config not found])
        CPPFLAGS="$CPPFLAGS $FLTK_INCLUDES"
        UI_FLTK_LIBS=$FLTK_LIBS
    else
        CPPFLAGS="$CPPFLAGS `$fltk_config --cxxflags`"

        dnl do a static build for win32/macosx
        if test x"$is_win32" = "xyes" -o x"$is_macosx" = "xyes"; then
            UI_FLTK_LIBS="`$fltk_config --ldstaticflags`"
        else
            UI_FLTK_LIBS="`$fltk_config --ldflags`"
        fi
    fi

    dnl check headers
    old_libs=$LIBS
    LIBS="$LIBS $UI_FLTK_LIBS"
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_CHECK_HEADERS(FL/Fl.H, [AM_CONDITIONAL(SUPPORT_UI_FLTK, true)], CPPFLAGS=$old_cppflags)
    dnl TODO check linking
    AC_LANG_RESTORE
    LIBS=$old_libs
fi
AC_SUBST(UI_FLTK_LIBS)


dnl - check if acme and php are available

AC_CHECK_PROGS(ACME, acme, no)
AC_CHECK_PROGS(PHP, php, no)
AM_CONDITIONAL(FIRMWARE, test x"$with_firmare" != "xno" -a x"$ACME" != "xno" -a x"$PHP" != "xno")


dnl - finish up

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile src/Makefile src/fw/Makefile doc/Makefile])
AC_OUTPUT
