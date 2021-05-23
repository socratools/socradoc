dnl ####################################################################
dnl
dnl MIT License
dnl
dnl Copyright (c) 2021 Hans Ulrich Niedermann
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a copy
dnl of this software and associated documentation files (the "Software"), to deal
dnl in the Software without restriction, including without limitation the rights
dnl to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
dnl copies of the Software, and to permit persons to whom the Software is
dnl furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice shall be included in all
dnl copies or substantial portions of the Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
dnl AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
dnl OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl SOFTWARE.
dnl
dnl ####################################################################
dnl
dnl NDIM_PATH_PROG([git])
dnl
dnl Expands into
dnl
dnl AC_ARG_VAR([GIT], [path to git utility (default: no)])
dnl AC_PATH_PROG([GIT], [git], [no])
dnl AM_CONDITIONAL([HAVE_GIT], [test "x$GIT" != xno])
dnl
dnl Note that unlike the second argument of AC_PATH_PROG, this macro
dnl does *not* support any parameters to the utility.
dnl
dnl ####################################################################
m4_pattern_forbid([NDIM_PATH_PROG])dnl
dnl
AC_DEFUN([NDIM_PATH_PROG], [dnl
AC_ARG_VAR(m4_toupper([$1]), [path to ][$1][ utility (default: no)])
AC_PATH_PROG(m4_toupper([$1]), [$1], [no])
AM_CONDITIONAL([HAVE_]m4_toupper([$1]), [test "x$]m4_toupper([$1])[" != xno])
])dnl
