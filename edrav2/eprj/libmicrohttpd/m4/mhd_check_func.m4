# SYNOPSIS
#
#   MHD_CHECK_FUNC([FUNCTION_NAME],
#                  [INCLUDES=AC_INCLUDES_DEFAULT], [CHECK_CODE],
#                  [ACTION-IF-AVAILABLE], [ACTION-IF-NOT-AVAILABLE],
#                  [ADDITIONAL_LIBS])
#
# DESCRIPTION
#
#   This macro checks for presence of specific function by including
#   specified headers and compiling and linking CHECK_CODE.
#   This check both declaration and presence in library.
#   Unlike AC_CHECK_FUNCS macro, this macro do not produce false
#   negative result if function is declared with specific calling
#   conventions like __stdcall' or attribute like
#   __attribute__((__dllimport__)) and linker failed to build test
#   program if library contains function with calling conventions
#   different from declared. 
#   By using definition from provided headers, this macro ensures that
#   correct calling convention is used for detection.
#
#   Example usage:
#
#     MHD_CHECK_FUNC([memmem],
#                    [[#include <string.h>]],
#                    [const void *ptr = memmem("aa", 2, "a", 1); (void)ptr;],
#                    [var_use_memmem='yes'], [var_use_memmem='no'])
#
#   Defined cache variable used in check so if any test will not work
#   correctly on some platform, user may simply fix it by giving cache
#   variable in configure parameters, for example:
#
#     ./configure mhd_cv_func_memmem_have=no
#
#   This simplify building from source on exotic platforms as patching
#   of configure.ac is not required to change results of tests.
#
# LICENSE
#
#   Copyright (c) 2019 Karlson2k (Evgeny Grin) <k2k@narod.ru>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([MHD_CHECK_FUNC],[dnl
  AC_PREREQ([2.64])dnl for AS_VAR_IF, m4_ifblank, m4_ifnblank
  m4_ifblank(m4_translit([$1],[()],[  ]), [m4_fatal([First macro argument must not be empty])])dnl
  m4_ifblank([$3], [m4_fatal([Third macro argument must not be empty])])dnl
  m4_bmatch(m4_normalize([$1]), [\s],dnl
            [m4_fatal([First macro argument must not contain whitespaces])])dnl
  m4_if(m4_index([$3], m4_normalize(m4_translit([$1],[()],[  ]))), [-1], dnl
        [m4_fatal([CHECK_CODE parameter (third macro argument) do not contain ']m4_normalize([$1])[' token])])dnl
  AS_VAR_PUSHDEF([cv_Var], [mhd_cv_func_]m4_bpatsubst(m4_normalize(m4_translit([$1],[()],[  ])),[[^a-zA-Z0-9]],[_]))dnl
  dnl
  AC_CACHE_CHECK([for function $1], [cv_Var],
    [dnl
      m4_ifnblank([$6],[dnl
        mhd_check_func_SAVE_LIBS="$LIBS"
        LIBS="$LIBS m4_normalize([$6])"
      ])dnl
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([m4_default_nblank([$2],[AC_INCLUDES_DEFAULT])], [$3]) ],
        [AS_VAR_SET([cv_Var],["yes"])], [AS_VAR_SET([cv_Var],["no"])] ) 
      m4_ifnblank([$6],[dnl
        LIBS="${mhd_check_func_SAVE_LIBS}"
        AS_UNSET([mhd_check_func_SAVE_LIBS])
      ])dnl
    ])
  AS_VAR_IF([cv_Var], ["yes"],
            [AC_DEFINE([[HAVE_]]m4_bpatsubst(m4_toupper(m4_normalize(m4_translit([$1],[()],[  ]))),[[^A-Z0-9]],[_]),
                       [1], [Define to 1 if you have the `]m4_normalize(m4_translit([$1],[()],[  ]))[' function.])
            m4_n([$4])dnl
            ], [$5])
  AS_VAR_POPDEF([cv_Var])dnl
])

