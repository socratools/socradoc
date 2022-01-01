#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
########################################################################

AC_PREREQ([2.69])
AC_INIT([Documentation for the Soundcraft Notepad series],
        [m4_esyscmd([./build-helpers/package-version . version-stamp])],
        [https://github.com/socratools/socradoc/issues],
        [socradoc],
        [https://github.com/socratools/socradoc])
AC_CONFIG_AUX_DIR([auto-aux])
AC_CONFIG_MACRO_DIR([auto-m4])
AC_CONFIG_SRCDIR([src/scnp-cli-main.c])
AC_CONFIG_HEADERS([include/auto-config.h])
AM_INIT_AUTOMAKE([
1.16
-Wall
-Werror
foreign
dist-xz
no-dist-gzip
filename-length-max=99
no-define
std-options
subdir-objects
])

########################################################################

AC_CANONICAL_HOST


########################################################################
# Checks for programs.
########################################################################

AC_PROG_SED
AC_PROG_CC
AC_PROG_INSTALL

########################################################################

m4_pattern_forbid([^NDIM_PATH_PROG])dnl
NDIM_PATH_PROG([git])
dnl NDIM_PATH_PROG([lsusb])
dnl NDIM_PATH_PROG([wc])


########################################################################
# Checks for libraries and other packages
########################################################################


########################################################################
# Look for libusb-1.0

m4_pattern_forbid([^PKG_CHECK_MODULES])dnl
PKG_CHECK_MODULES([LIBUSB10], [libusb-1.0])


########################################################################
# Figure out where to install the bash-completion file

AC_MSG_CHECKING([whether installing somewhere inside \$HOME])
AS_CASE([$prefix],
        [$HOME/*],
        [is_home_installation=yes],
        [is_home_installation=no])
AC_MSG_RESULT([$is_home_installation])

AC_MSG_CHECKING([per-user BASH_COMPLETION_USER_DIR])
AS_VAR_IF([BASH_COMPLETION_USER_DIR], [], [dnl
  AS_VAR_IF([XDG_DATA_HOME], [], [dnl
    BASH_COMPLETION_USER_DIR="$HOME/.local/share/bash-completion"
  ], [
    BASH_COMPLETION_USER_DIR="$XDG_DATA_HOME/bash-completion"
  ])
])

AC_MSG_RESULT([$BASH_COMPLETION_USER_DIR])
user_bashcompletiondir="$BASH_COMPLETION_USER_DIR/completions"
AC_MSG_CHECKING([for per-user bash completion install dir])
AC_MSG_RESULT([$user_bashcompletiondir])

m4_pattern_forbid([^PKG_CHECK_VAR])dnl
AC_MSG_CHECKING([system bash completion directory])
PKG_CHECK_VAR([system_bashcompletiondir],
              [bash-completion],
              [completionsdir],
              [AC_MSG_RESULT([default $system_bashcompletiondir])],
              [system_bashcompletiondir="${sysconfdir}/bash_completion.d"
               AC_MSG_RESULT([fallback $system_bashcompletiondir])])

AC_MSG_CHECKING([where to install bash completion files])
AC_ARG_VAR([bashcompletiondir], [where to install bash completion files])
AS_VAR_IF([bashcompletiondir], [], [dnl
  AS_VAR_IF([is_home_installation], [yes],
            [bashcompletiondir="$user_bashcompletiondir"],
            [bashcompletiondir="$system_bashcompletiondir"])
])
AC_MSG_RESULT([$bashcompletiondir])


########################################################################
# Checks for header files.
########################################################################

dnl We need windows.h on Windows for the Sleep(milliseconds) function
AC_CHECK_HEADER([windows.h],
                [AC_DEFINE([HAVE_WINDOWS_H], [1],
                           [Define to 1 if you have <windows.h>])])


########################################################################
# Checks for typedefs, structures, and compiler characteristics.
########################################################################

m4_pattern_forbid([^NDIM_CFLAGS_ADD])
NDIM_CFLAGS_ADD([PEDANTIC_C11_CFLAGS], [-Werror])
NDIM_CFLAGS_ADD([PEDANTIC_C11_CFLAGS], [-Wall])
NDIM_CFLAGS_ADD([PEDANTIC_C11_CFLAGS], [-Wextra])
NDIM_CFLAGS_ADD([PEDANTIC_C11_CFLAGS], [-Weverything -Wno-unused-macros -Wno-padded])
NDIM_CFLAGS_ADD([PEDANTIC_C11_CFLAGS], [-std=c11])
NDIM_CFLAGS_ADD([PEDANTIC_C11_CFLAGS], [-pedantic])

dnl As we cannot remove the #define lines from the conftest.c
dnl AC_LANG_SOURCE generates, and as we cannot know and thus use all
dnl the #define lines which Autoconf generates, we need to add
dnl -Wno-unused-macros for the automatic checks to work.
dnl
dnl However, for the eventual use of the PEDANTIC_C11_CFLAGS, we can
dnl make use of the warnings for unused macros, so let's filter out
dnl the -Wno-unused-macros flag from PEDANTIC_C11_CFLAGS.

ndim_new=""
for ndim_tmp in ${PEDANTIC_C11_CFLAGS}
do
  AS_VAR_IF([ndim_tmp], [-Wno-unused-macros], [], [dnl
    AS_VAR_APPEND([ndim_new], [" $ndim_tmp"])
  ])
done
PEDANTIC_C11_CFLAGS="$ndim_new"


########################################################################
# Checks for library functions.
########################################################################

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="-D_GNU_SOURCE"
saved_LIBS="$LIBS"
LIBS="-lm"
AC_MSG_CHECKING([for exp10 function])
AC_LINK_IFELSE([dnl
AC_LANG_PROGRAM([[
#include <math.h>
#include <stdio.h>
]], [[
  double x;
  for (x=-10.0; x<=10.0; x += 0.1) {
    const double d = exp10(x);
    printf("%f\n", d);
  }
]])
], [dnl
AC_MSG_RESULT([yes])
AM_CPPFLAGS="$AM_CPPFLAGS -D_GNU_SOURCE -DHAVE_EXP10_FUNCTION"
], [dnl
AC_MSG_RESULT([no])
])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


AC_SUBST([AM_CPPFLAGS])


########################################################################
# Conclude the Autoconf action
########################################################################

AC_CONFIG_FILES([GNUmakefile])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT


########################################################################
# Human readable (maybe even user readable!) summary
########################################################################

cat<<EOF

Configuration:
  Package: $PACKAGE_TARNAME ($PACKAGE_NAME)
  Version: $PACKAGE_VERSION

  srcdir: $srcdir

  CC:     $CC
  prefix: $prefix
  bindir: $bindir

You may run "make" and "make install" now.
EOF

########################################################################
