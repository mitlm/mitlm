dnl Process this file with autoconf to produce a configure script.

dnl AC_INIT(package, version, bug-report-address)
AC_INIT([MIT Language Modeling Toolkit], 0.4.1, [giuliopaci@gmail.com], mitlm)
dnl This file (configure.ac) revision.
AC_REVISION([$Revision$])
dnl The base directory (srcdir) must contain configure.ac.
AC_CONFIG_SRCDIR(configure.ac)
dnl Use ./ to store scripts and files used by configure.
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_F77

AC_F77_LIBRARY_LDFLAGS
AC_F77_DUMMY_MAIN
AC_F77_WRAPPERS

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_CHECK_HEADERS(string.h math.h)
AC_HEADER_STDC

dnl Checks for types.

dnl Checks for structures.

dnl Checks for compiler characteristics.
AC_C_INLINE
AM_PROG_CC_C_O

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_MALLOC
AC_FUNC_REALLOC


AC_DEFUN([AX_CHECK_BUILTIN], [
    AC_CACHE_CHECK([wheter we have $1], [ac_cv_have_$1],
[AC_LINK_IFELSE([AC_LANG_PROGRAM([$2], [$3])],
[ac_cv_have_$1=yes], [ac_cv_have_$1=no])])
    if test $ac_cv_have_$1 = yes; then
AC_DEFINE(AS_TR_CPP([HAVE_$1]), [1], [wheter we have $1])
    fi
])

AX_CHECK_BUILTIN([__builtin_clzl], [[unsigned long x = 1;]],
    [[int main(void) { return __builtin_clzll(x) == (sizeof(int)*8 - 1) ? 0 : 1; }
]])

AX_CHECK_BUILTIN([X86_ASM], [[
#define bool int
#define HAVE_X86_ASM 1
#include "src/util/BitOps.h"
unsigned long x = 2;
]],
    [[int main(void) { return find_last_bit_set(2)-2 ; }
]])

dnl Checks for system services.

dnl Output.
AC_CONFIG_FILES([
Makefile
])
AC_CONFIG_FILES([tests/test1.test], [chmod +x tests/test1.test])

AC_OUTPUT([])
