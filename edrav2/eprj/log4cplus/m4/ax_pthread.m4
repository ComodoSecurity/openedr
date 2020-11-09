# ===========================================================================
#        http://www.gnu.org/software/autoconf-archive/ax_pthread.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PTHREAD([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#   AX_PTHREAD_CXX([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro figures out how to build C programs using POSIX threads. It
#   sets the PTHREAD_LIBS output variable to the threads library and linker
#   flags, and the PTHREAD_CFLAGS output variable to any special C compiler
#   flags that are needed. (The user can also force certain compiler
#   flags/libs to be tested by setting these environment variables.)
#
#   NOTE: You are assumed to not only compile your program with these flags,
#   but also link it with them as well. e.g. you should link with
#   $CC $CFLAGS $PTHREAD_CFLAGS $LDFLAGS ... $PTHREAD_LIBS $LIBS
#
#   If you are only building threads programs, you may wish to use these
#   variables in your default LIBS, CFLAGS:
#
#     LIBS="$PTHREAD_LIBS $LIBS"
#     CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
#
#   In addition, if the PTHREAD_CREATE_JOINABLE thread-attribute constant
#   has a nonstandard name, defines PTHREAD_CREATE_JOINABLE to that name
#   (e.g. PTHREAD_CREATE_UNDETACHED on AIX).
#
#   Also HAVE_PTHREAD_PRIO_INHERIT is defined if pthread is found and the
#   PTHREAD_PRIO_INHERIT symbol is defined when compiling with
#   PTHREAD_CFLAGS.
#
#   ACTION-IF-FOUND is a list of shell commands to run if a threads library
#   is found, and ACTION-IF-NOT-FOUND is a list of commands to run it if it
#   is not found. If ACTION-IF-FOUND is not specified, the default action
#   will define HAVE_PTHREAD.
#
#   Please let the authors know if this macro fails on any platform, or if
#   you have any other suggestions or comments. This macro was based on work
#   by SGJ on autoconf scripts for FFTW (http://www.fftw.org/) (with help
#   from M. Frigo), as well as ac_pthread and hb_pthread macros posted by
#   Alejandro Forero Cuervo to the autoconf macro repository. We are also
#   grateful for the helpful feedback of numerous users.
#
#   Updated for Autoconf 2.68 by Daniel Richard G.
#
# LICENSE
#
#   Copyright (c) 2008 Steven G. Johnson <stevenj@alum.mit.edu>
#   Copyright (c) 2011 Daniel Richard G. <skunk@iSKUNK.ORG>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 19

AU_ALIAS([ACX_PTHREAD], [AX_PTHREAD])
AC_DEFUN([AX_PTHREAD_ALL], [
AC_REQUIRE([AC_CANONICAL_HOST])
AS_VAR_SET([ax_pthread_ok], [no])

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
AS_IF([test ! -z "$PTHREAD_LIBS$PTHREAD_$3"],
  [
        AS_VAR_COPY([save_$3], [$3])
        AS_VAR_SET([$3], ["$$3 $PTHREAD_$3"])
        AS_VAR_COPY([save_LIBS], [LIBS])
        AS_VAR_SET([LIBS], ["$PTHREAD_LIBS $LIBS"])
        AC_MSG_CHECKING(
          [for pthread_join in LIBS=$PTHREAD_LIBS with $3=$PTHREAD_$3])
        AC_TRY_LINK_FUNC([pthread_join],
          [AS_VAR_SET([ax_pthread_ok], [yes])])
        AC_MSG_RESULT([$ax_pthread_ok])
        AS_IF([test x"$ax_pthread_ok" = xno],
          [AS_UNSET([PTHREAD_LIBS])
           AS_UNSET([PTHREAD_$3])])
        AS_VAR_COPY([LIBS], [save_LIBS])
        AS_VAR_COPY([$3], [save_$3])
  ])

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

AS_VAR_SET([ax_pthread_flags], ["-Kthread -kthread lthread -pthread -pthreads -mt -mthreads --thread-safe pthread-config pthreads pthread"])

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings;
#       none will prepended at OS modifications below
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
#      ... -mt is also the pthreads flag for HP/aCC
# -mthreads: Mingw32/gcc, Lynx/gcc
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)
# pthread: Linux, etcetera
# pthreads: AIX (very old gcc's on AIX must check this before -lpthread,
#      handled below)

AS_CASE([${host_os}],
        [solaris*],
        [# On Solaris (at least, for some versions), libc contains
         # stubbed (non-functional) versions of the pthreads routines,
         # so link-based tests will erroneously succeed.  (We need to
         # link with -pthreads/-mt/ -lpthread.)  (The stubs are
         # missing pthread_cleanup_push, or rather a function called
         # by this macro, so we could check for that, but who knows
         # whether they'll stub that too in a future libc.)  So, we'll
         # just look for -pthreads and -lpthread first:
         AS_VAR_SET([ax_pthread_flags],
           ["-mt -pthread $ax_pthread_flags"])
         AS_IF([test "x$GCC" = "xyes"],
           [AS_VAR_SET([ax_pthread_flags],
              ["-pthreads pthread -pthread $ax_pthread_flags"])])],

        [darwin*],
        [AS_VAR_SET([ax_pthread_flags], ["-pthread $ax_pthread_flags"])],

        [hp-ux*],
        [# On HP-UX compiling with aCC, cc understands -mthreads as
         # '-mt' '-hreads' ..., the test succeeds but it fails to run.
         AS_IF([test x"$GCC" != xyes],
           [AS_VAR_SET([ax_pthread_flags], ["-mt $ax_pthread_flags"])])])

AS_VAR_SET([ax_pthread_flags], ["none $ax_pthread_flags"])

AS_IF([test x"$ax_pthread_ok" = xno], [
  for flag in $ax_pthread_flags; do
        AS_CASE([$flag],
                [none],
                [AC_MSG_CHECKING([whether pthreads work without any flags])],

                [-*],
                [AC_MSG_CHECKING([whether pthreads work with $flag])
                 AS_VAR_COPY([PTHREAD_$3], [flag])],

                [pthread-config],
                [AC_CHECK_PROG([ax_pthread_config], [pthread-config],
                   [yes], [no])
                 AS_IF([test x"$ax_pthread_config" = xno],
                   [continue])
                 AS_VAR_SET([PTHREAD_$3], ["`pthread-config --cflags`"])
                 AS_VAR_SET([PTHREAD_LIBS],
                   ["`pthread-config --ldflags` `pthread-config --libs`"])],

                [AC_MSG_CHECKING([for the pthreads library -l$flag])
                 AS_VAR_SET([PTHREAD_LIBS], ["-l$flag"])])

        AS_VAR_COPY([save_LIBS], [LIBS])
        AS_VAR_COPY([save_$3], [$3])
        AS_VAR_SET([LIBS], ["$PTHREAD_LIBS $LIBS"])
        AS_VAR_SET([$3], ["$$3 $PTHREAD_$3"])

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>
                        static void routine(void *a) { a = 0; }
                        static void *start_routine(void *a) { return a; }]],
                       [[pthread_t th; pthread_attr_t attr;
                        pthread_create(&th, 0, start_routine, 0);
                        pthread_join(th, 0);
                        pthread_attr_init(&attr);
                        pthread_cleanup_push(routine, 0);
                        pthread_cleanup_pop(0) /* ; */]])],
                [AS_VAR_SET([ax_pthread_ok], [yes])],
                [])

        AS_VAR_COPY([LIBS], [save_LIBS])
        AS_VAR_COPY([$3], [save_$3])

        AC_MSG_RESULT([$ax_pthread_ok])
        AS_IF([test "x$ax_pthread_ok" = xyes],
          [break])

        AS_UNSET([PTHREAD_LIBS])
        AS_UNSET([PTHREAD_$3])
  done
])

# Various other checks:
AS_IF([test "x$ax_pthread_ok" = xyes], [
        AS_VAR_COPY([save_LIBS], [LIBS])
        AS_VAR_SET([LIBS], ["$PTHREAD_LIBS $LIBS"])
        AS_VAR_COPY([save_$3], [$3])
        AS_VAR_SET([$3], ["$$3 $PTHREAD_$3"])

        # Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
        AC_MSG_CHECKING([for joinable pthread attribute])
        AS_VAR_SET([attr_name], [unknown])
        for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
            AC_LINK_IFELSE(
              [AC_LANG_PROGRAM([[#include <pthread.h>
                 ]],
                 [[int attr = $attr; return attr /* ; */]])],
              [AS_VAR_SET([attr_name], [$attr])
               break],
              [])
        done
        AC_MSG_RESULT([$attr_name])
        AS_IF([test "$attr_name" != PTHREAD_CREATE_JOINABLE],
          [AC_DEFINE_UNQUOTED([PTHREAD_CREATE_JOINABLE], [$attr_name],
             [Define to necessary symbol if this constant
              uses a non-standard name on your system.])])

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        AS_VAR_SET([flag], [no])
        AS_CASE([${host_os}],
            [aix* | freebsd* | darwin*],
            [AS_VAR_SET([flag], ["-D_THREAD_SAFE"])],

            [osf* | hpux*],
            [AS_VAR_SET([flag], ["-D_REENTRANT"])],

            [solaris*],
            [AS_IF([test "$GCC" = "yes"],
               [AS_VAR_SET([flag], ["-D_REENTRANT"])],
               [AS_VAR_SET([flag], ["-mt -D_REENTRANT"])])])

        AC_MSG_RESULT(${flag})
        AS_IF([test "x$flag" != xno],
          [AS_VAR_SET([PTHREAD_$3], ["$flag $PTHREAD_$3"])])

        AC_CACHE_CHECK([for PTHREAD_PRIO_INHERIT],
            [ax_cv_PTHREAD_PRIO_INHERIT], [
                AC_LINK_IFELSE([
                    AC_LANG_PROGRAM([[#include <pthread.h>
                      ]],
                      [[int i = PTHREAD_PRIO_INHERIT;]])],
                    [ax_cv_PTHREAD_PRIO_INHERIT=yes],
                    [ax_cv_PTHREAD_PRIO_INHERIT=no])])
        AS_IF([test "x$ax_cv_PTHREAD_PRIO_INHERIT" = "xyes"],
            [AC_DEFINE([HAVE_PTHREAD_PRIO_INHERIT], 1,
               [Have PTHREAD_PRIO_INHERIT.])])

        AS_VAR_COPY([LIBS], [save_LIBS])
        AS_VAR_COPY([$3], [save_$3])
])

AC_SUBST([PTHREAD_LIBS])
AC_SUBST([PTHREAD_$3])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
AS_IF([test x"$ax_pthread_ok" = xyes],
      [ifelse([$1],
         [],
         [AC_DEFINE([HAVE_PTHREAD], [1],
           [Define if you have POSIX threads libraries and header files.])],
         $1)],
      [AS_VAR_SET([ax_pthread_ok], [no])]
      $2)
])dnl AX_PTHREAD_ALL


AC_DEFUN([AX_PTHREAD_CXX],[dnl
  AC_LANG_PUSH([C++])
  AX_PTHREAD_ALL([$1], [$2], [CXXFLAGS])
  AC_LANG_POP([C++])])


AC_DEFUN([AX_PTHREAD],[dnl
  AC_LANG_PUSH([C])
  AX_PTHREAD_ALL([$1], [$2], [CFLAGS])
  AC_LANG_POP([C])])
