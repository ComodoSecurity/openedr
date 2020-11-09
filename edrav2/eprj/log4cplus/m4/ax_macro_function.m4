dnl Macros that check for availability of __FUNCTION__ and
dnl __PRETTY_FUNCTIN__ macros.

AC_DEFUN([AX___FUNCTION___MACRO],
[
AH_TEMPLATE([HAVE___FUNCTION___MACRO],
  [Defined if the compiler supports __FUNCTION__ macro.])

AC_CACHE_CHECK([for __FUNCTION__ macro], [ac_cv_have___function___macro],
[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[]],
      [[
        char const * func = __FUNCTION__;
      ]]
    )],
    [ac_cv_have___function___macro=yes],
    [ac_cv_have___function___macro=no])
])
])


AC_DEFUN([AX___PRETTY_FUNCTION___MACRO],
[
AH_TEMPLATE([HAVE___PRETTY_FUNCTION___MACRO],
  [Defined if the compiler supports __PRETTY_FUNCTION__ macro.])

AC_CACHE_CHECK([for __PRETTY_FUNCTION__ macro],
[ac_cv_have___pretty_function___macro],
[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[]],
      [[
        char const * func = __PRETTY_FUNCTION__;
      ]]
    )],
    [ac_cv_have___pretty_function___macro=yes],
    [ac_cv_have___pretty_function___macro=no])
])
])


AC_DEFUN([AX___FUNC___SYMBOL],
[
AH_TEMPLATE([HAVE___FUNC___SYMBOL],
  [Defined if the compiler supports __func__ symbol.])

AC_CACHE_CHECK([for __func__ symbol],
[ac_cv_have___func___symbol],
[
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[]],
      [[
        char const * func = __func__;
      ]]
    )],
    [ac_cv_have___func___symbol=yes],
    [ac_cv_have___func___symbol=no])
])
])
