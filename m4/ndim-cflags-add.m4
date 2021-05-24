dnl ####################################################################
dnl NDIM_CFLAGS_ADD([VAR], [flag])
dnl   If a test program can be compiled with the CFLAGS from <VAR> plus
dnl   <flag>, append <flag> to VAR. Otherwise, keep the <VAR> value.
dnl
dnl Example:
dnl   NDIM_CFLAGS_ADD([PEDANTIC_CFLAGS], [-Wall])
dnl   NDIM_CFLAGS_ADD([PEDANTIC_CFLAGS], [-Wextra])
dnl   NDIM_CFLAGS_ADD([PEDANTIC_CFLAGS], [-Werror])
dnl   NDIM_CFLAGS_ADD([PEDANTIC_CFLAGS], [-std=c11])
dnl   NDIM_CFLAGS_ADD([PEDANTIC_CFLAGS], [-pedantic])
dnl
dnl ####################################################################
m4_pattern_forbid([^NDIM_CFLAGS_ADD])dnl
dnl ####################################################################
AC_DEFUN([NDIM_CFLAGS_ADD], [dnl
  ndim_cflags_add_saved_CFLAGS="$CFLAGS"
  set dummy $[$1]; ndim_first_word=$[2]
  AS_IF([test "x$ndim_first_word" = x], [dnl
    AC_MSG_CHECKING([whether compiling with empty CFLAGS works])
    CFLAGS=""
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>

int testfunc(int argc, char *argv[]);
int testfunc(int argc, char *argv[])
{
  int i;
  for (i=0; i<argc; i++) {
    printf("%3d %s\n", i, argv[i]);
  }
  return 0;
}
    ]])], [dnl the empty CFLAGS work, so we can continue
      AC_MSG_RESULT([yes])
    ], [dnl the empty CFLAGS failed to compile, we cannot work with that
      AC_MSG_RESULT([no])
      AC_MSG_FAILURE([could not compile with empty CFLAGS])
    ])
  ])

  AC_MSG_CHECKING([whether to add ][$2][ to ][$1])
  CFLAGS="$[$1] $2"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>

int testfunc(int argc, char *argv[]);
int testfunc(int argc, char *argv[])
{
  int i;
  for (i=0; i<argc; i++) {
    printf("%3d %s\n", i, argv[i]);
  }
  return 0;
}
  ]])], [dnl the new CFLAGS work, keep the new value
    AC_MSG_RESULT([yes])
    AS_VAR_APPEND([$1], [" $2"])
  ], [dnl the new CFLAGS failed, restore CFLAGS to last working value
    AC_MSG_RESULT([no])
  ])
  AC_SUBST([$1])
  CFLAGS="$ndim_cflags_add_saved_CFLAGS"
  AS_UNSET([ndim_cflags_add_saved_CFLAGS])
])dnl
