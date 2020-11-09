dnl Check for the __declspec(dllexport) construct support.

AC_DEFUN([AX_DECLSPEC],
[
AH_TEMPLATE($1[]_IMPORT,
  [Defined if the compiler understands __declspec(dllimport)
   or __attribute__((visibility("default"))) or __global construct.])
AH_TEMPLATE($1[]_EXPORT,
  [Defined if the compiler understands __declspec(dllimport)
   or __attribute__((visibility("default"))) or __global construct.])
AH_TEMPLATE($1[]_PRIVATE,
  [Defined if the compiler understands __attribute__((visibility("hidden")))
   or __hidden construct.])

AS_VAR_SET([continue_checks], [1])

dnl This is for Solaris platforms with Solaris Studio.

AC_CACHE_CHECK([for __global and __hidden], [ac_cv__global],
[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
        __global int x = 0;
        __global int foo ();
        int foo () { return 0; }
        __global int bar () { return x; }
        __hidden int baz () { return 1; }
        class __global Class { public: Class () { } };
      ]],
      [[]])],
    [ac_cv__global=yes],
    [ac_cv__global=no])
])

AS_IF([test "x$ac_cv__global" = "xyes"],
  [AC_DEFINE($1[]_IMPORT, [__global])
   AC_DEFINE($1[]_EXPORT, [__global])
   AC_DEFINE($1[]_PRIVATE, [__hidden])
   AS_UNSET([continue_checks])])

dnl This typically succeeds on Windows/MinGW/Cygwin.

AS_VAR_SET_IF([continue_checks],
[
AC_CACHE_CHECK([for __declspec(dllexport) and __declspec(dllimport)],
  [ac_cv_declspec],
[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#if defined (__clang__) || defined (__HAIKU__)
// Here the problem is that Clang only warns that it does not support
// __declspec(dllexport) but still compiles the executable. GCC on Haiku OS
// suffers from the same problem.
#  error Please fail.
And extra please fail.
#else
        __declspec(dllexport) int x = 0;
        __declspec(dllexport) int foo ();
        int foo () { return 0; }
        __declspec(dllexport) int bar () { return x; }
        class __declspec(dllexport) Class { public: Class () { } };
#endif
      ]],
      [[]])],
    [ac_cv_declspec=yes],
    [ac_cv_declspec=no])
])

AS_IF([test "x$ac_cv_declspec" = "xyes"],
  [AC_DEFINE($1[]_IMPORT, [__declspec(dllimport)])
   AC_DEFINE($1[]_EXPORT, [__declspec(dllexport)])
   AC_DEFINE($1[]_PRIVATE, [/* empty */])
   AS_UNSET([continue_checks])])
])

dnl This typically succeeds on *NIX platforms with GCC or Clang.

AS_VAR_SET_IF([continue_checks],
[
AC_CACHE_CHECK([for __attribute__((visibility("default")))dnl
 and __attribute__((visibility("hidden")))], [ac_cv__attribute__visibility],
[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#if defined (__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 1))
#  error Please fail.
And extra please fail.
#else
        __attribute__((visibility("default"))) int x = 0;
        __attribute__((visibility("default"))) int foo ();
        int foo () { return 0; }
        __attribute__((visibility("default"))) int bar () { return x; }
        __attribute__((visibility("hidden"))) int baz () { return 1; }
        class __attribute__((visibility("default"))) Class { public: Class () { } };
#endif
      ]],
      [[]])],
    [ac_cv__attribute__visibility=yes],
    [ac_cv__attribute__visibility=no])
])

AS_IF([test "x$ac_cv__attribute__visibility" = "xyes"],
  [AC_DEFINE($1[]_IMPORT, [__attribute__ ((visibility("default")))])
   AC_DEFINE($1[]_EXPORT, [__attribute__ ((visibility("default")))])
   AC_DEFINE($1[]_PRIVATE, [__attribute__ ((visibility("hidden")))])
   AS_UNSET([continue_checks])])
])


AS_IF([test "x$ac_cv__attribute__visibility" = "xno" dnl
       && test "x$ac_cv_declspec" = "xno" dnl
       && test "x$ax_cv__global" = "xno"],
  [AC_DEFINE($1[]_IMPORT, [/* empty */])
   AC_DEFINE($1[]_EXPORT, [/* empty */])
   AC_DEFINE($1[]_PRIVATE, [/* empty */])])

AS_UNSET([continue_checks])

]) dnl AX_DECLSPEC
