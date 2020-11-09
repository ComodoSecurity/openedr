# SYNOPSIS
#
#   MHD_SYS_EXT([VAR-ADD-CPPFLAGS])
#
# DESCRIPTION
#
#   This macro checks system headers and add defines that enable maximum
#   number of exposed system interfaces. Macro verifies that added defines
#   will not break basic headers, some defines are also checked against
#   real recognition by headers.
#   If VAR-ADD-CPPFLAGS is specified, defines will be added to this variable
#   in form suitable for CPPFLAGS. Otherwise defines will be added to
#   configuration header (usually 'config.h').
#
#   Example usage:
#
#     MHD_SYS_EXT
#
#   or
#
#     MHD_SYS_EXT([CPPFLAGS])
#
#   Macro is not enforced to be called before AC_COMPILE_IFELSE, but it
#   advisable to call macro before any compile and header tests since
#   additional defines can change results of those tests.
#
#   Defined in command line macros are always honored and cache variables
#   used in all checks so if any test will not work correctly on some
#   platform, user may simply fix it by giving correct defines in CPPFLAGS
#   or by giving cache variable in configure parameters, for example:
#
#     ./configure CPPFLAGS='-D_XOPEN_SOURCE=1 -D_XOPEN_SOURCE_EXTENDED'
#
#   or
#
#     ./configure mhd_cv_define__xopen_source_sevenh_works=no
#
#   This simplify building from source on exotic platforms as patching
#   of configure.ac is not required to change results of tests.
#
# LICENSE
#
#   Copyright (c) 2016, 2017 Karlson2k (Evgeny Grin) <k2k@narod.ru>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 2

AC_DEFUN([MHD_SYS_EXT],[dnl
  AC_PREREQ([2.64])dnl for AS_VAR_IF, AS_VAR_SET_IF, m4_ifnblank
  AC_LANG_PUSH([C])dnl Use C language for simplicity
  mhd_mse_sys_ext_defines=""
  mhd_mse_sys_ext_flags=""

  dnl Check platform-specific extensions.
  dnl Use compiler-based test for determinig target.

  dnl Always add _GNU_SOURCE if headers allow.
  MHD_CHECK_DEF_AND_ACCEPT([[_GNU_SOURCE]], [],
    [[${mhd_mse_sys_ext_defines}]], [mhd_cv_macro_add__gnu_source="no"],
    [mhd_cv_macro_add__gnu_source="yes"],
    [mhd_cv_macro_add__gnu_source="no"]
  )
  AS_VAR_IF([mhd_cv_macro_add__gnu_source], ["yes"],
    [
      _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[_GNU_SOURCE]])
    ]
  )

  dnl __BSD_VISIBLE is actually a small hack for FreeBSD.
  dnl Funny that it's used in some Android versions too.
  AC_CACHE_CHECK([[whether to try __BSD_VISIBLE macro]],
    [[mhd_cv_macro_try___bsd_visible]], [dnl
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
/* Warning: test with inverted logic! */
#ifdef __FreeBSD__
#error Target FreeBSD
choke me now;
#endif /* __FreeBSD__ */

#ifdef __ANDROID__
#include <android/api-level.h>
#ifndef __ANDROID_API_O__
#error Target is Android NDK before R14
choke me now;
#endif /* ! __ANDROID_API_O__ */
#endif /* __ANDROID__ */
        ]],[])],
      [[mhd_cv_macro_try___bsd_visible="no"]],
      [[mhd_cv_macro_try___bsd_visible="yes"]]
    )
  ])
  AS_VAR_IF([[mhd_cv_macro_try___bsd_visible]], [["yes"]],
  [dnl
    AC_CACHE_CHECK([[whether __BSD_VISIBLE is already defined]],
      [[mhd_cv_macro___bsd_visible_defined]], [dnl
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
${mhd_mse_sys_ext_defines}
/* Warning: test with inverted logic! */
#ifdef __BSD_VISIBLE
#error __BSD_VISIBLE is defined
choke me now;
#endif
          ]],[])
      ],
        [[mhd_cv_macro___bsd_visible_defined="no"]],
        [[mhd_cv_macro___bsd_visible_defined="yes"]]
      )
    ])
    AS_VAR_IF([[mhd_cv_macro___bsd_visible_defined]], [["yes"]], [[:]],
    [dnl
      MHD_CHECK_ACCEPT_DEFINE(
        [[__BSD_VISIBLE]], [], [[${mhd_mse_sys_ext_defines}]],
        [
          _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[__BSD_VISIBLE]])
        ]
      )dnl
    ])
  ])


  dnl _DARWIN_C_SOURCE enables additional functionality on Darwin.
  MHD_CHECK_DEFINED_MSG([[__APPLE__]], [[${mhd_mse_sys_ext_defines}]],
    [[whether to try _DARWIN_C_SOURCE macro]],
  [dnl
    MHD_CHECK_DEF_AND_ACCEPT(
      [[_DARWIN_C_SOURCE]], [], [[${mhd_mse_sys_ext_defines}]], [],
      [
        _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[_DARWIN_C_SOURCE]])
      ]
    )dnl
  ])

  dnl __EXTENSIONS__ unlocks almost all interfaces on Solaris.
  MHD_CHECK_DEFINED_MSG([[__sun]], [[${mhd_mse_sys_ext_defines}]],
    [[whether to try __EXTENSIONS__ macro]],
  [dnl
    MHD_CHECK_DEF_AND_ACCEPT(
      [[__EXTENSIONS__]], [], [[${mhd_mse_sys_ext_defines}]], [],
      [
        _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[__EXTENSIONS__]])
      ]
    )dnl
  ])

  dnl _NETBSD_SOURCE switch on almost all headers definitions on NetBSD.
  MHD_CHECK_DEFINED_MSG([[__NetBSD__]], [[${mhd_mse_sys_ext_defines}]],
    [[whether to try _NETBSD_SOURCE macro]],
  [dnl
    MHD_CHECK_DEF_AND_ACCEPT(
      [[_NETBSD_SOURCE]], [], [[${mhd_mse_sys_ext_defines}]], [],
      [
        _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[_NETBSD_SOURCE]])
      ]
    )dnl
  ])

  dnl _BSD_SOURCE currently used only on OpenBSD to unhide functions.
  MHD_CHECK_DEFINED_MSG([[__OpenBSD__]], [[${mhd_mse_sys_ext_defines}]],
    [[whether to try _BSD_SOURCE macro]],
  [dnl
    MHD_CHECK_DEF_AND_ACCEPT(
      [[_BSD_SOURCE]], [], [[${mhd_mse_sys_ext_defines}]], [],
      [
        _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[_BSD_SOURCE]])
      ]
    )dnl
  ])

  dnl _TANDEM_SOURCE unhides most functions on NonStop OS
  dnl (which comes from Tandem Computers decades ago).
  MHD_CHECK_DEFINED_MSG([[__TANDEM]], [[${mhd_mse_sys_ext_defines}]],
    [[whether to try _TANDEM_SOURCE macro]],
  [dnl
    MHD_CHECK_DEF_AND_ACCEPT(
      [[_TANDEM_SOURCE]], [], [[${mhd_mse_sys_ext_defines}]], [],
      [
        _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[_TANDEM_SOURCE]])
      ]
    )dnl
  ])

  dnl _ALL_SOURCE makes visible POSIX and non-POSIX symbols
  dnl on z/OS, AIX and Interix.
  AC_CACHE_CHECK([[whether to try _ALL_SOURCE macro]],
    [[mhd_cv_macro_try__all_source]], [dnl
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !defined(__TOS_MVS__) && !defined (__INTERIX)
#error Target is not z/OS, AIX or Interix
choke me now;
#endif
        ]],[])],
      [[mhd_cv_macro_try__all_source="yes"]],
      [[mhd_cv_macro_try__all_source="no"]]
    )
  ])
  AS_VAR_IF([[mhd_cv_macro_try__all_source]], [["yes"]],
  [dnl
    MHD_CHECK_DEF_AND_ACCEPT(
      [[_ALL_SOURCE]], [], [[${mhd_mse_sys_ext_defines}]], [],
      [
        _MHD_SYS_EXT_VAR_ADD_FLAG([[mhd_mse_sys_ext_defines]], [[mhd_mse_sys_ext_flags]], [[_ALL_SOURCE]])
      ]
    )dnl
  ])

  mhd_mse_xopen_features=""
  mhd_mse_xopen_defines=""
  mhd_mse_xopen_flags=""

  AC_CACHE_CHECK([[whether _XOPEN_SOURCE is already defined]],
    [[mhd_cv_macro__xopen_source_defined]], [
      _MHD_CHECK_DEFINED([[_XOPEN_SOURCE]], [[${mhd_mse_sys_ext_defines}]],
        [[mhd_cv_macro__xopen_source_defined="yes"]],
        [[mhd_cv_macro__xopen_source_defined="no"]]
      )
    ]
  )
  AS_VAR_IF([[mhd_cv_macro__xopen_source_defined]], [["no"]],
    [
      dnl Some platforms (namely: Solaris) use '==' checks instead of '>='
      dnl for _XOPEN_SOURCE, resulting that unknown for platform values are
      dnl interpreted as oldest and platform expose reduced number of
      dnl interfaces. Next checks will ensure that platform recognise
      dnl requested mode instead of blindly define high number that can
      dnl be simply ignored by platform.
      _MHD_CHECK_POSIX2008([[mhd_mse_xopen_defines]],
        [[mhd_mse_xopen_flags]],
        [[${mhd_mse_sys_ext_defines}]],
        [mhd_mse_xopen_features="${mhd_cv_headers_posix2008}"],
        [
          _MHD_CHECK_POSIX2001([[mhd_mse_xopen_defines]],
            [[mhd_mse_xopen_flags]],
             [[${mhd_mse_sys_ext_defines}]],
            [mhd_mse_xopen_features="${mhd_cv_headers_posix2001}"],
            [
              _MHD_CHECK_SUSV2([[mhd_mse_xopen_defines]],
                [[mhd_mse_xopen_flags]],
                [[${mhd_mse_sys_ext_defines}]],
                [mhd_mse_xopen_features="${mhd_cv_headers_susv2}"],
                [mhd_mse_xopen_features="${mhd_cv_headers_susv2}"]
              )
            ]
          )
        ]
      )
    ]
  )

  AS_IF([[test "x${mhd_cv_macro__xopen_source_defined}" = "xno" && \
          test "x${mhd_cv_macro__xopen_source_def_fiveh}" = "xno" || \
          test "x${mhd_mse_xopen_features}" = "xnot available" ]],
    [
      _MHD_XOPEN_VAR_ADD([mhd_mse_xopen_defines], [mhd_mse_xopen_flags], [${mhd_mse_sys_ext_defines}])
    ]
  )

  mhd_mse_sys_features_src="
#ifdef __APPLE__
#include <sys/socket.h>
#else
#error No useful system features.
choke me now;
#endif

int main()
{
#ifdef __APPLE__
#ifndef sendfile
  (void) sendfile;
#endif
#endif
  return 0;
}
"
  AC_CACHE_CHECK([[for useful system-specific features]],
    [[mhd_cv_headers_useful_features_present]], [dnl
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
${mhd_mse_sys_ext_defines}
${mhd_mse_sys_features_src}
      ]])],
      [[mhd_cv_headers_useful_features_present="yes"]],
      [[mhd_cv_headers_useful_features_present="no"]]
    )dnl
  ])
  AS_VAR_IF([[mhd_cv_headers_useful_features_present]], [["yes"]],
    [
      AS_IF([[test "x${mhd_mse_xopen_flags}" = "x"]], [[:]],
        [
          AC_CACHE_CHECK([[whether useful system-specific features works with ${mhd_mse_xopen_flags}]],
            [[mhd_cv_headers_useful_features_works_xopen]], [dnl
            AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
${mhd_mse_sys_ext_defines}
${mhd_mse_xopen_defines}
${mhd_mse_sys_features_src}
              ]])],
              [[mhd_cv_headers_useful_features_works_xopen="yes"]],
              [[mhd_cv_headers_useful_features_works_xopen="no"]]
            )dnl
          ])dnl
        ]
      )dnl
    ]
  )


  AS_IF([[test "x${mhd_cv_headers_useful_features_present}" = "xyes" && \
          test "x${mhd_cv_headers_useful_features_works_xopen}" = "xno" && \
          test "x${mhd_mse_xopen_flags}" != "x"]],
    [
      _MHD_VAR_CONTAIN([[mhd_mse_xopen_features]], [[, activated by _XOPEN_SOURCE]],
        [
          AC_MSG_WARN([[_XOPEN_SOURCE macro is required to activate all headers symbols, however some useful system-specific features does not work with _XOPEN_SOURCE. ]dnl
          [_XOPEN_SOURCE macro will not be used.]])
        ]
      )
      AS_UNSET([[mhd_mse_xopen_defines]])
      AS_UNSET([[mhd_mse_xopen_flags]])
      AS_UNSET([[mhd_mse_xopen_values]])
    ]
  )

  dnl Discard temporal source variables
  AS_UNSET([[mhd_mse_sys_features_src]])
  AS_UNSET([[mhd_mse_xopen_defines]])
  AS_UNSET([[mhd_mse_sys_ext_defines]])

  dnl Determined all required defines.
  AC_MSG_CHECKING([[for final set of defined symbols]])
  m4_ifblank([$1], [dnl
    AH_TEMPLATE([[_XOPEN_SOURCE]], [Define to maximum value supported by system headers])dnl
    AH_TEMPLATE([[_XOPEN_SOURCE_EXTENDED]], [Define to 1 if _XOPEN_SOURCE is defined to value less than 500 ]dnl
      [and system headers requre this symbol])dnl
    AH_TEMPLATE([[_XOPEN_VERSION]], [Define to maximum value supported by system headers if _XOPEN_SOURCE ]dnl
      [is defined to value less than 500 and headers do not support _XOPEN_SOURCE_EXTENDED])dnl
    AH_TEMPLATE([[_GNU_SOURCE]], [Define to 1 to enable GNU-related header features])dnl
    AH_TEMPLATE([[__BSD_VISIBLE]], [Define to 1 if it is required by headers to expose additional symbols])dnl
    AH_TEMPLATE([[_DARWIN_C_SOURCE]], [Define to 1 if it is required by headers to expose additional symbols])dnl
    AH_TEMPLATE([[__EXTENSIONS__]], [Define to 1 if it is required by headers to expose additional symbols])dnl
    AH_TEMPLATE([[_NETBSD_SOURCE]], [Define to 1 if it is required by headers to expose additional symbols])dnl
    AH_TEMPLATE([[_BSD_SOURCE]], [Define to 1 if it is required by headers to expose additional symbols])dnl
    AH_TEMPLATE([[_TANDEM_SOURCE]], [Define to 1 if it is required by headers to expose additional symbols])dnl
    AH_TEMPLATE([[_ALL_SOURCE]], [Define to 1 if it is required by headers to expose additional symbols])dnl
  ])
  for mhd_mse_Flag in ${mhd_mse_sys_ext_flags} ${mhd_mse_xopen_flags}
  do
    m4_ifnblank([$1], [dnl
      AS_VAR_APPEND([$1],[[" -D$mhd_mse_Flag"]])
    ], [dnl
      AS_CASE([[$mhd_mse_Flag]], [[*=*]],
      [dnl
        AC_DEFINE_UNQUOTED([[`echo $mhd_mse_Flag | cut -f 1 -d =`]],
          [[`echo $mhd_mse_Flag | cut -f 2 -d = -s`]])
      ], [dnl
        AC_DEFINE_UNQUOTED([[$mhd_mse_Flag]])
      ])
    ])
  done
  AS_UNSET([[mhd_mse_Flag]])
  dnl Trim whitespaces
  mhd_mse_result=`echo ${mhd_mse_sys_ext_flags} ${mhd_mse_xopen_flags}`
  AC_MSG_RESULT([[${mhd_mse_result}]])
  AS_UNSET([[mhd_mse_result]])
  AS_UNSET([[mhd_mse_xopen_flags]])
  AS_UNSET([[mhd_mse_sys_ext_flags]])
  AC_LANG_POP([C])
])


#
# _MHD_SYS_EXT_VAR_ADD_FLAG(DEFINES_VAR, FLAGS_VAR,
#                           FLAG, [FLAG-VALUE = 1])
#
# Internal macro, only to be used from MHD_SYS_EXT, _MHD_XOPEN_VAR_ADD

m4_define([_MHD_SYS_EXT_VAR_ADD_FLAG], [dnl
  m4_ifnblank([$4],[dnl
    ]m4_normalize([$1])[="[$]{]m4_normalize([$1])[}[#define ]m4_normalize($3) ]m4_normalize([$4])[
"
    AS_IF([test "x[$]{]m4_normalize([$2])[}" = "x"],
      []m4_normalize([$2])[="]m4_normalize([$3])[=]m4_normalize([$4])["],
      []m4_normalize([$2])[="[$]{]m4_normalize([$2])[} ]m4_normalize([$3])[=]m4_normalize([$4])["]
    )dnl
  ], [dnl
    ]m4_normalize([$1])[="[$]{]m4_normalize([$1])[}[#define ]m4_normalize($3) 1
"
    AS_IF([test "x[$]{]m4_normalize([$2])[}" = "x"],
      []m4_normalize([$2])[="]m4_normalize([$3])["],
      []m4_normalize([$2])[="[$]{]m4_normalize([$2])[} ]m4_normalize([$3])["]
    )dnl
  ])dnl
])

#
# _MHD_VAR_IF(VAR, VALUE, [IF-EQ], [IF-NOT-EQ])
#
# Same as AS_VAR_IF, except that it expands to nothing if
# both IF-EQ and IF-NOT-EQ are empty.

m4_define([_MHD_VAR_IF],[dnl
m4_ifnblank([$3$4],[dnl
AS_VAR_IF([$1],[$2],[$3],[$4])])])

#
# _MHD_STRING_CONTAIN(VAR, MATCH, [IF-MATCH], [IF-NOT-MATCH])
#

AC_DEFUN([_MHD_VAR_CONTAIN],[dnl
AC_REQUIRE([AC_PROG_FGREP])dnl
AS_IF([[`echo "${]$1[}" | $FGREP ']$2[' >/dev/null 2>&1`]], [$3], [$4])
])

#
# _MHD_STRING_CONTAIN_BRE(VAR, BRE, [IF-MATCH], [IF-NOT-MATCH])
#

AC_DEFUN([_MHD_VAR_CONTAIN_BRE],[dnl
AC_REQUIRE([AC_PROG_GREP])dnl
AS_IF([[`echo "${]$1[}" | $GREP ']$2[' >/dev/null 2>&1`]], [$3], [$4])
])

# SYNOPSIS
#
# _MHD_CHECK_POSIX2008(DEFINES_VAR, FLAGS_VAR,
#                      [EXT_DEFINES],
#                      [ACTION-IF-AVAILABLE],
#                      [ACTION-IF-NOT-AVAILABLE])
#
#
AC_DEFUN([_MHD_CHECK_POSIX2008], [dnl
  AC_CACHE_CHECK([headers for POSIX.1-2008/SUSv4 features],
    [[mhd_cv_headers_posix2008]], [dnl
      _MHD_CHECK_POSIX_FEATURES_SINGLE([
_MHD_BASIC_INCLUDES
[
/* Check will be passed if ALL features are avalable 
 * and failed if ANY feature is not avalable. */
int main()
{

#ifndef stpncpy
  (void) stpncpy;
#endif
#ifndef strnlen
  (void) strnlen;
#endif

#if !defined(__NetBSD__) && !defined(__OpenBSD__)
/* NetBSD and OpenBSD didn't implement wcsnlen() for some reason. */
#ifndef wcsnlen
  (void) wcsnlen;
#endif
#endif

#ifdef __CYGWIN__
/* The only depend function on Cygwin, but missing on some other platforms */
#ifndef strndup
  (void) strndup;
#endif
#endif

#ifndef __sun
/* illumos forget to uncomment some _XPG7 macros. */
#ifndef renameat
  (void) renameat;
#endif

#ifndef getline
  (void) getline;
#endif
#endif /* ! __sun */

/* gmtime_r() becomes mandatory only in POSIX.1-2008. */
#ifndef gmtime_r
  (void) gmtime_r;
#endif

/* unsetenv() actually defined in POSIX.1-2001 so it
 * must be present with _XOPEN_SOURCE == 700 too. */
#ifndef unsetenv
  (void) unsetenv;
#endif

  return 0;
}
        ]],
        [$3], [[700]],
        [[mhd_cv_headers_posix2008]]dnl
      )
    ]
  )
  _MHD_VAR_CONTAIN_BRE([[mhd_cv_headers_posix2008]], [[^available]],
    [
      _MHD_VAR_CONTAIN([[mhd_cv_headers_posix2008]], [[does not work with _XOPEN_SOURCE]], [[:]],
        [_MHD_SYS_EXT_VAR_ADD_FLAG([$1],[$2],[[_XOPEN_SOURCE]],[[700]])]
      )
      $4
    ],
    [$5]
  )
])


# SYNOPSIS
#
# _MHD_CHECK_POSIX2001(DEFINES_VAR, FLAGS_VAR,
#                      [EXT_DEFINES],
#                      [ACTION-IF-AVAILABLE],
#                      [ACTION-IF-NOT-AVAILABLE])
#
#
AC_DEFUN([_MHD_CHECK_POSIX2001], [dnl
  AC_CACHE_CHECK([headers for POSIX.1-2001/SUSv3 features],
    [[mhd_cv_headers_posix2001]], [
      _MHD_CHECK_POSIX_FEATURES_SINGLE([
_MHD_BASIC_INCLUDES
[
/* Check will be passed if ALL features are available
 * and failed if ANY feature is not available. */
int main()
{

#ifndef setenv
  (void) setenv;
#endif

#ifndef __NetBSD__
#ifndef vsscanf
  (void) vsscanf;
#endif
#endif

/* Availability of next features varies, but they all must be present
 * on platform with support for _XOPEN_SOURCE = 600. */

/* vsnprintf() should be available with _XOPEN_SOURCE >= 500, but some platforms
 * provide it only with _POSIX_C_SOURCE >= 200112 (autodefined when
 * _XOPEN_SOURCE >= 600) where specification of vsnprintf() is aligned with
 * ISO C99 while others platforms defined it with even earlier standards. */
#ifndef vsnprintf
  (void) vsnprintf;
#endif

/* On platforms that prefer POSIX over X/Open, fseeko() is available
 * with _POSIX_C_SOURCE >= 200112 (autodefined when _XOPEN_SOURCE >= 600).
 * On other platforms it should be available with _XOPEN_SOURCE >= 500. */
#ifndef fseeko
  (void) fseeko;
#endif

/* F_GETOWN must be defined with _XOPEN_SOURCE >= 600, but some platforms
 * define it with _XOPEN_SOURCE >= 500. */
#ifndef F_GETOWN
#error F_GETOWN is not defined
choke me now;
#endif
  return 0;
}
        ]],
        [$3],[[600]],[[mhd_cv_headers_posix2001]]dnl
      )
    ]
  )
  _MHD_VAR_CONTAIN_BRE([[mhd_cv_headers_posix2001]], [[^available]],
    [
      _MHD_VAR_CONTAIN([[mhd_cv_headers_posix2001]], [[does not work with _XOPEN_SOURCE]], [[:]],
        [_MHD_SYS_EXT_VAR_ADD_FLAG([$1],[$2],[[_XOPEN_SOURCE]],[[600]])]
      )
      $4
    ],
    [$5]
  )
])


# SYNOPSIS
#
# _MHD_CHECK_SUSV2(DEFINES_VAR, FLAGS_VAR,
#                  [EXT_DEFINES],
#                  [ACTION-IF-AVAILABLE],
#                  [ACTION-IF-NOT-AVAILABLE])
#
#
AC_DEFUN([_MHD_CHECK_SUSV2], [dnl
  AC_CACHE_CHECK([headers for SUSv2/XPG5 features],
    [[mhd_cv_headers_susv2]], [
      _MHD_CHECK_POSIX_FEATURES_SINGLE([
_MHD_BASIC_INCLUDES
[
/* Check will be passed if ALL features are available
 * and failed if ANY feature is not available. */
int main()
{
/* It's not easy to write reliable test for _XOPEN_SOURCE = 500 as
 * platforms not always precisely follow this standard and some
 * functions are already deprecated in later standards. */

/* Availability of next features varies, but they all must be present
 * on platform with correct support for _XOPEN_SOURCE = 500. */

/* Mandatory with _XOPEN_SOURCE >= 500 but as XSI extension available
 * with much older standards. */
#ifndef ftruncate
  (void) ftruncate;
#endif

/* Added with _XOPEN_SOURCE >= 500 but was available in some standards
 * before. XSI extension. */
#ifndef pread
  (void) pread;
#endif

#ifndef __APPLE__
/* Actually comes from XPG4v2 and must be available
 * with _XOPEN_SOURCE >= 500 as well. */
#ifndef symlink
  (void) symlink;
#endif

/* Actually comes from XPG4v2 and must be available
 * with _XOPEN_SOURCE >= 500 as well. XSI extension. */
#ifndef strdup
  (void) strdup;
#endif
#endif /* ! __APPLE__ */
  return 0;
}
        ]],
        [$3],[[500]],[[mhd_cv_headers_susv2]]dnl
      )
    ]
  )
  _MHD_VAR_CONTAIN_BRE([[mhd_cv_headers_susv2]], [[^available]],
    [
      _MHD_VAR_CONTAIN([[mhd_cv_headers_susv2]], [[does not work with _XOPEN_SOURCE]], [[:]],
        [_MHD_SYS_EXT_VAR_ADD_FLAG([$1],[$2],[[_XOPEN_SOURCE]],[[500]])]
      )
      $4
    ],
    [$5]
  )
])


# SYNOPSIS
#
# _MHD_CHECK_POSIX_FEATURES_SINGLE(FEATURES_TEST,
#                                  EXT_DEFINES,
#                                  _XOPEN_SOURCE-VALUE,
#                                  VAR-RESULT)

AC_DEFUN([_MHD_CHECK_POSIX_FEATURES_SINGLE], [dnl
  AS_VAR_PUSHDEF([avail_Var], [mhd_cpsxfs_tmp_features_available])dnl
  AS_VAR_PUSHDEF([ext_Var], [mhd_cpsxfs_tmp_extentions_allowed])dnl
  AS_VAR_PUSHDEF([xopen_Var], [mhd_cpsxfs_tmp_src_xopen_allowed])dnl
  AS_VAR_PUSHDEF([dislb_Var], [mhd_cpsxfs_tmp_features_disableable])dnl
  AS_UNSET([avail_Var])
  AS_UNSET([ext_Var])
  AS_UNSET([xopen_Var])
  AS_UNSET([dislb_Var])
  _MHD_CHECK_POSIX_FEATURES([$1], [$2], [$3], [avail_Var], [ext_Var], [xopen_Var], [dislb_Var])
  AS_VAR_IF([avail_Var], ["yes"],
    [
      AS_VAR_IF([dislb_Var], ["yes"],
        [
          AS_VAR_IF([ext_Var], ["required"],
            [
              AS_VAR_IF([xopen_Var], ["allowed"],
                [
                  AS_VAR_SET(m4_normalize([$4]), ["available, activated by extension macro, works with _XOPEN_SOURCE=]m4_normalize([$3])["])
                ],
                [
                  AS_VAR_SET(m4_normalize([$4]), ["available, activated by extension macro, does not work with _XOPEN_SOURCE=]m4_normalize([$3])["])
                ]
              )
            ],
            [
              AS_VAR_IF([ext_Var], ["allowed"],
                [
                  AS_VAR_IF([xopen_Var], ["required"],
                    [
                      AS_VAR_SET(m4_normalize([$4]), ["available, activated by _XOPEN_SOURCE=]m4_normalize([$3])[, works with extension macro"])
                    ],
                    [
                      AS_VAR_IF([xopen_Var], ["allowed"],
                        [
                          AS_VAR_SET(m4_normalize([$4]), ["available, works with _XOPEN_SOURCE=]m4_normalize([$3])[, works with extension macro"])
                        ],
                        [
                          AS_VAR_SET(m4_normalize([$4]), ["available, works with extension macro, does not work with _XOPEN_SOURCE=]m4_normalize([$3])["])
                        ]
                      )
                    ]
                  )
                ],
                [
                  AS_VAR_IF([xopen_Var], ["required"],
                    [
                      AS_VAR_SET(m4_normalize([$4]), ["available, activated by _XOPEN_SOURCE=]m4_normalize([$3])[, does not work with extension macro"])
                    ],
                    [
                      AS_VAR_IF([xopen_Var], ["allowed"],
                        [
                          AS_VAR_SET(m4_normalize([$4]), ["available, works with _XOPEN_SOURCE=]m4_normalize([$3])[, does not work with extension macro"])
                        ],
                        [
                          AS_VAR_SET(m4_normalize([$4]), ["available, does not work with _XOPEN_SOURCE=]m4_normalize([$3])[, does not work with extension macro"])
                        ]
                      )
                    ]
                  )
                ]
              )
            ]
          )
        ],
        [
          AS_VAR_SET(m4_normalize([$4]), ["available, works always"])
        ]
      )
    ],
    [
      AS_VAR_SET(m4_normalize([$4]), ["not available"])
    ]
  )
  AS_UNSET([dislb_Var])
  AS_UNSET([xopen_Var])
  AS_UNSET([ext_Var])
  AS_UNSET([avail_Var])
  AS_VAR_POPDEF([dislb_Var])dnl
  AS_VAR_POPDEF([xopen_Var])dnl
  AS_VAR_POPDEF([ext_Var])dnl
  AS_VAR_POPDEF([avail_Var])dnl
])

# SYNOPSIS
#
# _MHD_CHECK_POSIX_FEATURES(FEATURES_TEST, EXT_DEFINES, _XOPEN_SOURCE-VALUE,
#                           [VAR-FEATURES-AVAILABLE-YES_NO],
#                           [VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED],
#                           [VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED],
#                           [VAR-FEATURES-DISABLEABLE-YES_NO])

AC_DEFUN([_MHD_CHECK_POSIX_FEATURES], [dnl
  AS_VAR_PUSHDEF([src_Var], [mhd_cpsxf_tmp_src_variable])dnl
  AS_VAR_PUSHDEF([defs_Var], [mhd_cpsxf_tmp_defs_variable])dnl
  AS_VAR_SET([src_Var],["
$1
"])
  dnl To reduce 'configure' size
  AS_VAR_SET([defs_Var],["
$2
"])
  dnl To reduce 'configure' size

  dnl Some platforms enable most features when no
  dnl specific mode is requested by macro.
  dnl Check whether features test works without _XOPEN_SOURCE and
  dnl with disabled extensions (undefined most of
  dnl predefined macros for specific requested mode).
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
_MHD_UNDEF_ALL_EXT
$src_Var
    ])],
    [dnl
      _AS_ECHO_LOG([[Checked features work with undefined all extensions and without _XOPEN_SOURCE]])
      AS_VAR_SET(m4_normalize([$4]),["yes"]) # VAR-FEATURES-AVAILABLE-YES_NO

      dnl Check whether features will work extensions
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$defs_Var
$src_Var
        ])],
        [dnl
          _AS_ECHO_LOG([[Checked features work with extensions]])
          AS_VAR_SET(m4_normalize([$5]),["allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
          dnl Check whether features will work with _XOPEN_SOURCE
          AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$defs_Var
[#define _XOPEN_SOURCE ]]m4_normalize([$3])[
$src_Var
            ])],
            [dnl
              _AS_ECHO_LOG([Checked features work with extensions and with _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$6]),["allowed"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
              dnl Check whether features could be disabled
              dnl Request oldest POSIX mode and strict ANSI mode
              AC_COMPILE_IFELSE([AC_LANG_SOURCE([
_MHD_UNDEF_ALL_EXT
[#define _POSIX_C_SOURCE 1]
[#define  _ANSI_SOURCE 1]
$src_Var
                ])],
                [dnl
                  _AS_ECHO_LOG([[Checked features work with disabled extensions, with _POSIX_C_SOURCE=1 and _ANSI_SOURCE=1]])
                  AS_VAR_SET(m4_normalize([$7]),["no"]) # VAR-FEATURES-DISABLEABLE-YES_NO
                ],
                [dnl
                  _AS_ECHO_LOG([[Checked features DO NOT work with disabled extensions, with _POSIX_C_SOURCE=1 and _ANSI_SOURCE=1]])
                  AS_VAR_SET(m4_normalize([$7]),["yes"]) # VAR-FEATURES-DISABLEABLE-YES_NO
                ]
              )
            ],
            [dnl
              _AS_ECHO_LOG([Checked features DO NOT work with extensions and with _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$6]),["not allowed"])  # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
              AS_VAR_SET(m4_normalize([$7]),["yes"]) # VAR-FEATURES-DISABLEABLE-YES_NO
            ]
          )
        ],
        [dnl
          _AS_ECHO_LOG([[Checked features DO NOT work with extensions]])
          AS_VAR_SET(m4_normalize([$7]),["yes"]) # VAR-FEATURES-DISABLEABLE-YES_NO
          dnl Check whether features work with _XOPEN_SOURCE
          AC_COMPILE_IFELSE([AC_LANG_SOURCE([
[#define _XOPEN_SOURCE ]m4_normalize($3)
$src_Var
            ])],
            [dnl
              _AS_ECHO_LOG([Checked features work with _XOPEN_SOURCE=]m4_normalize([$3])[])
              dnl Check default state (without enabling/disabling)
              AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$src_Var
                ])],
                [dnl
                  _AS_ECHO_LOG([[Checked features work by default]])
                  AS_VAR_SET(m4_normalize([$6]),["allowed"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
                ],
                [dnl
                  _AS_ECHO_LOG([[Checked features DO NOT by default]])
                  AS_VAR_SET(m4_normalize([$6]),["required"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
                ]
              )
              dnl Check whether features work with _XOPEN_SOURCE and extensions
              AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$defs_Var
[#define _XOPEN_SOURCE] ]m4_normalize([$3])[
$src_Var
                ])],
                [dnl
                  _AS_ECHO_LOG([[Checked features work with _XOPEN_SOURCE and extensions]])
                  AS_VAR_SET(m4_normalize([$5]),["allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
                ],
                [dnl
                  _AS_ECHO_LOG([[Checked features DO NOT work with _XOPEN_SOURCE and extensions]])
                  AS_VAR_SET(m4_normalize([$5]),["not allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
                ]
              )
            ],
            [dnl
              _AS_ECHO_LOG([Checked features DO NOT work with _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$5]),["not allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
              AS_VAR_SET(m4_normalize([$6]),["not allowed"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
            ]
          )
        ]
      )
    ],
    [dnl
      _AS_ECHO_LOG([[Checked features DO NOT work with undefined all extensions and without _XOPEN_SOURCE]])
      dnl Let's find the way to enable POSIX features
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$defs_Var
$src_Var
        ])],
        [dnl
          _AS_ECHO_LOG([[Checked features work with extensions]])
          AS_VAR_SET(m4_normalize([$4]),["yes"]) # VAR-FEATURES-AVAILABLE-YES_NO
          AS_VAR_SET(m4_normalize([$7]),["yes"]) # VAR-FEATURES-DISABLEABLE-YES_NO
          dnl Check default state (without enabling/disabling)
          AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$src_Var
            ])],
            [dnl
              _AS_ECHO_LOG([[Checked features work by default]])
              AS_VAR_SET(m4_normalize([$5]),["allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
            ],
            [dnl
              _AS_ECHO_LOG([[Checked features DO NOT by default]])
              AS_VAR_SET(m4_normalize([$5]),["required"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
            ]
          )
          dnl Check whether features work with extensions and _XOPEN_SOURCE
          AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$defs_Var
[#define _XOPEN_SOURCE] ]m4_normalize([$3])[
$src_Var
            ])],
            [dnl
              _AS_ECHO_LOG([Checked features work with extensions and _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$6]),["allowed"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
            ],
            [dnl
              _AS_ECHO_LOG([Checked features DO NOT work with extensions and _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$6]),["not allowed"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
            ]
          )
        ],
        [dnl
          _AS_ECHO_LOG([[Checked features DO NOT work with extensions]])
          dnl Check whether features work with _XOPEN_SOURCE
          AC_COMPILE_IFELSE([AC_LANG_SOURCE([
[#define _XOPEN_SOURCE] ]m4_normalize([$3])[
$src_Var
            ])],
            [dnl
              _AS_ECHO_LOG([Checked features work with _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$4]),["yes"]) # VAR-FEATURES-AVAILABLE-YES_NO
              dnl Check default state (without enabling/disabling)
              AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$src_Var
                ])],
                [dnl
                  _AS_ECHO_LOG([[Checked features work by default]])
                  AS_VAR_SET(m4_normalize([$6]),["allowed"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
                ],
                [dnl
                  _AS_ECHO_LOG([[Checked features DO NOT by default]])
                  AS_VAR_SET(m4_normalize([$6]),["required"]) # VAR-XOPEN-REQUIRED_NOT-ALLOWED_ALLOWED
                ]
              )
              dnl Check whether features work with _XOPEN_SOURCE and extensions
              AC_COMPILE_IFELSE([AC_LANG_SOURCE([
$defs_Var
[#define _XOPEN_SOURCE] ]m4_normalize([$3])[
$src_Var
                ])],
                [dnl
                  _AS_ECHO_LOG([Checked features work with _XOPEN_SOURCE=]m4_normalize([$3])[ and extensions])
                  AS_VAR_SET(m4_normalize([$5]),["allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
                ],
                [dnl
                  _AS_ECHO_LOG([Checked features DO NOT work with _XOPEN_SOURCE=]m4_normalize([$3])[ and extensions])
                  AS_VAR_SET(m4_normalize([$5]),["not allowed"]) # VAR-EXTENSIONS-REQUIRED_NOT-ALLOWED_ALLOWED
                ]
              )
            ],
            [dnl
              _AS_ECHO_LOG([Checked features DO NOT work with _XOPEN_SOURCE=]m4_normalize([$3])[])
              AS_VAR_SET(m4_normalize([$4]),["no"]) # VAR-FEATURES-AVAILABLE-YES_NO
            ]
          )
        ]
      )
    ]
  )
  AS_UNSET([defs_Var])
  AS_UNSET([src_Var])
  AS_VAR_POPDEF([defs_Var])dnl
  AS_VAR_POPDEF([src_Var])dnl
])


#
# MHD_CHECK_HEADER_PRESENCE(headername.h)
#
# Check only by preprocessor whether header file is present.

AC_DEFUN([MHD_CHECK_HEADER_PRESENCE], [dnl
  AC_PREREQ([2.64])dnl for AS_VAR_PUSHDEF, AS_VAR_SET
  AS_VAR_PUSHDEF([mhd_cache_Var],[mhd_cv_header_[]$1[]_present])dnl
  AC_CACHE_CHECK([for presence of $1], [mhd_cache_Var], [dnl
    dnl Hack autoconf to get pure result of only single header presence
    cat > conftest.$ac_ext <<_ACEOF
@%:@include <[]$1[]>
_ACEOF
    AC_PREPROC_IFELSE([],
      [AS_VAR_SET([mhd_cache_Var],[["yes"]])],
      [AS_VAR_SET([mhd_cache_Var],[["no"]])]
    )
    rm -f conftest.$ac_ext
  ])
  AS_VAR_POPDEF([mhd_cache_Var])dnl
])


#
# MHD_CHECK_HEADERS_PRESENCE(oneheader.h otherheader.h ...)
#
# Check each specified header in whitespace-separated list for presence.

AC_DEFUN([MHD_CHECK_HEADERS_PRESENCE], [dnl
  AC_PREREQ([2.60])dnl for m4_foreach_w
  m4_foreach_w([mhd_chk_Header], [$1],
    [MHD_CHECK_HEADER_PRESENCE(m4_defn([mhd_chk_Header]))]
  )dnl
])


#
# MHD_CHECK_HEADERS_PRESENCE_COMPACT(oneheader.h otherheader.h ...)
#
# Same as MHD_CHECK_HEADERS_PRESENCE, but a bit slower and produce more compact 'configure'.

AC_DEFUN([MHD_CHECK_HEADERS_PRESENCE_COMPACT], [dnl
  for mhd_chk_Header in $1 ; do
    MHD_CHECK_HEADER_PRESENCE([[${mhd_chk_Header}]])
  done
])


#
# MHD_CHECK_BASIC_HEADERS_PRESENCE
#
# Check basic headers for presence.

AC_DEFUN([MHD_CHECK_BASIC_HEADERS_PRESENCE], [dnl
  MHD_CHECK_HEADERS_PRESENCE([stdio.h wchar.h stdlib.h string.h strings.h stdint.h fcntl.h sys/types.h time.h unistd.h])
])


#
# _MHD_SET_BASIC_INCLUDES
#
# Internal preparatory macro.

AC_DEFUN([_MHD_SET_BASIC_INCLUDES], [dnl
  AC_REQUIRE([MHD_CHECK_BASIC_HEADERS_PRESENCE])dnl
  AS_IF([[test -z "$mhd_basic_headers_includes"]], [dnl
    AS_VAR_IF([mhd_cv_header_stdio_h_present], [["yes"]],
      [[mhd_basic_headers_includes="\
#include <stdio.h>
"     ]],[[mhd_basic_headers_includes=""]]
    )
    AS_VAR_IF([mhd_cv_header_sys_types_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <sys/types.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_wchar_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <wchar.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_stdlib_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <stdlib.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_string_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <string.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_strings_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <strings.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_stdint_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <stdint.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_fcntl_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <fcntl.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_time_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <time.h>
"     ]]
    )
    AS_VAR_IF([mhd_cv_header_unistd_h_present], [["yes"]],
      [[mhd_basic_headers_includes="${mhd_basic_headers_includes}\
#include <unistd.h>
"     ]]
    )
  ])dnl
])


#
# _MHD_BASIC_INCLUDES
#
# Internal macro. Output set of basic includes.

AC_DEFUN([_MHD_BASIC_INCLUDES], [AC_REQUIRE([_MHD_SET_BASIC_INCLUDES])dnl
[ /* Start of MHD basic test includes */
$mhd_basic_headers_includes /* End of MHD basic test includes */
]])


#
# MHD_CHECK_BASIC_HEADERS([PROLOG], [ACTION-IF-OK], [ACTION-IF-FAIL])
#
# Check whether basic headers can be compiled with specified prolog.

AC_DEFUN([MHD_CHECK_BASIC_HEADERS], [dnl
  AC_COMPILE_IFELSE([dnl
    AC_LANG_PROGRAM([m4_n([$1])dnl
_MHD_BASIC_INCLUDES
    ], [[int i = 1; i++; if(i) return i]])
  ], [$2], [$3])
])


#
# _MHD_SET_UNDEF_ALL_EXT
#
# Internal preparatory macro.

AC_DEFUN([_MHD_SET_UNDEF_ALL_EXT], [m4_divert_text([INIT_PREPARE],[dnl
[mhd_undef_all_extensions="
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#ifdef _XOPEN_SOURCE_EXTENDED
#undef _XOPEN_SOURCE_EXTENDED
#endif
#ifdef _XOPEN_VERSION
#undef _XOPEN_VERSION
#endif
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#ifdef _POSIX_SOURCE
#undef _POSIX_SOURCE
#endif
#ifdef _DEFAULT_SOURCE
#undef _DEFAULT_SOURCE
#endif
#ifdef _BSD_SOURCE
#undef _BSD_SOURCE
#endif
#ifdef _SVID_SOURCE
#undef _SVID_SOURCE
#endif
#ifdef __EXTENSIONS__
#undef __EXTENSIONS__
#endif
#ifdef _ALL_SOURCE
#undef _ALL_SOURCE
#endif
#ifdef _TANDEM_SOURCE
#undef _TANDEM_SOURCE
#endif
#ifdef _DARWIN_C_SOURCE
#undef _DARWIN_C_SOURCE
#endif
#ifdef __BSD_VISIBLE
#undef __BSD_VISIBLE
#endif
#ifdef _NETBSD_SOURCE
#undef _NETBSD_SOURCE
#endif
"
]])])


#
# _MHD_UNDEF_ALL_EXT
#
# Output prolog that undefine all known extension and visibility macros.

AC_DEFUN([_MHD_UNDEF_ALL_EXT], [dnl
AC_REQUIRE([_MHD_SET_UNDEF_ALL_EXT])dnl
$mhd_undef_all_extensions
])


#
# _MHD_CHECK_DEFINED(SYMBOL, [PROLOG],
#                    [ACTION-IF-DEFINED], [ACTION-IF-NOT-DEFINED])
#
# Silently checks for defined symbols.

AC_DEFUN([_MHD_CHECK_DEFINED], [dnl
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
m4_n([$2])dnl
[#ifndef ]$1[
#error ]$1[ is not defined
choke me now;
#endif
        ]],[])
    ], [m4_default_nblank([$3])], [m4_default_nblank([$4])]
  )
])


#
# MHD_CHECK_DEFINED(SYMBOL, [PROLOG],
#                   [ACTION-IF-DEFINED], [ACTION-IF-NOT-DEFINED],
#                   [MESSAGE])
#
# Cache-check for defined symbols with printing results.

AC_DEFUN([MHD_CHECK_DEFINED], [dnl
  AS_VAR_PUSHDEF([mhd_cache_Var],[mhd_cv_macro_[]m4_tolower(m4_normalize($1))_defined])dnl
  AC_CACHE_CHECK([dnl
m4_ifnblank([$5], [$5], [whether ]m4_normalize([$1])[ is already defined])],
    [mhd_cache_Var], [dnl
    _MHD_CHECK_DEFINED(m4_normalize([$1]), [$2],
      [mhd_cache_Var="yes"],
      [mhd_cache_Var="no"]
    )
  ])
  _MHD_VAR_IF([mhd_cache_Var], [["yes"]], [$3], [$4])
  AS_VAR_POPDEF([mhd_cache_Var])dnl
])


#
# MHD_CHECK_DEFINED_MSG(SYMBOL, [PROLOG], [MESSAGE]
#                       [ACTION-IF-DEFINED], [ACTION-IF-NOT-DEFINED])
#
# Cache-check for defined symbols with printing results.
# Reordered arguments for better readability.

AC_DEFUN([MHD_CHECK_DEFINED_MSG],[dnl
MHD_CHECK_DEFINED([$1],[$2],[$4],[$5],[$3])])

#
# MHD_CHECK_ACCEPT_DEFINE(DEFINE-SYMBOL, [DEFINE-VALUE = 1], [PROLOG],
#                         [ACTION-IF-ACCEPTED], [ACTION-IF-NOT-ACCEPTED],
#                         [MESSAGE])
#
# Cache-check whether specific defined symbol do not break basic headers.

AC_DEFUN([MHD_CHECK_ACCEPT_DEFINE], [dnl
  AC_PREREQ([2.64])dnl for AS_VAR_PUSHDEF, AS_VAR_SET, m4_ifnblank
  AS_VAR_PUSHDEF([mhd_cache_Var],
    [mhd_cv_define_[]m4_tolower(m4_normalize($1))[]_accepted[]m4_ifnblank([$2],[_[]$2])])dnl
  AC_CACHE_CHECK([dnl
m4_ifnblank([$6],[$6],[whether headers accept $1[]m4_ifnblank([$2],[[ with value ]$2])])],
    [mhd_cache_Var], [dnl
    MHD_CHECK_BASIC_HEADERS([
m4_n([$3])[#define ]$1 m4_default_nblank($2,[[1]])],
      [mhd_cache_Var="yes"], [mhd_cache_Var="no"]
    )
  ])
  _MHD_VAR_IF([mhd_cache_Var], [["yes"]], [$4], [$5])
  AS_VAR_POPDEF([mhd_cache_Var])dnl
])


#
# MHD_CHECK_DEF_AND_ACCEPT(DEFINE-SYMBOL, [DEFINE-VALUE = 1], [PROLOG],
#                         [ACTION-IF-DEFINED],
#                         [ACTION-IF-ACCEPTED], [ACTION-IF-NOT-ACCEPTED])
#
# Combination of MHD_CHECK_DEFINED_ECHO and MHD_CHECK_ACCEPT_DEFINE.
# First check whether symbol is already defined and, if not defined,
# checks whether it can be defined.

AC_DEFUN([MHD_CHECK_DEF_AND_ACCEPT], [dnl
  MHD_CHECK_DEFINED([$1], [$3], [$4], [dnl
    MHD_CHECK_ACCEPT_DEFINE([$1], [$2], [$3], [$5], [$6])dnl
  ])dnl
])

#
# _MHD_XOPEN_VAR_ADD(DEFINES_VAR, FLAGS_VAR, [PROLOG])
#
# Internal macro. Only to be used in MHD_SYS_EXT.

AC_DEFUN([_MHD_XOPEN_VAR_ADD], [dnl
  MHD_CHECK_DEF_AND_ACCEPT([[_XOPEN_SOURCE]], [[1]], [$3],
    [
      AC_CACHE_CHECK([[whether predefined value of _XOPEN_SOURCE is more or equal 500]],
        [[mhd_cv_macro__xopen_source_def_fiveh]], [dnl
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
m4_n([$3])dnl
[#if _XOPEN_SOURCE+0 < 500
#error Value of _XOPEN_SOURCE is less than 500
choke me now;
#endif
              ]],[[int i = 0; i++; if(i) return i]])],
            [[mhd_cv_macro__xopen_source_def_fiveh="yes"]],
            [[mhd_cv_macro__xopen_source_def_fiveh="no"]]
          )dnl
        ]
      )dnl
    ],
    [_MHD_SYS_EXT_VAR_ADD_FLAG([$1], [$2], [[_XOPEN_SOURCE]], [[1]])]
  )
  AS_IF([[test "x${mhd_cv_define__xopen_source_accepted_1}" = "xyes" || \
          test "x${mhd_cv_macro__xopen_source_def_fiveh}" = "xno"]],
    [
      MHD_CHECK_DEF_AND_ACCEPT([[_XOPEN_SOURCE_EXTENDED]], [],
        [
m4_n([$3])], [],
      [dnl
        _MHD_SYS_EXT_VAR_ADD_FLAG([$1],[$2],[[_XOPEN_SOURCE_EXTENDED]])
      ], [dnl
        MHD_CHECK_DEFINED([[_XOPEN_VERSION]],
          [
m4_n([$3])], [],
        [dnl
          AC_CACHE_CHECK([[for value of _XOPEN_VERSION accepted by headers]],
            [mhd_cv_define__xopen_version_accepted], [dnl
            MHD_CHECK_BASIC_HEADERS([
m4_n([$3])
[#define _XOPEN_VERSION 4]],
              [[mhd_cv_define__xopen_version_accepted="4"]],
            [
              MHD_CHECK_BASIC_HEADERS([
m4_n([$3])
[#define _XOPEN_VERSION 3]],
                [[mhd_cv_define__xopen_version_accepted="3"]],
                [[mhd_cv_define__xopen_version_accepted="no"]]
              )
            ])
          ])
          AS_VAR_IF([mhd_cv_define__xopen_version_accepted], [["no"]],
            [[:]],
          [dnl
            _MHD_SYS_EXT_VAR_ADD_FLAG([$1],[$2],[[_XOPEN_VERSION]],
              [[${mhd_cv_define__xopen_version_accepted}]]dnl
            )
          ])
        ])
      ])
    ]
  )dnl
])

