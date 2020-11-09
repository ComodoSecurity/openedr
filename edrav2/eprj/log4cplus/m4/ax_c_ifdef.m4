# ===========================================================================
#        http://www.gnu.org/software/autoconf-archive/ax_c_ifdef.html
# ===========================================================================
#
# OBSOLETE MACRO
#
#   Deprecated in favor of the standard Autoconf macro AC_CHECK_DECL.
#
# SYNOPSIS
#
#   AX_C_IFDEF(MACRO-NAME, ACTION-IF-DEF, ACTION-IF-NOT-DEF)
#
# DESCRIPTION
#
#   Check for the definition of macro MACRO-NAME using the current
#   language's compiler.
#
# LICENSE
#
#   Copyright (c) 2008 Ludovic Courtes <ludo@chbouib.org>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 6

AU_ALIAS([_AC_C_IFDEF], [AX_C_IFDEF])
AC_DEFUN([AX_C_IFDEF],
  [AC_COMPILE_IFELSE([#ifndef $1
                      # error "Macro $1 is undefined!"
		      /* For some compilers (eg. SGI's CC), #error is not
		         enough...  */
		      please, do fail
		      #endif],
		     [$2], [$3])])
