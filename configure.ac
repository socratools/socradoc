#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
########################################################################

AC_PREREQ([2.69])
AC_INIT([CLI for Soundcraft Notepad series mixers],
        [m4_esyscmd([./build-helpers/package-version . version-stamp])],
        [BUG-REPORT-ADDRESS],
        [scnp-cli],
        [PROJECT_URL])
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
AC_PROG_SED
AC_PROG_CC
AC_PROG_INSTALL

########################################################################

m4_pattern_forbid([^NDIM_PATH_PROG])dnl
NDIM_PATH_PROG([git])
NDIM_PATH_PROG([markdown])

########################################################################

AC_LANG_WERROR

########################################################################

# Checks for libraries.
m4_pattern_forbid([^PKG_CHECK_MODULES])dnl
PKG_CHECK_MODULES([LIBUSB10], [libusb-1.0])

########################################################################

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

########################################################################

# TODO: These should be checked to work before using them.
AM_CFLAGS="$AM_CFLAGS -Wall"
AM_CFLAGS="$AM_CFLAGS -Wextra"
AM_CFLAGS="$AM_CFLAGS -Werror"
AM_CFLAGS="$AM_CFLAGS -std=c11"
AM_CFLAGS="$AM_CFLAGS -pedantic"
AC_SUBST([AM_CFLAGS])

########################################################################

AC_CONFIG_FILES([GNUmakefile])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

########################################################################

cat<<EOF

Configuration:
  Package: $PACKAGE_TARNAME
  Version: $PACKAGE_VERSION

  srcdir: $srcdir

  CC:     $CC
  prefix: $prefix
  bindir: $bindir

You may run "make" and "make install" now.
EOF

########################################################################
