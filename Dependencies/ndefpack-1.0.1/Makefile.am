SUBDIRS = doc src

doc_DATA = README

if BUILD_MACOSX
bindist:
	$(SHELL) makebindist_macosx.sh $(STRIP) @PACKAGE_VERSION@ $(top_srcdir) $(EXEEXT)
endif

if BUILD_WIN32
bindist:
	$(SHELL) makebindist_win32.sh $(STRIP) @PACKAGE_VERSION@ $(top_srcdir) $(EXEEXT)
endif

EXTRA_DIST = COPYING_fw makebindist_macosx.sh makebindist_win32.sh Info.plist

