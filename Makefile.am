########################################################################

ACLOCAL_AMFLAGS = -I auto-m4 -I m4 --install
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

########################################################################

bin_PROGRAMS =
bashcompletion_DATA =
check_PROGRAMS =
check_SCRIPTS =
BUILT_SOURCES =
CLEANFILES =
doc_DATA =
EXTRA_DIST =
man1_MANS =
noinst_DATA =
noinst_SCRIPTS =
TESTS =
XFAIL_TESTS =

########################################################################

include build-helpers/package-version.mk

include Makefile-files
include bash-completion/Makefile-files
include doc/Makefile-files
include include/Makefile-files
include m4/Makefile-files
include man/Makefile-files
include src/Makefile-files
include test/Makefile-files
include udev/Makefile-files

########################################################################

sed_commands =
sed_commands += -e 's|[@]abs_top_builddir@|$(abs_top_builddir)|g'
sed_commands += -e 's|[@]abs_top_srcdir@|$(abs_top_srcdir)|g'
sed_commands += -e 's|[@]docdir@|$(docdir)|g'
sed_commands += -e 's|[@]LSUSB@|$(LSUSB)|g'
sed_commands += -e 's|[@]WC@|$(WC)|g'

.shin.sh:
	@$(MKDIR_P) $(@D)
	$(SED) $(sed_commands) $< > $@.$$$$.new && mv -f $@.$$$$.new $@
	chmod +x $@

.1in.1:
	@$(MKDIR_P) $(@D)
	$(SED) $(sed_commands) $< > $@.$$$$.new && mv -f $@.$$$$.new $@

########################################################################
