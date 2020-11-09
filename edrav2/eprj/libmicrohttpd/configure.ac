# This file is part of libmicrohttpd.
# (C) 2006-2018 Christian Grothoff (and other contributing authors)
#
# libmicrohttpd is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 3, or (at your
# option) any later version.
#
# libmicrohttpd is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libmicrohttpd; see the file COPYING.  If not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
#
#
# Process this file with autoconf to produce a configure script.
#
#
AC_PREREQ([2.64])
LT_PREREQ([2.4.0])
AC_INIT([GNU Libmicrohttpd],[0.9.64],[libmicrohttpd@gnu.org])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([silent-rules] [subdir-objects])
AC_CONFIG_HEADERS([MHD_config.h])
AC_CONFIG_MACRO_DIR([m4])

LIB_VERSION_CURRENT=63
LIB_VERSION_REVISION=0
LIB_VERSION_AGE=51
AC_SUBST(LIB_VERSION_CURRENT)
AC_SUBST(LIB_VERSION_REVISION)
AC_SUBST(LIB_VERSION_AGE)

AC_MSG_CHECKING([[whether z/OS special settings are required]])
AS_IF([test `uname -s` = "OS/390"],
[
# configure binaries for z/OS
  AS_IF([test -z "$CC"],
        [CC=`pwd`"/contrib/xcc"
         chmod +x $CC || true])
  AS_IF([test -z "$CPP"],
        CPP="c89 -E")
  AS_IF([test -z "$CXXCPP"],
        CXXCPP="c++ -E -+")
  AC_MSG_RESULT([[yes]])
#  _CCC_CCMODE=1
#  _C89_CCMODE=1
],
  AC_MSG_RESULT([[no]])
)

# Checks for programs.
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CANONICAL_HOST
AC_PROG_CC_STDC
MHD_SYS_EXT
LT_INIT([win32-dll])
LT_LANG([Windows Resource])

# Checks for gettext.
m4_ifdef([AM_GNU_GETTEXT], [
  AM_GNU_GETTEXT([external],[need-ngettext])
  AC_CONFIG_FILES([po/Makefile.in])
  have_po=yes
], [
  have_po=no
])
m4_ifdef([AM_GNU_GETTEXT_VERSION], [
#do not indent here
AM_GNU_GETTEXT_VERSION([0.19.8])
])
AM_CONDITIONAL([HAVE_PO], [ test "$have_po" = yes ])



# Workaround for libgcrypt
AS_IF([[test "x$lt_sysroot" != "x" && test "x$SYSROOT" = "x"]], [[SYSROOT="$lt_sysroot"]])

PACKAGE_VERSION_MAJOR='m4_car(m4_unquote(m4_split(AC_PACKAGE_VERSION, [\.])))'
PACKAGE_VERSION_MINOR='m4_argn(2, m4_unquote(m4_split(AC_PACKAGE_VERSION, [\.])))'
PACKAGE_VERSION_SUBMINOR='m4_argn(3, m4_unquote(m4_split(AC_PACKAGE_VERSION, [\.])))'
AS_VAR_ARITH([MHD_W32_DLL_SUFF],[[$LIB_VERSION_CURRENT - $LIB_VERSION_AGE]])
AC_SUBST([PACKAGE_VERSION_MAJOR])
AC_SUBST([PACKAGE_VERSION_MINOR])
AC_SUBST([PACKAGE_VERSION_SUBMINOR])
AC_SUBST([MHD_W32_DLL_SUFF])
AC_CONFIG_FILES([src/microhttpd/microhttpd_dll_res.rc])

MHD_LIB_CPPFLAGS=""
MHD_LIB_CFLAGS=""
MHD_LIB_LDFLAGS=""
MHD_LIBDEPS=""
# for pkg-config
MHD_REQ_PRIVATE=''
MHD_LIBDEPS_PKGCFG=''

AH_TEMPLATE([[HAVE_STDBOOL_H]], [Define to 1 if you have the <stdbool.h> header file and <stdbool.h> defines 'bool' type.])
AH_TEMPLATE([[HAVE_REAL_BOOL]], [Define to 1 if you have the real boolean type.])
AH_TEMPLATE([[bool]], [Define to type name which will be used as boolean type.])
AC_CHECK_HEADER([stdbool.h],
  [
   AC_CHECK_TYPE([bool],
     [
      AC_DEFINE([[HAVE_STDBOOL_H]], [[1]])
      AC_DEFINE([[HAVE_REAL_BOOL]], [[1]])
     ],
     [
      AC_MSG_WARN([[Header <stdbool.h> is present, but "bool" type cannot be detected. Check compiler flags.]])
      AC_DEFINE([[bool]], [[int]])
     ], [
#include <stdbool.h>
        ]
   )
  ],
  [
   AC_CHECK_TYPE([bool],
     [AC_DEFINE([[HAVE_REAL_BOOL]], [[1]])],
     [
      AC_CHECK_TYPE([_Bool],
        [
         AC_DEFINE([[HAVE_REAL_BOOL]], [[1]])
         AC_DEFINE([[bool]], [[_Bool]])
        ],
        [
         AC_DEFINE([[bool]], [[int]])
        ], []
      )
     ], []
   )
  ],
  [AC_INCLUDES_DEFAULT]
)

AC_CACHE_CHECK([[whether "true" is defined or builtin]], [[mhd_cv_macro_true_defined]],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
        [[
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
        ]], [[
#if defined(true)
    /* dummy */
#else
    (void)true;
#endif
        ]])
     ], [[mhd_cv_macro_true_defined='yes']], [[mhd_cv_macro_true_defined='no']])
  ])
AS_VAR_IF([[mhd_cv_macro_true_defined]], [["yes"]], [[:]],
  [AC_DEFINE([[true]],[[(!0)]], [Define to value interpreted by compiler as boolean "true", if "true" is not defined by system headers.])])

AC_CACHE_CHECK([[whether "false" is defined or builtin]], [[mhd_cv_macro_false_defined]],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
        [[
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
        ]], [[
#if !defined(false)
    (void)false;
#else
    /* dummy */
#endif
        ]])
     ], [[mhd_cv_macro_false_defined='yes']], [[mhd_cv_macro_false_defined='no']])
  ])
AS_VAR_IF([[mhd_cv_macro_false_defined]], [["yes"]], [[:]],
  [AC_DEFINE([[false]],[[0]], [Define to value interpreted by compiler as boolean "false", if "false" is not defined by system headers.])])

AC_CACHE_CHECK([[whether "true" and "false" could be used]], [[mhd_cv_macro_true_false_valid]],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
        [[
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
        ]], [[
          int var1[true ? 1 : -1] = { 1 };
          int var2[false ? -1 : 1] = { 2 };
          int var3[!true ? -1 : 1] = { 3 };
          int var4[!false ? 1 : -1] = { 4 };
          if (var1[0] == var2[0] || var3[0] == var4[0])
            return 1;
        ]])
     ], [[mhd_cv_macro_true_false_valid='yes']], [[mhd_cv_macro_true_false_valid='no']])
  ])
AS_VAR_IF([[mhd_cv_macro_true_false_valid]], [["yes"]], [[:]],
  [AC_MSG_ERROR([[Value of "true" or value of "false" is not valid. Check config.log for details.]])])


AX_CHECK_COMPILE_FLAG([[-Werror=attributes]],
  [
   save_CFLAGS="$CFLAGS"
   CFLAGS="$CFLAGS -Werror=attributes"
   AC_MSG_CHECKING([[whether -Werror=attributes actually works]])
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
       [[__attribute__((non_existing_attrb_dummy)) int SimpleFunc(void) {return 3;}]], [[int r = SimpleFunc(); if (r) return r;]])],
     [
       AC_MSG_RESULT([[no]])
       errattr_CFLAGS=""
     ], [
       AC_MSG_RESULT([[yes]])
       errattr_CFLAGS="-Werror=attributes"
     ])
   CFLAGS="$save_CFLAGS"
  ],
                      [[errattr_CFLAGS=""]], [], [])
AC_MSG_CHECKING([[for function inline keywords suppoted by $CC]])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $errattr_CFLAGS"
inln_prfx="none"
# Prefer always inline functions
for inln_prfx_chk in InlineWithAttr __forceinline inline __inline__ __inline _inline _Inline; do
  # Try to link to avoid "symbol undefined" problems at build time
  AS_IF([[test "x$inln_prfx_chk" = "xInlineWithAttr"]],
    [
     AS_IF([[test "x$errattr_CFLAGS" = "x"]],
       [[ # Skip test with attribute as negative result can't be detected
          inln_prfx_chk="__forceinline" # use next value
       ]],[[inln_prfx_chk="inline __attribute__((always_inline))"]])
    ])
  AC_LINK_IFELSE(
    [
     AC_LANG_PROGRAM(
       [[
#ifdef __cplusplus
#error This test is only for C.
choke me
#endif
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
         static $inln_prfx_chk bool cmpfn(int x, int y)
         { return x > y; }
         static $inln_prfx_chk int sumfn(int x, int y)
         { return x + y; }
       ]],[[
       int a = 1, b = 100, c;
       if (cmpfn(a, b))
         c = sumfn(a, b);
       else
         c = 0 - sumfn(a, b);
       ]])
    ],
    [[ inln_prfx="$inln_prfx_chk" ]])
  test "x$inln_prfx" != "xnone" && break
done
AS_IF([[test "x$inln_prfx" != "xnone"]],
      [
       AC_DEFINE([INLINE_FUNC],[1],[Define to 1 if your C compiler supports inline functions.])
       AC_DEFINE_UNQUOTED([_MHD_static_inline],[static $inln_prfx],[Define to prefix which will be used with MHD static inline functions.])
      ], [
       AC_DEFINE([_MHD_static_inline],[static],[Define to prefix which will be used with MHD static inline functions.])
      ])
AC_MSG_RESULT([[$inln_prfx]])
CFLAGS="$save_CFLAGS"

# Check system type
shutdown_trig_select='no'
AC_MSG_CHECKING([[for target host OS]])
AS_CASE(["$host_os"],
 [*darwin* | *rhapsody* | *macosx*],
 [AC_DEFINE_UNQUOTED(OSX,1,[This is an OS X system])
     CFLAGS="-no-cpp-precomp -fno-common $CFLAGS"
     mhd_host_os='Darwin'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [freebsd*],
 [AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(FREEBSD,1,[This is a FreeBSD system])
     mhd_host_os='FreeBSD'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [openbsd*],
 [AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(OPENBSD,1,[This is an OpenBSD system])
     mhd_host_os='OpenBSD'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [netbsd*],
 [AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(NETBSD,1,[This is a NetBSD system])
     mhd_host_os='NetBSD'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [*solaris*],
 [AC_DEFINE_UNQUOTED(SOLARIS,1,[This is a Solaris system])
     AC_DEFINE_UNQUOTED(_REENTRANT,1,[Need with solaris or errno doesnt work])
     mhd_host_os='Solaris'
     AC_MSG_RESULT([[$mhd_host_os]])
     AC_SEARCH_LIBS(gethostbyname, nsl)
     AC_SEARCH_LIBS(socket, socket)],
  [*arm-linux*],
  [AC_DEFINE_UNQUOTED(LINUX,1,[This is a Linux kernel])
     mhd_host_os='ARM Linux'
     AC_MSG_RESULT([[$mhd_host_os]])
     CFLAGS="-fPIC -pipe $CFLAGS"],
  [*linux*],
  [AC_DEFINE_UNQUOTED(LINUX,1,[This is a Linux kernel])
     mhd_host_os='Linux'
     AC_MSG_RESULT([[$mhd_host_os]])],
  [*cygwin*],
  [AC_DEFINE_UNQUOTED(CYGWIN,1,[This is a Cygwin system])
     mhd_host_os='Windows (Cygwin)'
     AC_MSG_RESULT([[$mhd_host_os]])
     os_is_windows=yes],
  [*mingw*],
  [AC_DEFINE_UNQUOTED(MINGW,1,[This is a MinGW system])
     AC_DEFINE_UNQUOTED(WINDOWS,1,[This is a Windows system])
     mhd_host_os='Windows (MinGW)'
     AC_MSG_RESULT([[$mhd_host_os]])
     LIBS="$LIBS -lws2_32"
     AC_CHECK_HEADERS([winsock2.h ws2tcpip.h], [], [AC_MSG_ERROR([[Winsock2 headers are required for W32]])], [AC_INCLUDES_DEFAULT])
     AC_CACHE_CHECK([for MS lib utility], [ac_cv_use_ms_lib_tool],
       [mslibcheck=`lib 2>&1`
        AS_IF([echo "$mslibcheck" | $GREP -e '^Microsoft (R) Library Manager' - >/dev/null],
          [ac_cv_use_ms_lib_tool=yes],
          [ac_cv_use_ms_lib_tool=no])
         ])
     AS_IF([test "x$ac_cv_use_ms_lib_tool" = "xyes"],
       [AC_SUBST([MS_LIB_TOOL], [[lib]])])
     AC_SUBST([lt_cv_objdir])
     os_is_windows=yes
     os_is_native_w32=yes
   ],
   [*openedition*],
   [AC_DEFINE_UNQUOTED(OS390,1,[This is a OS/390 system])
     mhd_host_os='OS/390'
     AC_MSG_RESULT([[$mhd_host_os]])],
   [gnu*],
   [AC_DEFINE([[GNU_HURD]], [[1]], [Define to `1' if host machine runs on GNU Hurd.])
     mhd_host_os='GNU Hurd'
     AC_MSG_RESULT([[$mhd_host_os]])
    ],
    [
     mhd_host_os='unrecognised OS'
     AC_MSG_RESULT([[$mhd_host_os]])
     AC_MSG_WARN([Unrecognised OS $host_os])
     AC_DEFINE_UNQUOTED(OTHEROS,1,[Some strange OS])
 ])

AM_CONDITIONAL([CYGWIN_TARGET], [[test "x$os_is_windows" = "xyes" && \
                                  test "x${os_is_native_w32}" != "xyes"]])

AC_ARG_WITH([threads],
   [AS_HELP_STRING([--with-threads=LIB],[choose threading library (posix, w32, auto, none) [auto]])],
   [], [with_threads='auto'])
AS_CASE([[$with_threads]],
  [[win32]], [[with_threads='w32']],
  [[pthreads]], [[with_threads='posix']],
  [[posix]], [[:]],
  [[w32]], [[:]],
  [[none]], [[with_threads='none']],
  [[auto]], [[:]],
    [AC_MSG_ERROR([[incorrect parameter "$with_threads" specified for --with-threads]])]
)

# Check for posix threads support, regardless of configure parameters as
# testsuite use only posix threads.
AX_PTHREAD(
  [
    mhd_have_posix_threads='yes'
    AC_DEFINE([[HAVE_PTHREAD_H]],[[1]],[Define to 1 if you have the <pthread.h> header file.])
  ],[[mhd_have_posix_threads='no']])
AM_CONDITIONAL([HAVE_POSIX_THREADS],[test "x$mhd_have_posix_threads" = "xyes"])

mhd_have_w32_threads='no'
AS_IF([[test "x$with_threads" = "xauto"]],
 [
 AS_IF([[test "x$os_is_windows" = "xyes"]],
   [
    AC_MSG_CHECKING([[for W32 threads]])
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <windows.h>
         ]], [ HANDLE h = CreateThread(NULL, 0, NULL, NULL, 0, NULL);])]
      , [[mhd_have_w32_threads='yes']], [[mhd_have_w32_threads='no']]
      )
    AC_MSG_RESULT([[$mhd_have_w32_threads]])
   ])
 ]
)

AC_MSG_CHECKING([[for threading lib to use with libmicrohttpd ($with_threads)]])
AS_IF([test "x$with_threads" = "xposix"],
  [ # forced posix threads
    AS_IF([test "x$mhd_have_posix_threads" = "xyes"], [USE_THREADS='posix'],
      [ AS_IF([[test "x$os_is_windows" = "xyes"]] ,
          [ AC_MSG_ERROR([[Posix threads are not available. Try to configure --with-threads=auto]])],
          [ AC_MSG_ERROR([[No threading lib is available. Consider installing pthreads]])] )
      ])
    ])
AS_IF([test "x$with_threads" = "xw32"],
  [ # forced w32 threads
    AS_IF([[test "x$mhd_have_w32_threads" = "xyes"]],
      [[ USE_THREADS='w32' ]],
      [ AC_MSG_ERROR([[W32 threads are not available. Try to configure --with-threads=auto]])])
    ])
AS_IF([test "x$with_threads" = "xauto"],
      [# automatic threads lib selection
       AS_IF([[test "x$os_is_native_w32" = "xyes" && test "x$mhd_have_w32_threads" = "xyes"]] ,
        [[ USE_THREADS='w32' ]] ,
        [[ test "x$mhd_have_posix_threads" = "xyes" ]], [[ USE_THREADS='posix' ]],
        [[ test "x$mhd_have_w32_threads" = "xyes" ]], [[ USE_THREADS='w32' ]],
        [ AC_MSG_ERROR([[No threading lib is available. Consider installing pthreads]]) ]
        )])
AS_IF([test "x$with_threads" = "xnone"],
   [USE_THREADS='none'])

AS_IF([test "x$USE_THREADS" = "xposix"],
  [CC="$PTHREAD_CC"
  AC_DEFINE([MHD_USE_POSIX_THREADS],[1],[define to use pthreads])
  MHD_LIB_CFLAGS="$MHD_LIB_CFLAGS $PTHREAD_CFLAGS"
  MHD_LIBDEPS="$PTHREAD_LIBS $MHD_LIBDEPS"
  MHD_LIBDEPS_PKGCFG="$PTHREAD_LIBS $MHD_LIBDEPS_PKGCFG"],
  [AS_IF([test "x$USE_THREADS" = "xw32"],
   [AC_DEFINE([MHD_USE_W32_THREADS],[1],[define to use W32 threads])])])
AM_CONDITIONAL([USE_POSIX_THREADS], [test "x$USE_THREADS" = "xposix"])
AM_CONDITIONAL([USE_W32_THREADS], [test "x$USE_THREADS" = "xw32"])
AM_CONDITIONAL([DISABLE_THREADS], [test "x$USE_THREADS" = "xnone"])
AC_MSG_RESULT([$USE_THREADS])

AC_ARG_ENABLE([[thread-names]],
   [AS_HELP_STRING([--disable-thread-names [auto] ],[do not set names on MHD generated threads])],
   [], [enable_thread_names='auto'])

AS_IF([test "x$enable_thread_names" != "xno" && test "x$USE_THREADS" = "xposix"],[
  # Check for thread name function
  HAVE_THREAD_NAME_FUNC="no"
  SAVE_LIBS="$LIBS"
  SAVE_CFLAGS="$CFLAGS"
  LIBS="$PTHREAD_LIBS $LIBS"
  CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
  AC_CHECK_HEADERS([pthread_np.h],[],[],
    [
AC_INCLUDES_DEFAULT
      [
#include <pthread.h>
      ]
    ])

  # Try to find how to set thread name by thread attributes.
  # If pthread_attr_setname_np(3) is not declared, it's not possible to detect
  # form of pthread_attr_setname_np(3) due to C "feature" "implicit declaration".
  AC_CHECK_DECL([[pthread_attr_setname_np]],[],[],[[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]])

  AS_IF([[test "x$ac_cv_have_decl_pthread_attr_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_attr_setname_np(3) in NetBSD or OSF1 form]])
     AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[
      pthread_attr_t thr_attr;
      pthread_attr_init(&thr_attr);
      pthread_attr_setname_np(&thr_attr, "name", 0);
      pthread_attr_destroy(&thr_attr);
        ]])],
        [AC_DEFINE([[HAVE_PTHREAD_ATTR_SETNAME_NP_NETBSD]], [[1]], [Define if you have NetBSD form (or OSF1 form) of pthread_attr_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_attr_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_attr_setname_np(3) in IBM i form]])
     AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[
      pthread_attr_t thr_attr;
      pthread_attr_init(&thr_attr);
      pthread_attr_setname_np(&thr_attr, "name");
      pthread_attr_destroy(&thr_attr);
        ]])],
        [AC_DEFINE([[HAVE_PTHREAD_ATTR_SETNAME_NP_IBMI]], [[1]], [Define if you have IBM i form of pthread_attr_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  # Try to find how to set thread name for started thread - less convinent
  # than setting name by attributes.
  # If pthread_setname_np(3) is not declared, it's not possible to detect
  # form of pthread_setname_np(3) due to C "feature" "implicit declaration".
  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes"]],
    [AC_CHECK_DECL([[pthread_setname_np]],[],[],[[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
       ]])
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_setname_np(3) in NetBSD or OSF1 form]])
     AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[int res = pthread_setname_np(pthread_self(), "name", 0);]])],
        [AC_DEFINE([[HAVE_PTHREAD_SETNAME_NP_NETBSD]], [[1]], [Define if you have NetBSD form (or OSF1 form) of pthread_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_setname_np(3) in GNU/Linux form]])
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[int res = pthread_setname_np(pthread_self(), "name");]])],
        [AC_DEFINE([[HAVE_PTHREAD_SETNAME_NP_GNU]], [[1]], [Define if you have GNU/Linux form of pthread_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_setname_np(3) in Darwin form]])
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[int res = pthread_setname_np("name");]])],
        [AC_DEFINE([[HAVE_PTHREAD_SETNAME_NP_DARWIN]], [[1]], [Define if you have Darwin form of pthread_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes"]],
    [
     AC_CHECK_DECL([[pthread_set_name_np]],
       [
        AC_MSG_CHECKING([[for pthread_set_name_np(3) in FreeBSD form]])
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[pthread_set_name_np(pthread_self(), "name");]])],
          [AC_DEFINE([[HAVE_PTHREAD_SET_NAME_NP_FREEBSD]], [[1]], [Define if you have FreeBSD form of pthread_set_name_np(3) function.])
           HAVE_THREAD_NAME_FUNC="yes"
           AC_MSG_RESULT([[yes]])],
          [AC_MSG_RESULT([[no]])]
          )
       ],[],[[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
       ]]
     )
    ])

  LIBS="$SAVE_LIBS"
  CFLAGS="$SAVE_CFLAGS"
])

AS_IF([[test "x$enable_thread_names" != "xno"]],
  [
    AC_MSG_CHECKING([[whether to enable thread names]])
    AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([], [[
#ifdef MHD_NO_THREAD_NAMES
#error Thread names are disabled.
choke me
#endif

/* Keep in sync with mhd_threads.h */
#if defined(MHD_USE_POSIX_THREADS) && (defined(HAVE_PTHREAD_ATTR_SETNAME_NP_NETBSD) || defined(HAVE_PTHREAD_ATTR_SETNAME_NP_IBMI) || \
    defined(HAVE_PTHREAD_SETNAME_NP_GNU) || defined(HAVE_PTHREAD_SET_NAME_NP_FREEBSD) || defined(HAVE_PTHREAD_SETNAME_NP_DARWIN) || \
    defined(HAVE_PTHREAD_SETNAME_NP_NETBSD) )
int a = 1;
#elif defined(MHD_USE_W32_THREADS) && defined(_MSC_FULL_VER)
int b = 2;
#else
#error No thread name function is available.
choke me
#endif
       ]])
     ], [
       enable_thread_names='yes'
     ], [
       AS_IF([[test "x$enable_thread_names" = "xyes"]],
         [
           AC_MSG_RESULT([[no]])
           AC_MSG_ERROR([[thread names was explicitly requested, but thread name function is not available]])
         ])
       enable_thread_names='no'
     ])
  AC_MSG_RESULT([[$enable_thread_names]])
  ])

AS_IF([[test "x$enable_thread_names" = "xno"]],
  [AC_DEFINE([[MHD_NO_THREAD_NAMES]], [[1]], [Define to 1 to disable setting name on generated threads])])

AM_CONDITIONAL(HAVE_W32, [test "x$os_is_native_w32" = "xyes"])
w32_shared_lib_exp=no
if test "x$enable_shared" = "xyes" && test "x$os_is_native_w32" = "xyes"; then
  if test "x$ac_cv_use_ms_lib_tool" = "xyes" || test -n "$DLLTOOL"; then
    w32_shared_lib_exp=yes
  else
    AC_MSG_WARN([[GNU dlltool or MS lib.exe is required for creating shared library export on W32]])
    AC_MSG_WARN([[Export library libmicrohttpd.lib will not be created]])
  fi
fi
AM_CONDITIONAL(W32_SHARED_LIB_EXP, [test "x$w32_shared_lib_exp" = "xyes"])
AM_CONDITIONAL(USE_MS_LIB_TOOL, [test "x$ac_cv_use_ms_lib_tool" = "xyes"])

MHD_CHECK_SOCKET_SHUTDOWN_TRIGGER([AC_DEFINE([HAVE_LISTEN_SHUTDOWN],[1],[can use shutdown on listen sockets])])
AM_CONDITIONAL([HAVE_LISTEN_SHUTDOWN], [test "x$mhd_cv_host_shtdwn_trgr_select" = "xyes"])

# set GCC options
# use '-fno-strict-aliasing', but only if the compiler
# and linker can take it
AX_CHECK_LINK_FLAG([-fno-strict-aliasing],
  [AX_APPEND_COMPILE_FLAGS([-fno-strict-aliasing])])

AC_C_BIGENDIAN
AC_C_VARARRAYS
AC_CACHE_CHECK([[whether __builtin_bswap32() is available]],
  [[mhd_cv_func___builtin_bswap32_avail]], [dnl
  AC_TRY_LINK([#include<stdint.h>],[uint32_t a = 1; uint32_t b = __builtin_bswap32(a); a = b;],
    [[mhd_cv_func___builtin_bswap32_avail="yes"]], [[mhd_cv_func___builtin_bswap32_avail="no"]])
])
AS_IF([[test "x$mhd_cv_func___builtin_bswap32_avail" = "xyes"]],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_BSWAP32]], [[1]], [Define to 1 if you have __builtin_bswap32() builtin function])])
AC_CACHE_CHECK([[whether __builtin_bswap64() is available]],
  [[mhd_cv_func___builtin_bswap64_avail]], [dnl
  AC_TRY_LINK([#include<stdint.h>],[uint64_t a = 1; uint32_t b = __builtin_bswap64(a); a = b;],
    [[mhd_cv_func___builtin_bswap64_avail="yes"]], [[mhd_cv_func___builtin_bswap64_avail="no"]])
])
AS_IF([[test "x$mhd_cv_func___builtin_bswap64_avail" = "xyes"]],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_BSWAP64]], [[1]], [Define to 1 if you have __builtin_bswap64() builtin function])])

AC_CHECK_PROG([HAVE_CURL_BINARY],[curl],[yes],[no])
AM_CONDITIONAL([HAVE_CURL_BINARY],[test "x$HAVE_CURL_BINARY" = "xyes"])
AC_CHECK_PROG([HAVE_MAKEINFO_BINARY],[makeinfo],[yes],[no])
AM_CONDITIONAL([HAVE_MAKEINFO_BINARY],[test "x$HAVE_MAKEINFO_BINARY" = "xyes"])
AM_CONDITIONAL(W32_STATIC_LIB, [test "x$os_is_native_w32" = "xyes"  && test "x$enable_static" = "xyes"])


AC_ARG_ENABLE([[doc]],
  [AS_HELP_STRING([[--disable-doc]], [do not build any documentation])], ,
    [enable_doc=yes])
test "x$enable_doc" = "xno" || enable_doc=yes
AM_CONDITIONAL([BUILD_DOC], [test "x$enable_doc" = "xyes"])

AC_ARG_ENABLE([[examples]],
  [AS_HELP_STRING([[--disable-examples]], [do not build any examples])], ,
    [enable_examples=yes])
test "x$enable_examples" = "xno" || enable_examples=yes
AM_CONDITIONAL([BUILD_EXAMPLES], [test "x$enable_examples" = "xyes"])

AC_ARG_ENABLE([[poll]],
  [AS_HELP_STRING([[--enable-poll[=ARG]]], [enable poll support (yes, no, auto) [auto]])],
    [enable_poll=${enableval}],
    [enable_poll='auto']
  )

AS_IF([test "$enable_poll" != "no"],
       [AS_IF([test "$os_is_native_w32" != "yes"],
             AC_CHECK_HEADERS([poll.h],
                [AC_CHECK_FUNCS([poll], [have_poll='yes'], [have_poll='no'])],
		[],
		[AC_INCLUDES_DEFAULT]),
             [AC_MSG_CHECKING([for WSAPoll()])
              AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#if defined(_WIN32) && ! defined(__CYGWIN__) && ! defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
              ]],[[
WSAPOLLFD fda[2];
WSAPoll(fda, 2, 0);]])],
              [have_poll='yes'
              AC_DEFINE([HAVE_POLL],[1])
              ],[have_poll='no'])
              AC_MSG_RESULT([$have_poll])])
       AS_IF([test "$enable_poll" = "yes" && test "$have_poll" != "yes"],
             AC_MSG_ERROR([[Support for poll was explicitly requested but cannot be enabled on this platform.]]))
       enable_poll="$have_poll"])

AC_ARG_ENABLE([[epoll]],
  [AS_HELP_STRING([[--enable-epoll[=ARG]]], [enable epoll support (yes, no, auto) [auto]])],
    [enable_epoll=${enableval}],
    [enable_epoll='auto']
  )

AS_IF([test "$enable_epoll" != "no"],
      [AX_HAVE_EPOLL
       AS_IF([test "${ax_cv_have_epoll}" = "yes"],
          [AC_DEFINE([[EPOLL_SUPPORT]],[[1]],[Define to 1 to enable epoll support])
           enable_epoll='yes'],
       [AS_IF([test "$enable_epoll" = "yes"],
              AC_MSG_ERROR([[Support for epoll was explicitly requested but cannot be enabled on this platform.]]))
        enable_epoll='no'])])

AM_CONDITIONAL([MHD_HAVE_EPOLL], [[test "x$enable_epoll" = xyes]])

AS_IF([test "x$enable_epoll" = "xyes"],
  AC_CACHE_CHECK([for epoll_create1()],
                 [mhd_cv_have_epoll_create1], [
    AC_LINK_IFELSE([
      AC_LANG_PROGRAM([[
#include <sys/epoll.h>
        ]], [[
int fd;
fd = epoll_create1(EPOLL_CLOEXEC);]])],
      [mhd_cv_have_epoll_create1=yes],
      [mhd_cv_have_epoll_create1=no])])
  AS_IF([test "x$mhd_cv_have_epoll_create1" = "xyes"],[
    AC_DEFINE([[HAVE_EPOLL_CREATE1]], [[1]], [Define if you have epoll_create1 function.])]))

# Check for headers that are ALWAYS required
AC_CHECK_HEADERS_ONCE([fcntl.h math.h errno.h limits.h stdio.h locale.h sys/stat.h sys/types.h], [], [AC_MSG_ERROR([Compiling libmicrohttpd requires standard UNIX headers files])], [AC_INCLUDES_DEFAULT])

# Check for optional headers
AC_CHECK_HEADERS([sys/types.h sys/time.h sys/msg.h time.h sys/mman.h sys/ioctl.h \
  sys/socket.h sys/select.h netdb.h netinet/in.h netinet/ip.h netinet/tcp.h arpa/inet.h \
  endian.h machine/endian.h sys/endian.h sys/param.h sys/machine.h sys/byteorder.h machine/param.h sys/isa_defs.h \
  inttypes.h stddef.h unistd.h \
  sockLib.h inetLib.h net/if.h], [], [], [AC_INCLUDES_DEFAULT])

AC_CHECK_HEADER([[search.h]],
  [
   gl_FUNC_TSEARCH
   AS_IF([[test "x$HAVE_TSEARCH" = "x1" && test "x$REPLACE_TSEARCH" != "x1"]],
     [AC_DEFINE([[HAVE_SEARCH_H]], [[1]],
       [Define to 1 if you have the <search.h> header file and your system have properly functioning tsearch(), tfind() and tdelete() functions])])
  ],
  [], [AC_INCLUDES_DEFAULT])

AM_CONDITIONAL([MHD_HAVE_TSEARCH], [[test "x$ac_cv_header_search_h" = xyes && test "x$HAVE_TSEARCH" = "x1" && test "x$REPLACE_TSEARCH" != "x1"]])

AC_CHECK_HEADERS([dlfcn.h],[have_tlsplugin=yes],[have_tlsplugin=no], [AC_INCLUDES_DEFAULT])
AM_CONDITIONAL([MHD_HAVE_TLS_PLUGIN], [[test "x$have_tlsplugin" = xyes]])

AC_CHECK_HEADERS([zlib.h],[have_zlib=yes],[have_zlib=no], [AC_INCLUDES_DEFAULT])
AM_CONDITIONAL([HAVE_ZLIB], [[test "x$have_zlib" = xyes]])

# Check for generic functions
MHD_CHECK_FUNC([random],
  [
AC_INCLUDES_DEFAULT
[#include <stdlib.h>
  ]],
  [[long int r = random(); (void)r;]],
  [],
  [
    MHD_CHECK_FUNC([rand],
      [
AC_INCLUDES_DEFAULT
[#include <stdlib.h>
      ]],
      [[int r = rand(); (void)r;]],
	)
  ]
)

AC_CHECK_MEMBERS([struct sockaddr_in.sin_len, struct sockaddr_in6.sin6_len, 
                  struct sockaddr_storage.ss_len],
   [], [],
   [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
   ])

MHD_CHECK_FUNC([getsockname],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
  ]],
  [
    struct sockaddr_storage ss;
    (void)getsockname(socket(0,0,0),(struct sockaddr *)&ss,(void*)0);
  ],
  [
    AC_CACHE_CHECK([[whether getsockname() is usable]], [[mhc_cv_getsockname_usable]],
      [
         AC_RUN_IFELSE(
           [
            AC_LANG_SOURCE(
             [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif


static void zr_mem(void *ptr, socklen_t size)
{ char *mem = ptr; while(size--) {mem[0] = 0; mem++;} }

int main(void)
{
  const socklen_t c_addr_size = (socklen_t)sizeof(struct sockaddr_in);
  struct sockaddr_in sa;
  socklen_t addr_size;
  int ret = 1;
#if !defined(_WIN32) || defined(__CYGWIN__)
  int sckt;
  const int invld_sckt = -1;
#else
  SOCKET sckt;
  const SOCKET invld_sckt = INVALID_SOCKET;
  WSADATA wsa_data;

  if (0 != WSAStartup(MAKEWORD(2, 2), &wsa_data) || MAKEWORD(2, 2) != wsa_data.wVersion)
    return 20;
#endif

  sckt = socket (PF_INET, SOCK_STREAM, 0);
  if (invld_sckt != sckt)
  {
    zr_mem(&sa, c_addr_size);
    sa.sin_family = AF_INET;
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    sa.sin_len = c_addr_size;
#endif
    if (0 == bind (sckt, (struct sockaddr *)&sa, c_addr_size))
    {
      if (0 == listen (sckt, 1))
      {
        addr_size = c_addr_size;
        if (0 == getsockname (sckt, (struct sockaddr  *)&sa, &addr_size))
        {
          if (c_addr_size >= addr_size)
          {
            if (0 != ntohs(sa.sin_port))
            { ret = 0;
            } else ret = 7;
          } else ret = 6;
        } else ret = 5;
      } else ret = 4;
    } else ret = 3;
  } else ret = 2;
#if !defined(_WIN32) || defined(__CYGWIN__)
  close (sckt);
#else
  closesocket (sckt);
  WSACleanup();
#endif
  return ret;
}
             ]]
            )
           ],
           [[mhc_cv_getsockname_usable='yes']],
           [[mhc_cv_getsockname_usable='no']],
           [[mhc_cv_getsockname_usable='assuming yes']]
         )
        ]
      )
    AS_VAR_IF([[mhc_cv_getsockname_usable]], [["no"]], [:],
        [AC_DEFINE([[MHD_USE_GETSOCKNAME]], [[1]], [Define if you have usable `getsockname' function.])])
  ]
)

# Check for inter-thread signaling type
AC_ARG_ENABLE([[itc]],
  [AS_HELP_STRING([[--enable-itc=TYPE]], [use TYPE of inter-thread communication (pipe, socketpair, eventfd) [auto]])], [],
  [[enable_itc='auto']]
)

AS_CASE([[$enable_itc]],
  [[pipe]], [[:]],
  [[socketpair]], [[:]],
  [[eventfd]], [[:]],
  [[auto]], [AS_VAR_IF([[os_is_windows]], [["yes"]], [[enable_itc='socketpair']])],
  [[eventFD]], [[enable_itc='eventfd']],
  [[socket]], [[enable_itc='socketpair']],
  [[no]], [AC_MSG_ERROR([[inter-thread communication cannot be disabled]])],
    [AC_MSG_ERROR([[unrecognized type "$enable_itc" of inter-thread communication specified by "--enable-itc=$enable_itc"]])]
)
# AS_UNSET([[use_itc]])

AS_IF([[test "x$enable_itc" = "xeventfd" || test "x$enable_itc" = "xauto"]], [
  AS_VAR_IF([[os_is_native_w32]], [["yes"]], [], [
    AC_CHECK_HEADERS([sys/eventfd.h], [], [], [AC_INCLUDES_DEFAULT])
    AS_VAR_IF([[ac_cv_header_sys_eventfd_h]], [["yes"]], [
      AC_CACHE_CHECK([whether eventfd(2) is usable], [[mhd_cv_eventfd_usable]], [
        AC_LINK_IFELSE([
          AC_LANG_PROGRAM([[
#include <sys/eventfd.h>
          ]], [[int ef = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)
          ]])
        ], [[mhd_cv_eventfd_usable='yes']], [[mhd_cv_eventfd_usable='no']])
      ])
    ])
  ])
  AS_VAR_IF([[mhd_cv_eventfd_usable]], [["yes"]], [
    use_itc='eventfd'
    enable_itc="$use_itc"
    AC_DEFINE([[_MHD_ITC_EVENTFD]], [[1]], [Define to use eventFD for inter-thread communication])
  ], [
    AS_VAR_IF([[enable_itc]], [["eventfd"]], [AC_MSG_ERROR([[eventfd(2) is not usable, consider using other type of inter-thread communication]])])
  ])
])

AS_IF([[test "x$enable_itc" = "xpipe" || test "x$enable_itc" = "xauto"]], [
  AS_VAR_IF([[os_is_native_w32]], [["yes"]], [], [
    AC_CACHE_CHECK([[whether pipe(3) is usable]], [[mhd_cv_pipe_usable]], [
      AC_LINK_IFELSE([
        AC_LANG_PROGRAM([
AC_INCLUDES_DEFAULT
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
        ], [[
          int arr[2];
          int res;
          res = pipe(arr)
        ]])
      ], [[mhd_cv_pipe_usable='yes']], [[mhd_cv_pipe_usable='no']])
    ])
  ])
  AS_VAR_IF([[mhd_cv_pipe_usable]], [["yes"]], [
    use_itc='pipe'
    enable_itc="$use_itc"
    AC_DEFINE([[_MHD_ITC_PIPE]], [[1]], [Define to use pipe for inter-thread communication])
    AC_CACHE_CHECK([[whether pipe2(2) is usable]], [[mhd_cv_pipe2_usable]], [
      AC_LINK_IFELSE([
        AC_LANG_PROGRAM([
AC_INCLUDES_DEFAULT
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
        ], [[
          int arr[2];
          int res;
          res = pipe2(arr, O_CLOEXEC | O_NONBLOCK)
        ]])
      ], [[mhd_cv_pipe2_usable='yes']], [[mhd_cv_pipe2_usable='no']])
    ])
    AS_VAR_IF([[mhd_cv_pipe2_usable]], [["yes"]],
      [AC_DEFINE([[HAVE_PIPE2_FUNC]], [[1]], [Define if you have usable pipe2(2) function])])
  ], [
    AS_VAR_IF([[enable_itc]], [["pipe"]], [AC_MSG_ERROR([[pipe(3) is not usable, consider using other type of inter-thread communication]])])
  ])
])

AS_IF([[test "x$enable_itc" = "xsocketpair" || test "x$enable_itc" = "xauto"]], [
  AS_VAR_IF([[os_is_native_w32]], [["yes"]], [[mhd_cv_socketpair_usable='yes']], [
    AC_CACHE_CHECK([[whether socketpair(3) is usable]], [[mhd_cv_socketpair_usable]], [
      AC_LINK_IFELSE([
        AC_LANG_PROGRAM([
AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
        ], [[
          int arr[2];
          int res;
#if defined(AF_LOCAL)
          res = socketpair(AF_LOCAL, SOCK_STREAM, 0, arr);
#elif defined(AF_UNIX)
          res = socketpair(AF_UNIX, SOCK_STREAM, 0, arr);
#else
#error AF_LOCAL and AF_UNIX are both undefined
          choke me now;
#endif
          if (res != 0) return 1
        ]])
      ], [[mhd_cv_socketpair_usable='yes']], [[mhd_cv_socketpair_usable='no']])
    ])
  ])
  AS_VAR_IF([[mhd_cv_socketpair_usable]], [["yes"]], [
    use_itc='socketpair'
    enable_itc="$use_itc"
    AC_DEFINE([[_MHD_ITC_SOCKETPAIR]], [[1]], [Define to use socketpair for inter-thread communication])
  ], [
    AS_VAR_IF([[enable_itc]], [["socketpair"]], [AC_MSG_ERROR([[socketpair(3) is not usable, consider using other type of inter-thread communication]])])
  ])
])

AS_IF([[test -z "$use_itc"]], [AC_MSG_ERROR([[cannot find useable type of inter-thread communication]])])


AC_CHECK_FUNCS_ONCE([accept4 gmtime_r memmem snprintf])
AC_CHECK_DECL([gmtime_s],
  [
    AC_MSG_CHECKING([[whether gmtime_s is in C11 form]])
    AC_LINK_IFELSE(
        [ AC_LANG_PROGRAM(
          [[
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#ifdef __cplusplus
extern "C"
#endif
             struct tm* gmtime_s(const time_t* time, struct tm* result);
           ]], [[
             struct tm res;
             time_t t;
             gmtime_s (&t, &res);
          ]])
        ],
        [
          AC_DEFINE([HAVE_C11_GMTIME_S], [1], [Define to 1 if you have the `gmtime_s' function in C11 form.])
          AC_MSG_RESULT([[yes]])
        ],
        [
          AC_MSG_RESULT([[no]])
          AC_MSG_CHECKING([[whether gmtime_s is in W32 form]])
          AC_LINK_IFELSE(
            [ AC_LANG_PROGRAM(
              [[
#include <time.h>
#ifdef __cplusplus
extern "C"
#endif
errno_t gmtime_s(struct tm* _tm, const time_t* time);
              ]], [[
                 struct tm res;
                 time_t t;
                 gmtime_s (&res, &t);
              ]])
            ],
            [
              AC_DEFINE([HAVE_W32_GMTIME_S], [1], [Define to 1 if you have the `gmtime_s' function in W32 form.])
              AC_MSG_RESULT([[yes]])
            ],
            [AC_MSG_RESULT([[no]])
            ])
        ])
  ], [],
  [[#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>]])


AC_CHECK_DECLS([SOCK_NONBLOCK], [AC_DEFINE([HAVE_SOCK_NONBLOCK], [1], [SOCK_NONBLOCK is defined in a socket header])], [],
                   [
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#if defined(HAVE_SYS_SOCKET_H)
#  include <sys/socket.h>
#elif defined(HAVE_WINSOCK2_H)
#  include <winsock2.h>
#endif
                   ])


AC_CHECK_DECL([[clock_gettime]],
  [
   SAVE_LIBS="$LIBS"
   AC_SEARCH_LIBS([clock_gettime], [rt],
     [
      AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [Have clock_gettime])
      AS_VAR_IF([[ac_cv_search_clock_gettime]],[["none required"]], [],
        [
         MHD_LIBDEPS="$ac_cv_search_clock_gettime $MHD_LIBDEPS"
         MHD_LIBDEPS_PKGCFG="$ac_cv_search_clock_gettime $MHD_LIBDEPS_PKGCFG"
        ])
     ], [], [$MHD_LIBDEPS])
   LIBS="$SAVE_LIBS"
  ], [], [[
#include <time.h>
  ]])

AC_MSG_CHECKING([[for clock_get_time]])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
    [[
#include <mach/clock.h>
#include <mach/mach.h>
    ]],
    [[
      clock_serv_t cs;
      mach_timespec_t mt;
      host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cs);
      clock_get_time(cs, &mt);
      mach_port_deallocate(mach_task_self(), cs);
    ]])
  ],
  [
    AC_DEFINE([HAVE_CLOCK_GET_TIME], [1], [Define to 1 if you have `clock_get_time', `host_get_clock_service' and `mach_port_deallocate' functions.])
    AC_MSG_RESULT([[yes]])
  ],
  [AC_MSG_RESULT([[no]])
  ])

AC_MSG_CHECKING([[for gethrtime]])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
    [[
#ifdef HAVE_SYS_TIME_H
/* Solaris define gethrtime() in sys/time.h */
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_TIME_H
/* HP-UX define gethrtime() in time.h */
#include <time.h>
#endif /* HAVE_TIME_H */
    ]], [[hrtime_t hrt = gethrtime(); ]])
  ],
  [
    AC_DEFINE([HAVE_GETHRTIME], [1], [Define to 1 if you have `gethrtime' function.])
    AC_MSG_RESULT([[yes]])
  ],
  [AC_MSG_RESULT([[no]])
  ])

# IPv6
AC_MSG_CHECKING(for IPv6)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#if HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
]], [[
int af=AF_INET6;
int pf=PF_INET6;
struct sockaddr_in6 sa;
printf("%d %d %p\n", af, pf, &sa);
]])],[
have_inet6=yes;
AC_DEFINE([HAVE_INET6], [1], [Provides IPv6 headers])
],[
have_inet6=no
])
AC_MSG_RESULT($have_inet6)

HIDDEN_VISIBILITY_CFLAGS=""
AS_CASE(["$host"],
  [*-*-mingw*],[
    dnl on mingw32 we do -fvisibility=hidden and __declspec(dllexport)
    AC_DEFINE([_MHD_EXTERN], [__attribute__((visibility("default"))) __declspec(dllexport) extern],
              [defines how to decorate public symbols while building])
    HIDDEN_VISIBILITY_CFLAGS="-fvisibility=hidden"
  ],[
    dnl on other compilers, check if we can do -fvisibility=hidden
    AX_CHECK_LINK_FLAG([-fvisibility=hidden],
      [AX_CHECK_COMPILE_FLAG([-fvisibility=hidden],
                           [AC_DEFINE([_MHD_EXTERN], [__attribute__((visibility("default"))) extern],
                                       [defines how to decorate public symbols while building])
                            HIDDEN_VISIBILITY_CFLAGS="-fvisibility=hidden"])])
   ])
AC_SUBST(HIDDEN_VISIBILITY_CFLAGS)

# libcurl (required for testing)
AC_ARG_ENABLE([curl],
  [AS_HELP_STRING([--disable-curl],[disable cURL based testcases])],
  [enable_curl=${enableval}])
curl=0
AS_IF([test "$enable_curl" != "no"],
 [LIBCURL_CHECK_CONFIG([yes],[7.16.4],[enable_curl=yes],
    [
      AS_IF([test "x$enable_curl" = "xyes"],
       [AC_MSG_WARN([[cURL-based tests cannot be enabled because libcurl is missing]])])
      enable_curl=no
    ])
])
AS_IF([test "$enable_curl" != "no"],
 [
# Lib cURL & cURL - OpenSSL versions
  AC_DEFINE([MHD_REQ_CURL_VERSION], ["7.16.4"], [required cURL version to run tests])
  AC_DEFINE([MHD_REQ_CURL_OPENSSL_VERSION], ["0.9.8"], [required cURL SSL version to run tests])
  AC_DEFINE([MHD_REQ_CURL_GNUTLS_VERSION], ["2.8.6"], [gnuTLS lib version - used in conjunction with cURL])
  AC_DEFINE([MHD_REQ_CURL_NSS_VERSION], ["3.12.0"], [NSS lib version - used in conjunction with cURL])
 ])
AM_CONDITIONAL([HAVE_CURL], [test "x$enable_curl" = "xyes"])

mhd_have_libmagic="no"
SAVE_LIBS="$LIBS"
LIBS="$LIBS -lmagic"
AC_MSG_CHECKING([[for suitable libmagic]])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
    [[
#include <magic.h>
    ]],
    [[
      char var_data[256];
      const char *var_mime;
      magic_t var_magic = magic_open (MAGIC_MIME_TYPE);
      (void)magic_load (var_magic, NULL);
      var_data[0] = 0;
      var_mime = magic_buffer (var_magic, var_data, 1);
      magic_close (var_magic);
    ]]
   )
  ],
  [
    AC_DEFINE([HAVE_LIBMAGIC], [1], [Define to 1 if you have suitable libmagic.])
    mhd_have_libmagic="yes"
    AC_MSG_RESULT([[yes]])
  ],
  [AC_MSG_RESULT([[no]])
  ]
)
LIBS="$SAVE_LIBS"
AM_CONDITIONAL([HAVE_LIBMAGIC], [[test "x$mhd_have_libmagic" = "xyes"]])

# large file support (> 4 GB)
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
AC_CHECK_FUNCS([lseek64 pread64 pread])

# check for various sendfile functions
AC_ARG_ENABLE([sendfile],
   [AS_HELP_STRING([--disable-sendfile],
               [disable usage of sendfile() for HTTP connections [auto]])],
   [],
   [enable_sendfile="auto"])
AS_CASE([$enable_sendfile],
  [[auto | yes]],[[found_sendfile="no"]],
  [[no]],[[found_sendfile="disabled"]],
  [AC_MSG_ERROR([[unknown value specified: --enable-sendfile=$enable_sendfile]])]
)
AS_VAR_IF([[found_sendfile]], [["no"]],
  [
    AC_MSG_CHECKING([[for Linux-style sendfile(2)]])
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[
#include <sys/sendfile.h>

static void empty_func(void)
{
/* Check for declaration */
  (void)sendfile;
}
/* Declare again to check form match */
ssize_t sendfile(int, int, off_t*, size_t);
        ]],
        [[
          int fd1=0, fd2=2;
          off_t o = 0;
          size_t s = 5;
          ssize_t r;
          r = sendfile (fd1, fd2, &o, s);
          if (r)
            empty_func();
        ]]
       )
      ],
      [
        AC_DEFINE([HAVE_LINUX_SENDFILE], [1], [Define to 1 if you have linux-style sendfile(2).])
        found_sendfile="yes, Linux-style"
        AC_MSG_RESULT([[yes]])
        AC_CHECK_FUNCS([sendfile64])
      ],
      [AC_MSG_RESULT([[no]])
      ]
    )
  ]
)
AS_VAR_IF([[found_sendfile]], [["no"]],
  [
   AC_MSG_CHECKING([[for FreeBSD-style sendfile(2)]])
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
       [[
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

static void empty_func(void)
{
/* Check for declaration */
  (void)sendfile;
}
/* Declare again to check form match */
int sendfile(int, int, off_t, size_t,
             struct sf_hdtr*, off_t*, int);
       ]],
       [[
         int fd1=0, fd2=1;
         off_t o = 0;
         size_t s = 5;
         off_t r1;
         int r2;
         r2 = sendfile (fd1, fd2, o, s, (void*)0, &r1, 0);
         if (r2)
           empty_func();
       ]]
      )
     ],
     [
       AC_DEFINE([HAVE_FREEBSD_SENDFILE], [1], [Define to 1 if you have FreeBSD-style sendfile(2).])
       found_sendfile="yes, FreeBSD-style"
       AC_MSG_RESULT([[yes]])
     ],
     [AC_MSG_RESULT([[no]])
     ]
   )
  ]
)
AS_VAR_IF([[found_sendfile]], [["no"]],
  [
   AC_MSG_CHECKING([[for Darwin-style sendfile(2)]])
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
       [[
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

static void empty_func(void)
{
/* Check for declaration */
  (void)sendfile;
}
/* Declare again to check form match */
int sendfile(int, int, off_t, off_t*,
             struct sf_hdtr *, int);
       ]],
       [[
         int fd=0, s=1;
         off_t o = 0;
         off_t l = 5;
         int r;
         r = sendfile (fd, s, o, &l, (void*)0, 0);
         if (r)
           empty_func();
       ]]
      )
     ],
     [
       AC_DEFINE([HAVE_DARWIN_SENDFILE], [1], [Define to 1 if you have Darwin-style sendfile(2).])
       found_sendfile="yes, Darwin-style"
       AC_MSG_RESULT([[yes]])
     ],
     [AC_MSG_RESULT([[no]])
     ]
   )
  ]
)

AS_VAR_IF([[found_sendfile]], [["no"]],
  [
   AC_MSG_CHECKING([[for Solaris-style sendfile(3)]])
   SAVE_LIBS="$LIBS"
   LIBS="$LIBS -lsendfile"
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
       [[
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

static void empty_func(void)
{
/* Check for declaration */
  (void)sendfile;
}
/* Declare again to check form match */
ssize_t sendfile(int out_fd, int in_fd,
                 off_t *off, size_t len);
       ]],
       [[
         int fd1=0, fd2=1;
         off_t o = 0;
         size_t l = 5;
         ssize_t r;
         r = sendfile (fd1, fd2, &o, l);
         if (r)
           empty_func();
       ]]
      )
     ],
     [
       AC_DEFINE([HAVE_SOLARIS_SENDFILE], [1], [Define to 1 if you have Solaris-style sendfile(3).])
       found_sendfile="yes, Solaris-style"
       MHD_LIBDEPS="-lsendfile $MHD_LIBDEPS"
       MHD_LIBDEPS_PKGCFG="-lsendfile $MHD_LIBDEPS_PKGCFG"
       AC_MSG_RESULT([[yes]])
       AC_CHECK_FUNCS([sendfile64])
     ],
     [AC_MSG_RESULT([[no]])
     ]
   )
   LIBS="$SAVE_LIBS"
  ]
)
AS_IF([[test "x$found_sendfile" = "xno" && test "x$enable_sendfile" = "xyes"]],
  AC_MSG_ERROR([[sendfile() usage was requested by configure parameter, but no usable sendfile() function is detected]])
)

# optional: have error messages ?
AC_MSG_CHECKING([[whether to generate error messages]])
AC_ARG_ENABLE([messages],
   [AS_HELP_STRING([--disable-messages],
               [disable MHD error messages])],
   [enable_messages=${enableval}],
   [enable_messages=yes])
AS_IF([[test "x$enable_messages" = "xyes"]],
  [ AC_DEFINE([HAVE_MESSAGES],[1],[Define to 1 to enable support for error messages.]) ],
  [[ enable_messages=no ]])
AC_MSG_RESULT([[$enable_messages]])


# optional: have postprocessor?
AC_MSG_CHECKING([[whether to enable postprocessor]])
AC_ARG_ENABLE([postprocessor],
   [AS_HELP_STRING([--disable-postprocessor],
               [disable MHD PostProcessor functionality])],
   [enable_postprocessor=${enableval}],
   [enable_postprocessor=yes])
AS_IF([[test "x$enable_postprocessor" != "xno"]],
  [ enable_postprocessor=yes
    AC_DEFINE([HAVE_POSTPROCESSOR],[1],[Define to 1 if libmicrohttpd is compiled with postprocessor support.]) ])
AM_CONDITIONAL([HAVE_POSTPROCESSOR], [test "x$enable_postprocessor" != "xno"])
AC_MSG_RESULT([[$enable_postprocessor]])


# optional: have zzuf, socat?
AC_CHECK_PROG([have_zzuf],[zzuf], [yes], [no])
AC_CHECK_PROG([have_socat],[socat], [yes], [no])
AM_CONDITIONAL([HAVE_ZZUF], [test "x$have_zzuf" = "xyes"])
AM_CONDITIONAL([HAVE_SOCAT], [test "x$have_socat" = "xyes"])

have_gnutls=no
have_gnutls_sni=no
have_gcrypt=no
AS_UNSET([GNUTLS_CPPFLAGS])
AS_UNSET([GNUTLS_LDFLAGS])

# optional: HTTPS support.  Enabled by default
AC_ARG_ENABLE([https],
   [AS_HELP_STRING([--enable-https],
               [enable HTTPS support (yes, no, auto)[auto]])],
   [enable_https=${enableval}])
AS_IF([test "x$enable_https" != "xno"],[
#
# Next block is large unindented block
#

# gnutls
have_gnutls_pkgcfg=no
AC_MSG_CHECKING([[how to find GnuTLS library]])
AC_ARG_WITH([[gnutls]],
   [AS_HELP_STRING([[--with-gnutls[=PFX]]],[use GnuTLS for HTTPS support, optional PFX overrides pkg-config data for GnuTLS headers (PFX/include) and libs (PFX/lib)])],
   [
    AS_CASE([$with_gnutls],
     [no],[
        AC_MSG_RESULT([[GnuTLS disabled]])
        AS_UNSET([GNUTLS_CPPFLAGS])
        AS_UNSET([GNUTLS_CFLAGS])
        AS_UNSET([GNUTLS_LDFLAGS])
        AS_UNSET([GNUTLS_LIBS])
      ],
      [yes],[
        AC_MSG_RESULT([[automatically, forced]])
      ],
      [
        AC_MSG_RESULT([[-I$with_gnutls/include -L$with_gnutls/lib -lgnutls]])
        SAVE_LDFLAGS="$LDFLAGS"
        SAVE_CPPFLAGS="$CPPFLAGS"
        SAVE_CFLAGS="$CFLAGS"
        SAVE_LIBS="$LIBS"
        LDFLAGS="-L$with_gnutls/lib $LDFLAGS"
        CPPFLAGS="-I$with_gnutls/include $CPPFLAGS"
        have_gnutls_pkgcfg=no
          AC_CHECK_HEADERS([gnutls/gnutls.h],
            [AC_CHECK_LIB([gnutls], [gnutls_priority_set],
              [
                GNUTLS_CPPFLAGS="-I$with_gnutls/include"
                GNUTLS_LDFLAGS="-L$with_gnutls/lib"
                GNUTLS_LIBS="-lgnutls"
                have_gnutls=yes
              ])], [], [AC_INCLUDES_DEFAULT])
        AS_IF([test "x$have_gnutls" != "xyes"], [AC_MSG_ERROR([can't find usable libgnutls at specified prefix $with_gnutls])])
        CPPFLAGS="$SAVE_CPPFLAGS"
        CFLAGS="$SAVE_CFLAGS"
        LDFLAGS="$SAVE_LDFLAGS"
        LIBS="$SAVE_LIBS"
      ])
   ],
   [AC_MSG_RESULT([[automatically]])
   ])

AS_IF([test "x$with_gnutls" != "xno" && test "x$have_gnutls" != "xyes"],
  [
    AC_CACHE_CHECK([[whether to add pkg-config special search directories]], [mhd_cv_pkgconf_add_dirs],
     [
      AS_IF([[test "x$host_os" = "xsolaris2.11" && test "x$cross_compiling" = "xno"]],
       [
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
char size_chk[7-sizeof(char*)];
#if defined(_LP64) || defined(__LP64__) || defined(__x86_64) || defined(__x86_64__)
#error This is 64-bit target.
choke me now
#endif
#if defined(__amd64) || defined(__amd64__) || defined(__sparcv9) || defined(__sparc_v9__)
#error This is 64-bit target.
choke me now
#endif
           ]], [[]]
          )
         ],
         [
          mhd_cv_pkgconf_add_dirs='/usr/lib/pkgconfig/gnutls-3'
         ],
         [
          AS_IF([[test "x$host_cpu" = "xx86_64" || test "x$host_cpu" = "xi386"]],
                  [[mhd_cv_pkgconf_add_dirs='/usr/lib/amd64/pkgconfig/gnutls-3:/usr/lib/amd64/pkgconfig']],
                [[test "x$host_cpu" = "xsparc"]],
                  [[mhd_cv_pkgconf_add_dirs='/usr/lib/sparkv9/pkgconfig/gnutls-3:/usr/lib/sparkv9/pkgconfig']],
                  [[mhd_cv_pkgconf_add_dirs='/usr/lib/64/pkgconfig/gnutls-3:/usr/lib/64/pkgconfig']]
          )
         ]
        )
       ],
       [[ mhd_cv_pkgconf_add_dirs='no' ]]
      )
     ]
    )
    AS_IF([[test "x$mhd_cv_pkgconf_add_dirs" != "xno"]],
     [
      test "x$PKG_CONFIG_PATH" = "x" || PKG_CONFIG_PATH="${PKG_CONFIG_PATH}${PATH_SEPARATOR}"
      PKG_CONFIG_PATH="${PKG_CONFIG_PATH}${mhd_cv_pkgconf_add_dirs}"
      export PKG_CONFIG_PATH
     ]
    )
    PKG_CHECK_MODULES(GNUTLS, [[gnutls]],
      [
       have_gnutls_pkgcfg='yes'
       SAVE_CPPFLAGS="$CPPFLAGS"
       SAVE_CFLAGS="$CFLAGS"
       SAVE_LDFLAGS="$LDFLAGS"
       SAVE_LIBS="$LIBS"
       CFLAGS="$GNUTLS_CFLAGS $CFLAGS"
       LIBS="$LIBS $GNUTLS_LIBS"
       AC_MSG_CHECKING([[whether GnuTLS is usable]])
       AC_LINK_IFELSE([
         AC_LANG_PROGRAM([[
#include <gnutls/gnutls.h>
          ]], [[
                gnutls_session_t session;
                gnutls_priority_t priorities;
                gnutls_global_init();
                gnutls_priority_init(&priorities, "NORMAL", NULL);
                gnutls_init(&session, GNUTLS_SERVER);
                gnutls_priority_set(session, priorities);
          ]])],
          [
           AC_MSG_RESULT([[yes]])
           have_gnutls=yes
           # GNUTLS_CFLAGS is actually CPPFLAGS
           GNUTLS_CPPFLAGS="$GNUTLS_CFLAGS"
           # GNUTLS_CFLAGS is combination of LDFLAGS and LIBS
           GNUTLS_LDFLAGS="$GNUTLS_LIBS"
          ],
          [
           AC_MSG_RESULT([[no]])
           have_gnutls=no
          ])

       AS_IF([test "x$have_gnutls" != "xyes"],
         [
          AC_MSG_WARN([pkg-config reports that GnuTLS is present, but GnuTLS can't be used])
          AS_UNSET([GNUTLS_CPPFLAGS])
          AS_UNSET([GNUTLS_CFLAGS])
          AS_UNSET([GNUTLS_LDFLAGS])
          AS_UNSET([GNUTLS_LIBS])
         ]
       )
       CPPFLAGS="$SAVE_CPPFLAGS"
       CFLAGS="$SAVE_CFLAGS"
       LDFLAGS="$SAVE_LDFLAGS"
       LIBS="$SAVE_LIBS"
      ],
      [
       # check for GnuTLS at default paths
       have_gnutls_pkgcfg='no'
       AC_CHECK_HEADERS([gnutls/gnutls.h],
        [AC_CHECK_LIB([gnutls], [gnutls_priority_set],
          [
            GNUTLS_LIBS="-lgnutls"
            have_gnutls=yes
          ])], [], [AC_INCLUDES_DEFAULT])
      ])
  ])

have_gcrypt='unknown'
AS_IF([test "x$with_gnutls" != "xno" && test "x$have_gnutls" != "xyes"],
  [
   AM_PATH_LIBGCRYPT([1.2.2], [have_gcrypt=yes], [have_gcrypt=no])
   AS_IF([[test "x$have_gcrypt" = "xyes"]],
     [
       SAVE_CPPFLAGS="$CPPFLAGS"
       SAVE_CFLAGS="$CFLAGS"
       SAVE_LIBS="$LIBS"
       SAVE_LDFLAGS="$LDFLAGS"
       CFLAGS="$CFLAGS $LIBGCRYPT_CFLAGS"
       # LIBGCRYPT_CFLAGS can be actually a CPPFLAGS, so check them both
       CPPFLAGS="$CPPFLAGS $LIBGCRYPT_CFLAGS"
       AC_CHECK_HEADERS([gcrypt.h], [], [have_gcrypt=no], [AC_INCLUDES_DEFAULT])
       # Check for GnuTLS with gcrypt flags
       LDFLAGS="$LDFLAGS $LIBGCRYPT_LIBS"
       # A bit of hack: unset cache variable to force recheck
       AS_UNSET([ac_cv_header_gnutls_gnutls_h])
       AC_CHECK_HEADERS([gnutls/gnutls.h],
        [AS_UNSET([ac_cv_lib_gnutls_gnutls_priority_set]) # A bit of hack: unset cache variable to force recheck
         AC_CHECK_LIB([gnutls], [gnutls_priority_set],
          [
            GNUTLS_CPPFLAGS="$LIBGCRYPT_CFLAGS"
            GNUTLS_CFLAGS="$LIBGCRYPT_CFLAGS"
            GNUTLS_LDFLAGS="$LIBGCRYPT_LIBS"
            GNUTLS_LIBS="-lgnutls"
            have_gnutls=yes
          ])], [], [AC_INCLUDES_DEFAULT])
       CPPFLAGS="$SAVE_CPPFLAGS"
       CFLAGS="$SAVE_CFLAGS"
       LDFLAGS="$SAVE_LDFLAGS"
       LIBS="$SAVE_LIBS"
     ]
   )
  ]
)
AS_IF([test "x$have_gnutls" != "xyes" && test "x$with_gnutls" = "xyes"],
      [AC_MSG_ERROR([[can't find usable libgnutls]])])

  AS_IF([test "x$have_gnutls" = "xyes"],
    [
     SAVE_CPPFLAGS="$CPPFLAGS"
     SAVE_CFLAGS="$CFLAGS"
     SAVE_LDFLAGS="$LDFLAGS"
     SAVE_LIBS="$LIBS"
     CPPFLAGS="$GNUTLS_CPPFLAGS $CPPFLAGS"
     CFLAGS="$GNUTLS_CFLAGS $CFLAGS"
     LDFLAGS="$GNUTLS_LDFLAGS $LDFLAGS"
     LIBS="$LIBS $GNUTLS_LIBS"
     AC_MSG_CHECKING([[for gnutls_privkey_import_x509_raw()]])
     AC_LINK_IFELSE([
       AC_LANG_PROGRAM([[
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
          ]], [[
            gnutls_datum_t data;
            gnutls_privkey_t key = 0;
#ifndef gnutls_load_file
            (void)gnutls_load_file; /* Check for declaration. */
#endif
#ifndef gnutls_privkey_import_x509_raw
            (void)gnutls_privkey_import_x509_raw; /* Check for declaration. */
#endif
            gnutls_load_file("key.pem", &data);
            gnutls_privkey_import_x509_raw(key, &data, GNUTLS_X509_FMT_PEM, NULL, 0);
            gnutls_free(data.data);
          ]])], [[have_gnutls_sni=yes]], [[have_gnutls_sni=no]])
     AC_MSG_RESULT([[$have_gnutls_sni]])
     AC_CACHE_CHECK([[whether GnuTLS require libgcrypt initialisaion]], [mhd_cv_gcrypt_required],
       [
        AC_COMPILE_IFELSE(
          [
           AC_LANG_PROGRAM(
             [
#include <gnutls/gnutls.h>
             ],
             [
#if !defined(GNUTLS_VERSION_NUMBER) || GNUTLS_VERSION_NUMBER+0 <= 0x020c14
#error Old versions of GnuTLS require libgcript initialisaion
choke me now
#endif
             ]
           )
          ],
          [[mhd_cv_gcrypt_required='no']], [[mhd_cv_gcrypt_required='yes']]
        )
       ]
     )
     CPPFLAGS="$SAVE_CPPFLAGS"
     CFLAGS="$SAVE_CFLAGS"
     LDFLAGS="$SAVE_LDFLAGS"
     LIBS="$SAVE_LIBS"
    ],
    [
     AS_UNSET([GNUTLS_CPPFLAGS])
     AS_UNSET([GNUTLS_LDFLAGS])
    ]
  )

  AS_IF([[test "x$mhd_cv_gcrypt_required" = "xyes" && test "x$have_gcrypt" = "xunknown"]],
    [
     AM_PATH_LIBGCRYPT([1.2.2], [have_gcrypt=yes], [have_gcrypt=no])
     AS_IF([[test "x$have_gcrypt" = "xyes"]],
       [
         SAVE_CPPFLAGS="$CPPFLAGS"
         SAVE_CFLAGS="$CFLAGS"
         CFLAGS="$CFLAGS $LIBGCRYPT_CFLAGS"
         # LIBGCRYPT_CFLAGS can be actually a CPPFLAGS, so check them both
         CPPFLAGS="$CPPFLAGS $LIBGCRYPT_CFLAGS"
         AC_CHECK_HEADERS([gcrypt.h], [], [have_gcrypt=no], [AC_INCLUDES_DEFAULT])
         CPPFLAGS="$SAVE_CPPFLAGS"
         CFLAGS="$SAVE_CFLAGS"
       ]
     )
    ]
  )

  AS_UNSET([[crypt_missing]])
  AS_IF([[test "x$have_gnutls" = "xyes"]],
    [
     AS_IF([[test "x$mhd_cv_gcrypt_required" = "xyes" && test "x$have_gcrypt" != "xyes"]],
       [
        crypt_missing="required libgcrypt"
        AS_IF([[test "x$enable_https" = "xyes" ]], [AC_MSG_ERROR([[HTTPS support cannot be enabled without $crypt_missing.]])])
        enable_https=no
        MSG_HTTPS="no (lacking $crypt_missing)"
        AS_UNSET([LIBGCRYPT_CFLAGS])
        AS_UNSET([LIBGCRYPT_LIBS])
        AS_UNSET([GNUTLS_CPPFLAGS])
        AS_UNSET([GNUTLS_CFLAGS])
        AS_UNSET([GNUTLS_LDFLAGS])
        AS_UNSET([GNUTLS_LIBS])
       ],
         [
          AC_DEFINE([[HTTPS_SUPPORT]],[[1]],[Define to 1 if libmicrohttpd is compiled with HTTPS support.])
          enable_https=yes
          AS_IF([[test "x$mhd_cv_gcrypt_required" = "xyes"]],
            [
             MSG_HTTPS="yes (using libgnutls and libgcrypt)"
	         MHD_TLS_LIB_CPPFLAGS="$LIBGCRYPT_CFLAGS $GNUTLS_CPPFLAGS"
             MHD_TLS_LIB_CFLAGS="$LIBGCRYPT_CFLAGS $GNUTLS_CFLAGS"
             MHD_TLS_LIB_LDFLAGS="$GNUTLS_LDFLAGS"
             MHD_TLS_LIBDEPS="$GNUTLS_LIBS $LIBGCRYPT_LIBS"
             AC_DEFINE([[MHD_HTTPS_REQUIRE_GCRYPT]], [[1]], [Define to `1' if HTTPS require initialisation of libgcrypt])
            ],
            [
             MSG_HTTPS="yes (using libgnutls)"
             AS_UNSET([LIBGCRYPT_CFLAGS])
             AS_UNSET([LIBGCRYPT_LIBS])
             MHD_TLS_LIB_CPPFLAGS="$GNUTLS_CPPFLAGS"
             MHD_TLS_LIB_CFLAGS="$GNUTLS_CFLAGS"
             MHD_TLS_LIB_LDFLAGS="$GNUTLS_LDFLAGS"
             MHD_TLS_LIBDEPS="$GNUTLS_LIBS"
            ]
          )
          AS_IF([[ test "x$have_gnutls_pkgcfg" = "xyes" ]],
            [ # remove GnuTLS from private libs in .pc file as it defined in Requires.private
              MHD_REQ_PRIVATE='gnutls'
              AS_IF([[test "x$mhd_cv_gcrypt_required" = "xyes"]],
                [[MHD_LIBDEPS_PKGCFG="$LIBGCRYPT_LIBS $MHD_LIBDEPS_PKGCFG"]]
              )
            ],
            [
              MHD_REQ_PRIVATE=''
              AS_IF([[test "x$mhd_cv_gcrypt_required" = "xyes"]],
                [[MHD_LIBDEPS_PKGCFG="$LIBGCRYPT_LIBS $MHD_LIBDEPS_PKGCFG"]]
              )
              MHD_LIBDEPS_PKGCFG="$GNUTLS_LIBS $MHD_LIBDEPS_PKGCFG"
          ])
         ]
     )
    ],
    [
     crypt_missing="libgnutls"
     AS_IF([[test "x$enable_https" = "xyes" ]], [AC_MSG_ERROR([[HTTPS support cannot be enabled without $crypt_missing.]])])
     enable_https=no
     MSG_HTTPS="no (lacking $crypt_missing)"
     AS_UNSET([LIBGCRYPT_CFLAGS])
     AS_UNSET([LIBGCRYPT_LIBS])
     AS_UNSET([GNUTLS_CPPFLAGS])
     AS_UNSET([GNUTLS_CFLAGS])
     AS_UNSET([GNUTLS_LDFLAGS])
     AS_UNSET([GNUTLS_LIBS])
    ]
  )
],[
  MSG_HTTPS="no (disabled)"
])

#
# End of large unindented block
#


AC_MSG_CHECKING(whether to support HTTPS)
AC_MSG_RESULT([$MSG_HTTPS])

AM_CONDITIONAL([HAVE_GNUTLS], [[test "x$have_gnutls" = "xyes"]])
AM_CONDITIONAL([HAVE_GNUTLS_SNI], [[test "x$have_gnutls_sni" = "xyes"]])
AM_CONDITIONAL([ENABLE_HTTPS], [test "x$enable_https" = "xyes"])
AM_CONDITIONAL([HTTPS_REQUIRE_GCRYPT], [[test "x$enable_https" = "xyes" && test "x$mhd_cv_gcrypt_required" = "xyes"]])
AC_SUBST([GNUTLS_CPPFLAGS])
AC_SUBST([GNUTLS_CFLAGS])
AC_SUBST([GNUTLS_LDFLAGS])
AC_SUBST([GNUTLS_LIBS])

# optional: HTTP Basic Auth support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP basic authentication]])
AC_ARG_ENABLE([bauth],
		AS_HELP_STRING([--disable-bauth],
			[disable HTTP basic Auth support]),
		[enable_bauth=${enableval}],
		[enable_bauth=yes])
AS_IF([[test "x$enable_bauth" != "xno"]],
  [ enable_bauth=yes
    AC_DEFINE([BAUTH_SUPPORT],[1],[Define to 1 if libmicrohttpd is compiled with basic Auth support.]) ])
AM_CONDITIONAL([ENABLE_BAUTH], [test "x$enable_bauth" != "xno"])
AC_MSG_RESULT([[$enable_bauth]])

# optional: HTTP Digest Auth support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP digest authentication]])
AC_ARG_ENABLE([dauth],
		AS_HELP_STRING([--disable-dauth],
			[disable HTTP basic and digest Auth support]),
		[enable_dauth=${enableval}],
		[enable_dauth=yes])
AS_IF([[test "x$enable_dauth" != "xno"]],
  [ enable_dauth=yes
    AC_DEFINE([DAUTH_SUPPORT],[1],[Define to 1 if libmicrohttpd is compiled with digest Auth support.]) ])
AM_CONDITIONAL([ENABLE_DAUTH], [test "x$enable_dauth" != "xno"])
AC_MSG_RESULT([[$enable_dauth]])

# optional: HTTP "Upgrade" support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP "Upgrade"]])
AC_ARG_ENABLE([[httpupgrade]],
    AS_HELP_STRING([[--disable-httpupgrade]],
      [disable HTTP "Upgrade" support]),
    [AS_VAR_IF([[enable_httpupgrade]],[["no"]],[],[[enable_httpupgrade='yes']])],
    [[enable_httpupgrade='yes']])
AS_VAR_IF([[enable_httpupgrade]],[["yes"]],
  [
   AC_DEFINE([[UPGRADE_SUPPORT]],[[1]],[Define to 1 if libmicrohttpd is compiled with HTTP Upgrade support.]) ])
AM_CONDITIONAL([ENABLE_UPGRADE], [[test "x$enable_httpupgrade" = "xyes"]])
AC_MSG_RESULT([[$enable_httpupgrade]])

AC_CACHE_CHECK([[for calloc()]], [[mhd_cv_have_func_calloc]],
  [
   AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
       ]],[[void * ptr = calloc(1, 2)]])
     ],
     [[mhd_cv_have_func_calloc='yes']],
     [[mhd_cv_have_func_calloc='no']]
   )
  ]
)
AS_VAR_IF([[mhd_cv_have_func_calloc]], [["yes"]],
  [AC_DEFINE([[HAVE_CALLOC]], [[1]], [Define to 1 if you have the usable `calloc' function.])])

# Check for fork() and waitpid(). They are used for tests.
AC_MSG_CHECKING([[for fork()]])
mhd_have_fork_waitpid='no'
AC_LINK_IFELSE(
  [
   AC_LANG_PROGRAM(
     [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
     ]], [[
  pid_t p = fork ();
  if (0 == p)
    return 1;
     ]])
  ], [
   AC_DEFINE([[HAVE_FORK]], [[1]], [Define to 1 if you have the usable `fork' function.])
   AC_MSG_RESULT([[yes]])

   AC_MSG_CHECKING([[for waitpid()]])
   AC_LINK_IFELSE(
     [
      AC_LANG_PROGRAM(
        [[
#include <sys/wait.h>
        ]], [[
     pid_t p = fork ();
     if (0 == p)
       return 1;
     waitpid (p, (void*)0, 0)
        ]])
     ], [
      AC_DEFINE([[HAVE_WAITPID]], [[1]], [Define to 1 if you have the usable `waitpid' function.])
      AC_MSG_RESULT([[yes]])
      mhd_have_fork_waitpid='yes'
    ],[
       AC_MSG_RESULT([[no]])
    ])
],[
   AC_MSG_RESULT([[no]])
])

AM_CONDITIONAL([HAVE_FORK_WAITPID], [test "x$mhd_have_fork_waitpid" = "xyes"])

# gcov compilation
AC_MSG_CHECKING(whether to compile with support for code coverage analysis)
AC_ARG_ENABLE([coverage],
              AS_HELP_STRING([--enable-coverage],
                             [compile the library with code coverage support]),
              [use_gcov=${enableval}],
              [use_gcov=no])
AC_MSG_RESULT($use_gcov)
AM_CONDITIONAL([USE_COVERAGE], [test "x$use_gcov" = "xyes"])

AX_COUNT_CPUS
AS_IF([[test "$CPU_COUNT" -gt "32"]], [[CPU_COUNT="32"]])dnl Limit resource usage
AC_SUBST([CPU_COUNT])

AC_MSG_CHECKING([[whether to enable debug asserts]])
AC_ARG_ENABLE([[asserts]],
              AS_HELP_STRING([[--enable-asserts]],
                             [enable test build with debug asserts]),
              [], [[enable_asserts='no']])
AS_CASE([[$enable_asserts]], [[yes]], [[:]], [[no]], [[:]], [[enable_asserts='no']])
AC_MSG_RESULT([[$enable_asserts]])

AS_VAR_IF([[enable_asserts]], [["yes"]],
  [
   AC_DEFINE([[_DEBUG]], [[1]], [Define to use debug asserts.])
   [mhd_assert_test_prg="#include <assert.h>
   int pos_val(void) {return 5;}
   int neg_val(void) {return -5;}
   int main(void)
   { int pos_var = pos_val(), neg_var = neg_val();
     assert(neg_var > pos_var); /* Must trigger assert. */
     (void)pos_var; (void)neg_var;
     return 0; }
   "]
   AC_CACHE_CHECK([[whether system assert() is available]], [mhd_cv_sys_assert_avail],
     [
      AC_LINK_IFELSE([AC_LANG_SOURCE([[$mhd_assert_test_prg]])],
                     [[mhd_cv_sys_assert_avail='yes']],
                     [[mhd_cv_sys_assert_avail='no']])
     ]
   )
   AS_VAR_IF([[mhd_cv_sys_assert_avail]], [["yes"]],
     [
      AC_CACHE_CHECK([[whether system assert() is usable]], [mhd_cv_sys_assert_use],
        [
         AC_RUN_IFELSE([AC_LANG_SOURCE([[$mhd_assert_test_prg]])],
                       [[mhd_cv_sys_assert_use='no']],
                       [[mhd_cv_sys_assert_use='yes']],
                       [[mhd_cv_sys_assert_use='assuming yes']])
        ]
      )
      AS_VAR_IF([[mhd_cv_sys_assert_use]], [["no"]], [],
        [AC_DEFINE([[HAVE_ASSERT]], [[1]], [Define if you have usable assert() and assert.h])])
     ]
   )
   AS_UNSET([mhd_assert_test_prg])
  ],
  [AC_DEFINE([[NDEBUG]], [[1]], [Define to disable usage of debug asserts.])]
)

MHD_LIB_LDFLAGS="$MHD_LIB_LDFLAGS -export-dynamic -no-undefined"

AC_SUBST(MHD_LIB_CPPFLAGS)
AC_SUBST(MHD_LIB_CFLAGS)
AC_SUBST(MHD_LIB_LDFLAGS)
AC_SUBST(MHD_LIBDEPS)
AC_SUBST(MHD_TLS_LIB_CPPFLAGS)
AC_SUBST(MHD_TLS_LIB_CFLAGS)
AC_SUBST(MHD_TLS_LIB_LDFLAGS)
AC_SUBST(MHD_TLS_LIBDEPS)

# for pkg-config
AC_SUBST([MHD_REQ_PRIVATE])
AC_SUBST([MHD_LIBDEPS_PKGCFG])

AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)

# Used for 'po' directory staff
AC_SUBST([ac_configure_args])
AC_SUBST([EMPTY_VAR], [[]])

# We define the paths here, because MinGW/GCC expands paths
# passed through the command line ("-DDIR=..."). This would
# lead to hard-coded paths ("C:\mingw\mingw\bin...") that do
# not contain the actual installation.
AC_DEFINE_DIR([MHD_PLUGIN_INSTALL_PREFIX], [libdir/libmicrohttpd], [tls plugins])


# should experimental code be compiled (code that may not yet compile)?
AC_MSG_CHECKING(whether to compile experimental code)
AC_ARG_ENABLE([experimental],
   [AS_HELP_STRING([--enable-experimental], [enable compiling experimental code])],
   [enable_experimental=${enableval}],
   [enable_experimental=no])
AC_MSG_RESULT($enable_experimental)
AM_CONDITIONAL([HAVE_EXPERIMENTAL], [test "x$enable_experimental" = "xyes"])


AC_CONFIG_FILES([libmicrohttpd.pc
w32/common/microhttpd_dll_res_vc.rc
Makefile
contrib/Makefile
doc/Makefile
doc/doxygen/libmicrohttpd.doxy
doc/doxygen/Makefile
doc/examples/Makefile
m4/Makefile
src/Makefile
src/include/Makefile
src/lib/Makefile
src/microhttpd/Makefile
src/examples/Makefile
src/testcurl/Makefile
src/testcurl/https/Makefile
src/testzzuf/Makefile])
AC_OUTPUT

# Finally: summary
AS_IF([test "x$enable_curl" != "xyes"],
 [MSG_CURL="no, many unit tests will not run"],
 [MSG_CURL="yes"])

AC_MSG_NOTICE([GNU libmicrohttpd ${PACKAGE_VERSION} Configuration Summary:
  Target directory:  ${prefix}
  Cross-compiling:   ${cross_compiling}
  Operating System:  ${host_os}
  Shutdown of listening socket trigger select: ${mhd_cv_host_shtdwn_trgr_select}
  Inter-thread comm: ${use_itc}
  poll support:      ${enable_poll=no}
  epoll support:     ${enable_epoll=no}
  sendfile used:     ${found_sendfile}
  HTTPS support:     ${MSG_HTTPS}
  Threading lib:     ${USE_THREADS}
  Use thread names:  ${enable_thread_names}
  Use debug asserts: ${enable_asserts}
  Messages:          ${enable_messages}
  Gettext:           ${have_po}
  Basic auth.:       ${enable_bauth}
  Digest auth.:      ${enable_dauth}
  HTTP "Upgrade":    ${enable_httpupgrade}
  Postproc:          ${enable_postprocessor}
  Build docs:        ${enable_doc}
  Build examples:    ${enable_examples}
  Test with libcurl: ${MSG_CURL}
])

AS_IF([test "x$enable_https" = "xyes"],
 [AC_MSG_NOTICE([HTTPS subsystem configuration:
  License         :  LGPL only
 ])],
 [AC_MSG_NOTICE([
  License         :  LGPL or eCos
 ])])

AS_IF([test "x$enable_bauth" != "xyes" || \
   test "x$enable_dauth" != "xyes" || \
   test "x$enable_httpupgrade" != "xyes" || \
   test "x$enable_postprocessor" != "xyes"],
   [AC_MSG_NOTICE([WARNING: This will be a custom build with missing symbols. Do NOT use this build in a distribution. Building with these kinds of configure options is only for custom builds for embedded systems.])])
