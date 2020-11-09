dnl Check for the __thread/__declspec(thread) construct support.

AC_DEFUN([AX_TLS_SUPPORT],
[
AH_TEMPLATE(HAVE_TLS_SUPPORT,
  [Defined if the compiler understands __thread or __declspec(thread)
   construct.])
AH_TEMPLATE(TLS_SUPPORT_CONSTRUCT,
  [Defined to the actual TLS support construct.])

ax_tls_support=no

AC_CACHE_CHECK([for thread_local], [ac_cv_thread_local],
[
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[
      // check that pointers to classes work as well
      struct S { S () { } void foo () { } int member; };
      extern thread_local S * p_s;
      thread_local S * p_s = 0;

      extern thread_local int x;
      thread_local int * ptr = 0;
      int foo () { ptr = &x; return x; }
      thread_local int x = 1;
      ]],
      [[x = 2;
        foo ();
        p_s = new S;]])],
    [ac_cv_thread_local=yes
     ax_tls_support=yes],
    [ac_cv_thread_local=no],
    [ac_cv_thread_local=no])
])
AS_IF([test "x$ac_cv_thread_local" = "xyes"],
  [AC_DEFINE(HAVE_TLS_SUPPORT, [1])
   AC_DEFINE(TLS_SUPPORT_CONSTRUCT, [thread_local])])


AS_IF([test "x$ax_tls_support" = "xno"], [
AC_CACHE_CHECK([for __thread], [ac_cv__thread_keyword], [
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[#if defined (__NetBSD__)
        #include <sys/param.h>
        #if ! __NetBSD_Prereq__(5,1,0)
        #error NetBSD __thread support does not work before 5.1.0. It is missing __tls_get_addr.
        #endif
        #endif

        // check that pointers to classes work as well
        struct S { S () { } void foo () { } int member; };
        extern __thread S * p_s;
        __thread S * p_s = 0;

        extern __thread int x;
        __thread int * ptr = 0;
        int foo () { ptr = &x; return x; }
        __thread int x = 1;
      ]],
      [[x = 2;
        foo ();
        p_s = new S;
      ]])],
    [ac_cv__thread_keyword=yes
     ax_tls_support=yes],
    [ac_cv__thread_keyword=no],
    [ac_cv__thread_keyword=no])
])

AS_IF([test "x$ac_cv__thread_keyword" = "xyes"],
  [AC_DEFINE(HAVE_TLS_SUPPORT, [1])
   AC_DEFINE(TLS_SUPPORT_CONSTRUCT, [__thread])])])


AS_IF([test "x$ax_tls_support" = "xno"], [
AC_CACHE_CHECK([for __declspec(thread)], [ac_cv_declspec_thread], [
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
       [[
#if defined (__GNUC__)
#  error Please fail.
And extra please fail.
#else
        // check that pointers to classes work as well
        struct S { S () { } void foo () { } int member; };
        extern __declspec(thread) S * p_s;
        __declspec(thread) S * p_s = 0;

       extern __declspec(thread) int x;
       __declspec(thread) int * ptr = 0;
       int foo () { ptr = &x; return x; }
       __declspec(thread) int x = 1;
#endif
       ]],
       [[x = 2;
         foo ();
         p_s = new S;]])],
    [ac_cv_declspec_thread=yes
     ax_tls_support=yes],
    [ac_cv_declspec_thread=no],
    [ac_cv_declspec_thread=no])])

  AS_IF([test "x$ac_cv_declspec_thread" = "xyes"],
    [AC_DEFINE(HAVE_TLS_SUPPORT, [1])
     AC_DEFINE(TLS_SUPPORT_CONSTRUCT, [__declspec(thread)])])])

])dnl AX_TLS_SUPPORT
