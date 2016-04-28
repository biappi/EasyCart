SUFFIXES = .prg .asm

noinst_LIBRARIES = libeasyfw.a

noinst_DATA = \
	eapi_am29f040.prg \
	easyloader_launcher_nrm.prg \
	easyloader_launcher_ocm.prg \
	easyloader_nrm.prg \
	easyloader_ocm.prg

EMBEDDED_H_FILES = $(noinst_DATA:.prg=.h)
el_top_files = $(noinst_DATA:.prg=.asm)

noinst_HEADERS = $(EMBEDDED_H_FILES)
BUILT_SOURCES = $(EMBEDDED_H_FILES)

noinst_SCRIPTS = prg2h.sh generate_font.php generate_screen.php generate_sprites.php

eapi_deps = eapi_defs.asm
el_launcher_deps = easyloader_launcher.asm
el_gfx_png_files = easyloader_font.png easyloader_sprites.png
el_gfx_bin_files = easyloader_cols.bin easyloader_screen.bin $(el_gfx_png_files:.png=.bin)
el_deps = easyloader.asm easyloader_screendefs.asm $(el_gfx_bin_files)

if FIRMWARE
BUILT_SOURCES += $(noinst_DATA) $(el_gfx_bin_files)

eapi_am29f040.prg: eapi_am29f040.asm $(eapi_deps)
easyloader_launcher_nrm.prg: easyloader_launcher_nrm.asm $(el_launcher_deps)
easyloader_launcher_ocm.prg: easyloader_launcher_ocm.asm $(el_launcher_deps)
easyloader_nrm.prg: easyloader_nrm.asm $(el_deps)
easyloader_ocm.prg: easyloader_ocm.asm $(el_deps)

easyloader_font.bin: easyloader_font.png
	$(AM_V_GEN)$(PHP) $(srcdir)/generate_font.php $< $(top_builddir)/src/fw/$@

easyloader_sprites.bin: easyloader_sprites.png
	$(AM_V_GEN)$(PHP) $(srcdir)/generate_sprites.php $< $(top_builddir)/src/fw/$@

easyloader_screen.bin easyloader_cols.bin easyloader_screendefs.asm: generate_screen.php
	$(AM_V_GEN)$(PHP) $< $(top_builddir)/src/fw/easyloader_screen.bin $(top_builddir)/src/fw/easyloader_cols.bin $(top_builddir)/src/fw/easyloader_screendefs.asm
endif

EXTRA_DIST = $(noinst_SCRIPTS) $(noinst_DATA) $(EMBEDDED_H_FILES) $(el_top_files) $(eapi_deps) $(el_deps) $(el_launcher_deps) $(el_gfx_png_files)

libeasyfw_a_SOURCES = \
	easyfw.c \
	$(EMBEDDED_H_FILES)

.prg.h:
	$(AM_V_GEN)$(SHELL) prg2h.sh $< > $(top_builddir)/src/fw/$@

.asm.prg:
	$(AM_V_GEN)$(ACME) -f cbm --cpu 6502 -o $(top_builddir)/src/fw/$@ $<
