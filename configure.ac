#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(ulppk, 2.0.1, ulppk@garveybz.com, ulppk, http://www.garveybz.com/swdev/ulppk-home.php)
#AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
#AM_INIT_AUTOMAKE(ulppk, 2.0.0)
AC_CONFIG_SRCDIR([src/ulppk-doc.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])

# Options
AC_DISABLE_STATIC

#Variables

AC_SUBST(LIBVERSION, [2:0:0])
# Checks for programs.
AC_PROG_CC
AC_PROG_YACC
AC_PROG_LIBTOOL

AM_PROG_LEX

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [dlerror])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset strdup strerror strtol])

AC_CONFIG_FILES([Makefile
         ulppk/Makefile
		 src/Makefile])
AC_OUTPUT
