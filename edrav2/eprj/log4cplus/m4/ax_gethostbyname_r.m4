dnl AX_GETHOSTBYNAME_R checks for gethostbyname_r() function and sets
dnl $ax_cv_have_gethostbyname_r accordingly and defines
dnl HAVE_GETHOSTBYNAME_R.

AC_DEFUN([AX_GETHOSTBYNAME_R], [
AH_TEMPLATE([HAVE_GETHOSTBYNAME_R])
AC_CACHE_CHECK([for gethostbyname_r], [ax_cv_have_gethostbyname_r],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([
#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
    ], [
gethostbyname_r (NULL, NULL, NULL, 0, NULL, NULL);
    ])],
    [ax_cv_have_gethostbyname_r=yes],
    [ax_cv_have_gethostbyname_r=no])])

AS_IF([test "x$ax_cv_have_gethostbyname_r" = "xyes"],
  [AC_DEFINE([HAVE_GETHOSTBYNAME_R])])
])
