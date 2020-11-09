dnl AX_GETADDRINFO checks for getaddrinfo() function and sets
dnl $ax_cv_have_getaddrinfo accordingly and defines
dnl HAVE_GETADDRINFO.

AC_DEFUN([AX_GETADDRINFO], [
AH_TEMPLATE([HAVE_GETADDRINFO])
AC_CACHE_CHECK([for getaddrinfo], [ax_cv_have_getaddrinfo],
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
getaddrinfo (NULL, NULL, NULL, NULL);
    ])],
    [ax_cv_have_getaddrinfo=yes],
    [ax_cv_have_getaddrinfo=no])])

AS_IF([test "x$ax_cv_have_getaddrinfo" = "xyes"],
  [AC_DEFINE([HAVE_GETADDRINFO])])
])
