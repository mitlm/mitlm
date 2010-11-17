dnl Process this file with autoconf to produce a configure script.

dnl AC_INIT(package, version, bug-report-address)
AC_INIT([MIT Language Modeling Toolkit], 0.4, [giuliopaci@interfree.it], mitlm)
dnl This file (configure.ac) revision.
AC_REVISION([$Revision$])
dnl The base directory (srcdir) must contain configure.ac.
AC_CONFIG_SRCDIR(configure.ac)
dnl Use ./ to store scripts and files used by configure.
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_F77

AC_F77_LIBRARY_LDFLAGS

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
dnl Checks for system services.

dnl Output.
AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT([])
