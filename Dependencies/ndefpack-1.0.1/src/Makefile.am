SUBDIRS = fw

common_sources = \
	archdep.h \
	boot.c \
	boot.h \
	cart.c \
	cart.h \
	eapi.c \
	eapi.h \
	easyfs.c \
	easyfs.h \
	efname.c \
	efname.h \
	flash.c \
	flash.h \
	fw.h \
	lib.c \
	lib.h \
	loader.c \
	loader.h \
	lst.c \
	lst.h \
	options.c \
	options.h \
	petscii.c \
	petscii.h \
	place.c \
	place.h \
	prg.c \
	prg.h \
	types.h \
	ui.h \
	util.c \
	util.h

fw_lib = $(top_builddir)/src/fw/libeasyfw.a

bin_PROGRAMS = ndeft

if SUPPORT_UI_FLTK
bin_PROGRAMS += ndefgui
endif

# ndeft
ndeft_SOURCES = \
	$(common_sources) \
	ui_cmdline.c

ndeft_DEPENDENCIES = $(fw_lib)
ndeft_LDADD = $(fw_lib)


# ndefgui
ndefgui_SOURCES = \
	$(common_sources) \
	ui_fltk.cc

ndefgui_DEPENDENCIES = $(fw_lib)
ndefgui_LDADD = $(fw_lib) @UI_FLTK_LIBS@
