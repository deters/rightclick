#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rightclick], [0.1.0], [lucasdeters@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(LIBX11, x11)
PKG_CHECK_MODULES(LIBXI, xi)
PKG_CHECK_MODULES(LIBXTST, xtst)

# PKG_CHECK_MODULES(LIBINPUT, libinput)

AX_PTHREAD

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT


