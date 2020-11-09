dnl Process this file with autoconf to produce a configure script.
dnl This version of log4cplus requires the follwing autotools versions:
dnl autoconf-2.69
dnl automake-1.16.1
dnl libtool-2.4.6

AC_INIT([log4cplus],[2.0.3])
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR([src/logger.cxx])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.14 no-define no-dist nostdinc foreign subdir-objects -Wall])
AC_CONFIG_TESTDIR([tests])
AM_MAINTAINER_MODE([enable])
AM_PROG_AR

#
# +1 : ? : +1  == new interface that does not break old one
# +1 : ? : 0   == new interface that breaks old one
#  ? : ? : 0   == no new interfaces, but breaks apps
#  ? :+1 : ?   == just some internal changes, nothing breaks but might work
#                 better
# CURRENT : REVISION : AGE
LT_VERSION=7:3:4
LT_RELEASE=2.0
AC_SUBST([LT_VERSION])
AC_SUBST([LT_RELEASE])

dnl Use C++ std::locale and std::codecvt facet for string conversion.

LOG4CPLUS_ARG_WITH([working-locale],
  [Use std::locale for char->wchar_t conversion.],
  [with_working_locale=no])

LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_WORKING_LOCALE],
  [Define for compilers/standard libraries that support more than
   just the "C" locale.],
   [test "x$with_working_locale" = "xyes"], [1])

dnl Use C99 wcsrtombs() etc. functions for string conversion.

LOG4CPLUS_ARG_WITH([working-c-locale],
  [Use C99 locale facilities for char->wchar_t conversion.],
  [with_working_c_locale=no])

LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_WORKING_C_LOCALE],
  [Define for C99 compilers/standard libraries that support more than
   just the "C" locale.],
  [test "x$with_working_c_locale" = "xyes"], [1])

dnl Use iconv() function for string conversion.

LOG4CPLUS_ARG_WITH([iconv],
  [Use iconv() for char->wchar_t conversion.],
  [with_iconv=no])

LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_WITH_ICONV],
  [Define when iconv() is available.],
  [test "x$with_iconv" = "xyes"], [1])

AS_IF([test "x$with_working_locale" = "xno" \
  -a "x$with_working_c_locale" = "xno" \
  -a "x$with_iconv" = "xno"],
  [AC_MSG_WARN([Neither C++ locale support nor C locale support \
nor iconv() support requested, using poor man's locale conversion.])])

dnl Debugging or release build?

LOG4CPLUS_ARG_ENABLE([debugging],
  [Turns off optimization and adds switches that generate debugging
   information. [default=no]],
  [enable_debugging=no])

AH_TEMPLATE([LOG4CPLUS_DEBUGGING],
  [Defined for --enable-debugging builds.])

AS_IF([test "x$enable_debugging" = "xyes"],
  [AC_DEFINE([LOG4CPLUS_DEBUGGING])
   LOG4CPLUS_NDEBUG=-UNDEBUG
   AS_CASE(["$target_os"],
     [linux*],
     [dnl The -U_FORTIFY_SOURCE bit is here to avoid redefinition
      dnl warning on Ubuntu where _FORTIFY_SOURCE is enabled by
      dnl default.
      AS_VAR_APPEND([CPPFLAGS], [" -U_FORTIFY_SOURCE"])
      AS_VAR_APPEND([CPPFLAGS], [" -D_FORTIFY_SOURCE=1"])])],
  [LOG4CPLUS_NDEBUG=-DNDEBUG])
AC_SUBST([LOG4CPLUS_NDEBUG])

dnl Turn warnings on/off?

LOG4CPLUS_ARG_ENABLE([warnings],
  [Use compiler warnings option, e.g. -Wall. [default=yes]],
  [enable_warnings=yes])

dnl Enable SO version.

LOG4CPLUS_ARG_ENABLE([so-version],
  [Use libtool -version-info option. [default=yes]],
  [enable_so_version=yes])
AM_CONDITIONAL([ENABLE_VERSION_INFO_OPTION],
  [test "x$enable_so_version" = "xyes"])

dnl Enable release version.

LOG4CPLUS_ARG_ENABLE([release-version],
  [Use libtool -release option. [default=yes]],
  [enable_release_version=yes])
AM_CONDITIONAL([ENABLE_RELEASE_OPTION],
  [test "x$enable_release_version" = "xyes"])

dnl Enable use of symbol visibility options.

LOG4CPLUS_ARG_ENABLE([symbols-visibility-options],
  [Use platform and compiler specific symbols visibility options,
   where they are available. [default=yes]],
  [enable_symbols_visibility_options=yes])

dnl Build wchar_t aware alternative.

LOG4CPLUS_ARG_WITH([wchar_t-support],
  [Additionally, produce binaries with log4cplus::tchar as wchar_t.
   The additional produced binaries will have 'U' suffix. [default=yes]],
  [with_wchar_t_support=yes])
AM_CONDITIONAL([BUILD_WITH_WCHAR_T_SUPPORT],
  [test "x$with_wchar_t_support" = "xyes"])
AS_VAR_SET([BUILD_WITH_WCHAR_T_SUPPORT], [$with_wchar_t_support])
AC_SUBST([BUILD_WITH_WCHAR_T_SUPPORT])

dnl Enable unit tests

LOG4CPLUS_ARG_ENABLE([unit-tests],
  [Enable unit tests [default=no]],
  [enable_unit_tests=no])

LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_WITH_UNIT_TESTS],
  [Defined to enable unit tests.],
  [test "x$enable_unit_tests" = "xyes"], [1])

dnl Enable LTO build.

LOG4CPLUS_ARG_ENABLE([lto],
  [Enable LTO build [default=no]],
  [enable_lto=no])
LOG4CPLUS_LTO_LDFLAGS=
LOG4CPLUS_LTO_CXXFLAGS=

dnl Check for c/c++ compilers.

AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG([C++])

AX_CXX_COMPILE_STDCXX_11

AS_VAR_APPEND([CPPFLAGS], [" -D_GNU_SOURCE=1"])

AS_CASE(["$target_os"],
  [mingw*],
  [dnl Undefining __STRICT_ANSI__ is a workaround for MinGW
   dnl compilation problem.  C++ <cwchar> header does not compile in
   dnl GCC 4.4.0 because some functions like swprintf() are not
   dnl visible.
   AS_VAR_APPEND([CPPFLAGS], [" -U__STRICT_ANSI__"])
   AS_VAR_APPEND([CPPFLAGS], [" -DWIN32"])
   AS_VAR_APPEND([CPPFLAGS], [" -D_WIN32"])
   AS_VAR_APPEND([CPPFLAGS], [" -DMINGW_HAS_SECURE_API=1"])
   AS_IF([AS_ECHO_N(["$CPPFLAGS"]) dnl
          | $GREP -e '_WIN32_WINNT=' >/dev/null],
     [],
     [AS_VAR_APPEND([CPPFLAGS], [" -D_WIN32_WINNT=0x600"])])],
  [cygwin*],
  [dnl Undefining __STRICT_ANSI__ is a workaround for compilation
   dnl problem.  C <stdio.h> header does not define vsnprintf() or
   dnl _vsnprintf().
   AS_VAR_APPEND([CPPFLAGS], [" -U__STRICT_ANSI__"])],
  [hpux*],
  [dnl We need vsnprintf() to return required buffer size instead of -1 when
   dnl buffer is too small. For this to happen we have to compile for Unix 2003
   dnl standard. The man page standards(5) for HP-UX says have to define
   dnl _XOPEN_SOURCE=600 preprocessor symbol and UNIX_STD=2003 environment
   dnl variable.
   AS_IF([AS_ECHO_N(["$CPPFLAGS"]) dnl
          | $GREP -e '_XOPEN_SOURCE=' >/dev/null],
     [],
     [AS_VAR_APPEND([CPPFLAGS], [" -D_XOPEN_SOURCE=600"])])

   AS_IF([test -z "$UNIX_STD"],
     [UNIX_STD=2003])
   AS_IF([test "$UNIX_STD" -lt 2003],
     [UNIX_STD=2003])
   export UNIX_STD])

LOG4CPLUS_ARG_ENABLE([profiling],
  [Compile with profiling compiler options. [default=no]],
  [enable_profiling=no])
LOG4CPLUS_PROFILING_LDFLAGS=
LOG4CPLUS_PROFILING_CXXFLAGS=

dnl Checks for programs.

AX_COMPILER_VENDOR

AS_IF([test "x$enable_warnings" = "xyes"],
  [AS_CASE([$ax_cv_cxx_compiler_vendor],
    dnl Skip all warnings detection for Solaris Studio compilers. It
    dnl adds the -v option and that breaks PIC support detection.
    [sun], [],
    [AX_CXXFLAGS_WARN_ALL])])

LOG4CPLUS_AIX_XLC_LDFLAGS=
AS_CASE([$ax_cv_cxx_compiler_vendor],
  [ibm],
  [AS_VAR_APPEND([CXXFLAGS], [" -qroconst"])
   AS_VAR_APPEND([LOG4CPLUS_AIX_XLC_LDFLAGS], [" -qmkshrobj=-300"])],
  [gnu|clang],
  [AX_CXXFLAGS_GCC_OPTION([-fdiagnostics-show-caret])
   AX_CXXFLAGS_GCC_OPTION([-ftrack-macro-expansion])

   dnl This is Clang flavour. It defaults to on, so we do not have to
   dnl check for it here.
   dnl AX_CXXFLAGS_GCC_OPTION([-fcolor-diagnostics])

   dnl This is GCC flavour.  It defaults to off, so we have to enable
   dnl it here.
   AX_CXXFLAGS_GCC_OPTION([-fdiagnostics-color=auto])

   AS_IF([test "x$enable_warnings" = "xyes"],
     [AX_CXXFLAGS_GCC_OPTION([-Wextra])
      dnl The -ansi switch sets __STRICT_ANSI__ which disables some
      dnl useful function declarations in MinGW and Cygwin headers.
      dnl AX_CXXFLAGS_GCC_OPTION([-ansi])
      AX_CXXFLAGS_GCC_OPTION([-pedantic])
      AX_CXXFLAGS_GCC_OPTION([-Wstrict-aliasing])
      AX_CXXFLAGS_GCC_OPTION([-Wstrict-overflow])
      dnl AX_CXXFLAGS_GCC_OPTION([-Weffc++])
      AX_CXXFLAGS_GCC_OPTION([-Woverloaded-virtual])
      AS_CASE(["$target_os"],
        dnl Disable -Wold-style-cast for MinGW as it generates too
        dnl much noise there.
        [mingw*], [],
        [AX_CXXFLAGS_GCC_OPTION([-Wold-style-cast])])
      dnl AX_CXXFLAGS_GCC_OPTION([-Wabi])
      AX_CXXFLAGS_GCC_OPTION([-Wc++14-compat])
      dnl AX_CXXFLAGS_GCC_OPTION([-Wconversion])
      AX_CXXFLAGS_GCC_OPTION([-Wundef])
      AX_CXXFLAGS_GCC_OPTION([-Wshadow])
      AX_CXXFLAGS_GCC_OPTION([-Wformat])
      AX_CXXFLAGS_GCC_OPTION([-Wnoexcept])
      AX_CXXFLAGS_GCC_OPTION([-Wsuggest-attribute=format])
      dnl AX_CXXFLAGS_GCC_OPTION([-Wsuggest-attribute=pure])
      dnl AX_CXXFLAGS_GCC_OPTION([-Wsuggest-attribute=const])
      AX_CXXFLAGS_GCC_OPTION([-Wsuggest-attribute=noreturn])
      AX_CXXFLAGS_GCC_OPTION([-Wno-variadic-macros])])

   AS_IF([test "x$enable_debugging" = "xyes"],
     [AX_CXXFLAGS_GCC_OPTION([-g3])
      dnl AX_CXXFLAGS_GCC_OPTION([-fkeep-inline-functions])
      AS_CASE(["$target_os"],
        dnl Disable these for MinGW32 as it seems to generate DLLs that
        dnl segfault in DllMain().
        [mingw32], [],
        [AX_CXXFLAGS_GCC_OPTION([-fstack-check])
         AX_CXXFLAGS_GCC_OPTION([-fstack-protector])])
      dnl -Wstack-protector results into not too interesting warnings.
      dnl AX_CXXFLAGS_GCC_OPTION([-Wstack-protector])
      AX_CXXFLAGS_GCC_OPTION([-fsanitize=address])
      AX_CXXFLAGS_GCC_OPTION([-fsanitize=undefined])
      dnl The -fsanitize=thread option requires -pie or -shared flags.
      dnl AX_CXXFLAGS_GCC_OPTION([-fsanitize=thread])
      AX_CXXFLAGS_GCC_OPTION([-ftrapv])
      LOG4CPLUS_CXXFLAGS_ADD_IF_NO_OPTIMIZATION([-Og])
      LOG4CPLUS_CXXFLAGS_ADD_IF_NO_OPTIMIZATION([-O1])],
     [LOG4CPLUS_CXXFLAGS_ADD_IF_NO_OPTIMIZATION([-O2])])

   dnl Try -pg profiling option.
   AS_IF([test "x$enable_profiling" = "xyes"],
     [AX_CXXFLAGS_GCC_OPTION([-pg],[],
        [LOG4CPLUS_PROFILING_LDFLAGS="-pg"
         LOG4CPLUS_PROFILING_CXXFLAGS="-pg"])
      AX_CXXFLAGS_GCC_OPTION([-g3])])

   dnl LTO options.

   AS_IF([test "x$enable_lto" = "xyes"],
     [AX_CXXFLAGS_GCC_OPTION([-flto],[],
        [LOG4CPLUS_LTO_LDFLAGS="-flto"
         LOG4CPLUS_LTO_CXXFLAGS="-flto"])])

   dnl The --enable-auto-import and --enable-runtime-pseudo-reloc
   dnl options are required for log4cplus to work when compiled with
   dnl Cygwin's GCC 4.3.x.
   AS_CASE(["$target_os"],
     [cygwin*|mingw*],
     [LDFLAGS="${LDFLAGS} -Wl,--enable-auto-import"
      LDFLAGS="${LDFLAGS} -Wl,--enable-runtime-pseudo-reloc"])],

  [sun],
  [AS_IF([test "x$enable_warnings" = "xyes"],
     [dnl This test is disabled. See comment before
      dnl AX_CXXFLAGS_WARN_ALL invocation.
      dnl AX_CXXFLAGS_SUN_OPTION([-v])
      AX_CXXFLAGS_SUN_OPTION([+w])])

   dnl Sun CC does support -g flag.
   AS_IF([test "x$enable_debugging" = "xyes"],
     [AX_CXXFLAGS_SUN_OPTION([-g])])

   dnl Try -pg profiling option.
   AS_IF([test "x$enable_profiling" = "xyes"],
     [AX_CXXFLAGS_SUN_OPTION([-pg],[],
        [LOG4CPLUS_PROFILING_LDFLAGS="-pg"
         LOG4CPLUS_PROFILING_CXXFLAGS="-pg"])
      AX_CXXFLAGS_SUN_OPTION([-g])])

   dnl This is here for compilation with _GNU_SOURCE=1 on Linux. Their
   dnl headers then use zero length arrays which are not supported by
   dnl C++ itself.
   AX_CXXFLAGS_SUN_OPTION([-features=zla])

   dnl This is for __func__ symbol. However it is disabled as it would
   dnl force user code to use the switch as well.
   dnl AX_CXXFLAGS_SUN_OPTION([-features=extensions])

   dnl log4cplus does not work with the default C++ library
   dnl implementation from Sun everywhere. E.g., on Linux it seems to
   dnl be missing std::FILE, etc., in the std:: namespace but on
   dnl Solaris it works. Thus we use the available STLport that they
   dnl also ship unless the user specifies own
   dnl -library=(stlport4|stdcxx4|Cstd) switch.

   AC_REQUIRE([AC_PROG_GREP])
   AS_IF([AS_ECHO_N(["$CXXFLAGS"]) dnl
          | $GREP -e '-library=\(stlport4\|stdcxx4\|Cstd\)' >/dev/null],
     [],
     [AX_CXXFLAGS_SUN_OPTION([-library=stlport4])])

   dnl Adding -library=Crun is always necessary.
   AX_CXXFLAGS_SUN_OPTION([-library=Crun])])
AC_SUBST([LOG4CPLUS_AIX_XLC_LDFLAGS])

use_export_symbols_regex=no
AS_IF([test "x$enable_symbols_visibility_options" = "xyes"],
  [AX_DECLSPEC([LOG4CPLUS_DECLSPEC])
   AS_IF([test "x$ac_cv_declspec" = "xyes" dnl
    || test "x$ac_cv__attribute__visibility" = "xyes" dnl
    || test "x$ac_cv__global" = "xyes"],
    [AS_CASE([$ax_cv_cxx_compiler_vendor],
      [gnu|clang],
      [AX_CXXFLAGS_GCC_OPTION([-fvisibility=hidden])],

      dnl Adding -Bhidden or -Bhidden_def to CXXFLAGS causes configure
      dnl tests to fail because main() becomes hidden.
      dnl[hp],
      dnl[AS_VAR_APPEND([CXXFLAGS], [" -Bhidden_def -Wl,-a"])],

      [sun],
      [AX_CXXFLAGS_SUN_OPTION([-xldscope=hidden])],

      [use_export_symbols_regex=yes])],
    [use_export_symbols_regex=yes])])
AM_CONDITIONAL([USE_LOG4CPLUS_EXPORT_SYMBOLS_REGEX],
  [test "x$use_export_symbols_regex" = "xyes"])

AX___FUNCTION___MACRO
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_FUNCTION_MACRO],
  [Define if compiler supports __FUNCTION__ macro.],
  [test "x$ac_cv_have___function___macro" = "xyes"], [1])

AX___PRETTY_FUNCTION___MACRO
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_PRETTY_FUNCTION_MACRO],
  [Define if compiler supports __PRETTY_FUNCTION__ macro.],
  [test "x$ac_cv_have___pretty_function___macro" = "xyes"], [1])

AX___FUNC___SYMBOL
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_FUNC_SYMBOL],
  [Define if compiler supports __func__ symbol.],
  [test "x$ac_cv_have___func___symbol" = "xyes"], [1])

AX_GCC_FUNC_ATTRIBUTE([constructor_priority])
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR_PRIORITY],
  [Define if compiler supports __attribute__((constructor(PRIORITY))).],
  [test "x$ax_cv_have_func_attribute_constructor_priority" = "xyes"], [1])

AX_GCC_FUNC_ATTRIBUTE([constructor])
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR],
  [Define if compiler supports __attribute__((constructor)).],
  [test "x$ax_cv_have_func_attribute_constructor" = "xyes"], [1])

AX_GCC_VAR_ATTRIBUTE([init_priority])
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_VAR_ATTRIBUTE_INIT_PRIORITY],
  [Define if compiler supports __attribute__((init_priority(PRIORITY))).],
  [test "x$ax_cv_have_var_attribute_init_priority" = "xyes"], [1])

dnl Checks for libraries.

AC_SEARCH_LIBS([strerror], [cposix])
dnl On some systems libcompat exists only as a static library which
dnl breaks compilation of shared library log4cplus.
dnl AC_SEARCH_LIBS([ftime], [compat])
AC_SEARCH_LIBS([gethostbyname], [nsl network net])
AC_SEARCH_LIBS([setsockopt], [socket network net])
AS_IF([test "x$with_iconv" = "xyes"],
  [AC_SEARCH_LIBS([iconv_open], [iconv], [],
     [AC_SEARCH_LIBS([libiconv_open], [iconv])])])

dnl Windows/MinGW specific.

AS_CASE(["$target_os"],
  [mingw*], [
    AC_LANG_PUSH([C])
    AC_CHECK_LIB([kernel32], [main])
    AC_CHECK_LIB([advapi32], [main])
    AC_CHECK_LIB([ws2_32], [main])
    AC_CHECK_LIB([oleaut32], [main])
    AC_LANG_POP([C])],
  [cygwin*], [
    AC_LANG_PUSH([C])
    AC_CHECK_LIB([kernel32], [main])
    AC_CHECK_LIB([oleaut32], [main])
    AC_LANG_POP([C])])

dnl Checks for header files.

AC_HEADER_STDC

LOG4CPLUS_CHECK_HEADER([sys/types.h], [LOG4CPLUS_HAVE_SYS_TYPES_H])
LOG4CPLUS_CHECK_HEADER([sys/socket.h], [LOG4CPLUS_HAVE_SYS_SOCKET_H])
LOG4CPLUS_CHECK_HEADER([sys/time.h], [LOG4CPLUS_HAVE_SYS_TIME_H])
LOG4CPLUS_CHECK_HEADER([sys/timeb.h], [LOG4CPLUS_HAVE_SYS_TIMEB_H])
LOG4CPLUS_CHECK_HEADER([sys/stat.h], [LOG4CPLUS_HAVE_SYS_STAT_H])
LOG4CPLUS_CHECK_HEADER([sys/syscall.h], [LOG4CPLUS_HAVE_SYS_SYSCALL_H])
LOG4CPLUS_CHECK_HEADER([sys/file.h], [LOG4CPLUS_HAVE_SYS_FILE_H])
LOG4CPLUS_CHECK_HEADER([syslog.h], [LOG4CPLUS_HAVE_SYSLOG_H])
LOG4CPLUS_CHECK_HEADER([arpa/inet.h], [LOG4CPLUS_HAVE_ARPA_INET_H])
LOG4CPLUS_CHECK_HEADER([netinet/in.h], [LOG4CPLUS_HAVE_NETINET_IN_H])
LOG4CPLUS_CHECK_HEADER([netinet/tcp.h], [LOG4CPLUS_HAVE_NETINET_TCP_H])
LOG4CPLUS_CHECK_HEADER([netdb.h], [LOG4CPLUS_HAVE_NETDB_H])
LOG4CPLUS_CHECK_HEADER([unistd.h], [LOG4CPLUS_HAVE_UNISTD_H])
LOG4CPLUS_CHECK_HEADER([fcntl.h], [LOG4CPLUS_HAVE_FCNTL_H])
LOG4CPLUS_CHECK_HEADER([stdio.h], [LOG4CPLUS_HAVE_STDIO_H])
LOG4CPLUS_CHECK_HEADER([stdarg.h], [LOG4CPLUS_HAVE_STDARG_H])
LOG4CPLUS_CHECK_HEADER([stdlib.h], [LOG4CPLUS_HAVE_STDLIB_H])
LOG4CPLUS_CHECK_HEADER([wchar.h], [LOG4CPLUS_HAVE_WCHAR_H])
LOG4CPLUS_CHECK_HEADER([time.h], [LOG4CPLUS_HAVE_TIME_H])
LOG4CPLUS_CHECK_HEADER([errno.h], [LOG4CPLUS_HAVE_ERRNO_H])
LOG4CPLUS_CHECK_HEADER([limits.h], [LOG4CPLUS_HAVE_LIMITS_H])
LOG4CPLUS_CHECK_HEADER([poll.h], [LOG4CPLUS_HAVE_POLL_H])
AS_IF([test "x$with_iconv" = "xyes"],
  [LOG4CPLUS_CHECK_HEADER([iconv.h], [LOG4CPLUS_HAVE_ICONV_H])])

dnl Check for the existance of type socklen_t.

AX_TYPE_SOCKLEN_T

dnl Check for single-threaded compilation

AH_TEMPLATE([LOG4CPLUS_USE_PTHREADS])
LOG4CPLUS_ARG_ENABLE([threads],
  [Use this option to create a singled-threaded version of this library [default=yes]],
  [enable_threads=yes])
AS_VAR_SET([ENABLE_THREADS], [$enable_threads])
AC_SUBST([ENABLE_THREADS])

AS_IF([test "x$enable_threads" = "xyes"],
dnl Multi threaded library.
  [AC_MSG_NOTICE([Creating a multi-threaded library.])
   AC_MSG_NOTICE([Threads support:])

   AS_VAR_APPEND([CPPFLAGS], [" -D_REENTRANT"])

   AS_CASE(["$target_os"],
     [mingw*], [],
     [AX_PTHREAD_CXX([],
       [AC_MSG_ERROR([Requested threads support but no threads were found.])])])

   AS_VAR_APPEND([CXXFLAGS], [" $PTHREAD_CXXFLAGS"])
   AS_VAR_APPEND([LIBS], [" $PTHREAD_LIBS"])

   dnl required on HP-UX
   AC_SEARCH_LIBS([sem_init], [rt])

   AS_CASE([$ax_cv_cxx_compiler_vendor],
     [gnu|clang],
     [dnl GCC, empty case to avoid false positive for -qtls check.
     ],
     [sun],
     [dnl Check for Sun's compiler's TLS option.
      AX_CXXFLAGS_SUN_OPTION([-xthreadvar])],
     [ibm],
     [dnl Check for IBM's compiler's TLS option.
      AX_CXXFLAGS_AIX_OPTION([-qtls])])

   dnl Check if TLS is supported, but only if it isn't iOS target, because it
   dnl is forbidden by Apple Store to use __tlv_* functions. Also, Cygwin
   dnl passes the compilation and link test here but fails during linking of
   dnl the real code, so we want to avoid it too. See GCC PR64697.
   AS_IF([test \( "$target_os" != "darwin" -o "$target_cpu" != "arm" \) \
               -a "$target_os" != "cygwin"],
     [AX_TLS_SUPPORT])

   AH_TEMPLATE([LOG4CPLUS_HAVE_TLS_SUPPORT])
   AH_TEMPLATE([LOG4CPLUS_THREAD_LOCAL_VAR])
   AS_IF([test "x$ac_cv_thread_local" = "xyes"],
     [AC_DEFINE([LOG4CPLUS_HAVE_TLS_SUPPORT])
      AC_DEFINE([LOG4CPLUS_THREAD_LOCAL_VAR], [thread_local])],
     [AS_IF([test "x$ac_cv_declspec_thread" = "xyes"],
        [AC_DEFINE([LOG4CPLUS_HAVE_TLS_SUPPORT])
         AC_DEFINE([LOG4CPLUS_THREAD_LOCAL_VAR], [__declspec(thread)])],
        [AS_IF([test "x$ac_cv__thread_keyword" = "xyes"],
           [AC_DEFINE([LOG4CPLUS_HAVE_TLS_SUPPORT])
            AC_DEFINE([LOG4CPLUS_THREAD_LOCAL_VAR], [__thread])])])])],

dnl Single threaded library.
  [AC_MSG_NOTICE([Creating a single-threaded library])
   AC_DEFINE([LOG4CPLUS_SINGLE_THREADED], [1],
     [Define if this is a single-threaded library.])])

AM_CONDITIONAL([MULTI_THREADED], [test "x$enable_threads" = "xyes"])

dnl Library functions checks are here because preprocessor symbols
dnl like _REENTRANT can influence availability of some functions.

dnl Checks for library functions.

LOG4CPLUS_CHECK_FUNCS([gmtime_r], [LOG4CPLUS_HAVE_GMTIME_R])
LOG4CPLUS_CHECK_FUNCS([localtime_r], [LOG4CPLUS_HAVE_LOCALTIME_R])
LOG4CPLUS_CHECK_FUNCS([getpid], [LOG4CPLUS_HAVE_GETPID])
LOG4CPLUS_CHECK_FUNCS([poll], [LOG4CPLUS_HAVE_POLL])
LOG4CPLUS_CHECK_FUNCS([pipe], [LOG4CPLUS_HAVE_PIPE])
LOG4CPLUS_CHECK_FUNCS([pipe2], [LOG4CPLUS_HAVE_PIPE2])
LOG4CPLUS_CHECK_FUNCS([ftime], [LOG4CPLUS_HAVE_FTIME])
LOG4CPLUS_CHECK_FUNCS([stat], [LOG4CPLUS_HAVE_STAT])
LOG4CPLUS_CHECK_FUNCS([lstat], [LOG4CPLUS_HAVE_LSTAT])
LOG4CPLUS_CHECK_FUNCS([fcntl], [LOG4CPLUS_HAVE_FCNTL])
LOG4CPLUS_CHECK_FUNCS([lockf], [LOG4CPLUS_HAVE_LOCKF])
LOG4CPLUS_CHECK_FUNCS([flock], [LOG4CPLUS_HAVE_FLOCK])
LOG4CPLUS_CHECK_FUNCS([htons], [LOG4CPLUS_HAVE_HTONS])
LOG4CPLUS_CHECK_FUNCS([ntohs], [LOG4CPLUS_HAVE_NTOHS])
LOG4CPLUS_CHECK_FUNCS([htonl], [LOG4CPLUS_HAVE_HTONL])
LOG4CPLUS_CHECK_FUNCS([ntohl], [LOG4CPLUS_HAVE_NTOHL])
LOG4CPLUS_CHECK_FUNCS([shutdown], [LOG4CPLUS_HAVE_SHUTDOWN])
LOG4CPLUS_CHECK_FUNCS([mbstowcs], [LOG4CPLUS_HAVE_MBSTOWCS])
LOG4CPLUS_CHECK_FUNCS([wcstombs], [LOG4CPLUS_HAVE_WCSTOMBS])
AS_IF([test "x$with_iconv" = "xyes"],
  [LOG4CPLUS_CHECK_FUNCS([iconv_open], [LOG4CPLUS_HAVE_ICONV_OPEN])
   LOG4CPLUS_CHECK_FUNCS([iconv_close], [LOG4CPLUS_HAVE_ICONV_CLOSE])
   LOG4CPLUS_CHECK_FUNCS([iconv], [LOG4CPLUS_HAVE_ICONV])
   LOG4CPLUS_CHECK_FUNCS([libiconv_open], [LOG4CPLUS_HAVE_ICONV_OPEN])
   LOG4CPLUS_CHECK_FUNCS([libiconv_close], [LOG4CPLUS_HAVE_ICONV_CLOSE])
   LOG4CPLUS_CHECK_FUNCS([libiconv], [LOG4CPLUS_HAVE_ICONV])])

dnl Various functions used by snprintf.cxx.

LOG4CPLUS_CHECK_FUNCS([vsnprintf], [LOG4CPLUS_HAVE_VSNPRINTF])
LOG4CPLUS_CHECK_FUNCS([_vsnprintf], [LOG4CPLUS_HAVE__VSNPRINTF])
LOG4CPLUS_CHECK_FUNCS([vsnwprintf], [LOG4CPLUS_HAVE_VSNWPRINTF])
LOG4CPLUS_CHECK_FUNCS([_vsnwprintf], [LOG4CPLUS_HAVE__VSNWPRINTF])
LOG4CPLUS_CHECK_FUNCS([vsprintf_s], [LOG4CPLUS_HAVE_VSPRINTF_S])
LOG4CPLUS_CHECK_FUNCS([vswprintf_s], [LOG4CPLUS_HAVE_VSWPRINTF_S])
LOG4CPLUS_CHECK_FUNCS([vfprintf_s], [LOG4CPLUS_HAVE_VFPRINTF_S])
LOG4CPLUS_CHECK_FUNCS([vfwprintf_s], [LOG4CPLUS_HAVE_VFWPRINTF_S])
LOG4CPLUS_CHECK_FUNCS([_vsnprintf_s], [LOG4CPLUS_HAVE__VSNPRINTF_S])
LOG4CPLUS_CHECK_FUNCS([_vsnwprintf_s], [LOG4CPLUS_HAVE__VSNWPRINTF_S])

dnl Checks for vsprintf, vfprintf and vswprintf should not be necessary, they
dnl ought to be in std:: namespace via the <cstdio>, <cwchar> headers.

AS_CASE(["$target_os"],
  [cygwin*], [
    LOG4CPLUS_CHECK_FUNCS([OutputDebugStringW],
      [LOG4CPLUS_HAVE_OUTPUTDEBUGSTRING])])

AC_CACHE_CHECK([for ENAMETOOLONG], [ax_cv_have_enametoolong],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <errno.h>], [int value = ENAMETOOLONG;])],
    [ax_cv_have_enametoolong=yes],
    [ax_cv_have_enametoolong=no])])
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_ENAMETOOLONG],
  [Define if ENAMETOOLONG is provided.],
  [test "x$ax_cv_have_enametoolong" = "xyes"], [1])

AX_GETADDRINFO
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_GETADDRINFO],
  [Define if getaddrinfo() is provided.],
  [test "x$ax_cv_have_getaddrinfo" = "xyes"], [1])

AX_GETHOSTBYNAME_R
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_GETHOSTBYNAME_R],
  [Define if gethostbyname_r() is provided.],
  [test "x$ax_cv_have_gethostbyname_r" = "xyes"], [1])

AC_CACHE_CHECK([for gettid], [ax_cv_have_gettid],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>], [int rv = syscall(SYS_gettid);])],
    [ax_cv_have_gettid=yes],
    [ax_cv_have_gettid=no])])
LOG4CPLUS_DEFINE_MACRO_IF([LOG4CPLUS_HAVE_GETTID],
  [Define if gettid() Linux syscall is available.],
  [test "x$ax_cv_have_gettid" = "xyes"], [1])

dnl Qt4 setup using pkg-config.

PKG_PROG_PKG_CONFIG

LOG4CPLUS_ARG_WITH([qt],
  [Build liblog4cplusqt4debugappender.],
  [with_qt=no])

AS_IF([test "x$with_qt" = "xyes"],
  [PKG_CHECK_MODULES([QT], [QtCore >= 4.0.0])],
  [QT_CFLAGS=
   QT_LIBS=])
AM_CONDITIONAL([QT], [test "x$with_qt" = "xyes"])
AC_SUBST([QT_CFLAGS])
AC_SUBST([QT_LIBS])

dnl Qt5 setup using pkg-config.

LOG4CPLUS_ARG_WITH([qt5],
  [Build liblog4cplusqt5debugappender.],
  [with_qt5=no])

AS_IF([test "x$with_qt5" = "xyes"],
  [PKG_CHECK_MODULES([QT5], [Qt5Core >= 5.0.0])],
  [QT5_CFLAGS=
   QT5_LIBS=])
AM_CONDITIONAL([QT5], [test "x$with_qt5" = "xyes"])
AC_SUBST([QT5_CFLAGS])
AC_SUBST([QT5_LIBS])

dnl SWIG generated bindings related configure machinery is here.

AS_VAR_SET([with_swig], [no])

AC_ARG_WITH([python],
  [AS_HELP_STRING([--with-python], [Build Python/SWIG bindings.])],
  [LOG4CPLUS_CHECK_YESNO([${withval}], [--with-python])
   AS_IF([test "x$with_python" = "xyes"], [AS_VAR_SET([with_swig], [yes])])],
  [AS_VAR_SET([with_python], [no])])

AS_IF([test "x$with_swig" = "xyes"],
  [AX_PKG_SWIG([2.0.0], [:], [AC_MSG_ERROR([SWIG is required to build.])])
   AX_SWIG_MULTI_MODULE_SUPPORT])

AM_CONDITIONAL([WITH_SWIG], [test "x$with_swig" = "xyes"])

AS_IF([test "x$with_python" = "xyes"],
  [AM_PATH_PYTHON([2.3])
   AX_PYTHON_DEVEL
   AC_SUBST([AX_SWIG_PYTHON_OPT],[-python$swig_shadow])
   AC_SUBST([AX_SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])])

AM_CONDITIONAL([WITH_PYTHON], [test "x$with_python" = "xyes"])

dnl Libtool setup.
dnl Must be latest to catch rpath compiled in by changes made during
dnl checks before.

LT_PREREQ([2.4.2])
LT_INIT([win32-dll disable-static pic-only])

AH_TOP([#ifndef LOG4CPLUS_CONFIG_H])
AH_TOP([#define LOG4CPLUS_CONFIG_H])
AH_BOTTOM([#endif // LOG4CPLUS_CONFIG_H])

AC_CONFIG_HEADERS([include/log4cplus/config.h])
AC_CONFIG_HEADERS([include/log4cplus/config/defines.hxx])
AC_CONFIG_FILES([Makefile
           include/Makefile
           tests/atlocal
           log4cplus.pc])

m4_include([tests/configure.m4])

AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_SUBST([LOG4CPLUS_PROFILING_LDFLAGS])
AC_SUBST([LOG4CPLUS_PROFILING_CXXFLAGS])

AC_SUBST([LOG4CPLUS_LTO_LDFLAGS])
AC_SUBST([LOG4CPLUS_LTO_CXXFLAGS])

AC_OUTPUT
