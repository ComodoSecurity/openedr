# SYNOPSIS
#
#   MHD_CHECK_SOCKET_SHUTDOWN_TRIGGER([ACTION-IF-TRIGGER], [ACTION-IF-NOT],
#                                     [ACTION-IF-UNKNOWN])
#
# DESCRIPTION
#
#   Check whether shutdown of listen socket triggers waiting select().
#   If cross-compiling, result may be unknown (third action).
#   Result is cached in $mhd_cv_host_shtdwn_trgr_select variable.
#
# LICENSE
#
#   Copyright (c) 2017 Karlson2k (Evgeny Grin) <k2k@narod.ru>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 2

AC_DEFUN([MHD_CHECK_SOCKET_SHUTDOWN_TRIGGER],[dnl
  AC_PREREQ([2.64])dnl
  AC_REQUIRE([AC_CANONICAL_HOST])dnl
  AC_REQUIRE([AC_PROG_CC])dnl
  AC_REQUIRE([AX_PTHREAD])dnl
  AC_CHECK_HEADERS([sys/time.h],[AC_CHECK_FUNCS([gettimeofday])],[], [AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADERS([time.h],[AC_CHECK_FUNCS([[nanosleep]])],[], [AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADERS([unistd.h],[AC_CHECK_FUNCS([usleep])],[], [AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADERS([string.h sys/types.h sys/socket.h netinet/in.h time.h sys/select.h netinet/tcp.h],[],[], [AC_INCLUDES_DEFAULT])
  AC_CACHE_CHECK([[whether shutdown of listen socket trigger select()]],
    [[mhd_cv_host_shtdwn_trgr_select]], [dnl
    _MHD_OS_KNOWN_SOCKET_SHUTDOWN_TRIGGER([[mhd_cv_host_shtdwn_trgr_select]])
    AS_VAR_IF([mhd_cv_host_shtdwn_trgr_select], [["maybe"]],
      [_MHD_RUN_CHECK_SOCKET_SHUTDOWN_TRIGGER([[mhd_cv_host_shtdwn_trgr_select]])])
    ]
  )
  AS_IF([[test "x$mhd_cv_host_shtdwn_trgr_select" = "xyes"]], [$1],
    [[test "x$mhd_cv_host_shtdwn_trgr_select" = "xno"]], [$2], [$3])
  ]
)

#
# _MHD_OS_KNOWN_SOCKET_SHUTDOWN_TRIGGER(VAR)
#
# Sets VAR to 'yes', 'no' or 'maybe'.

AC_DEFUN([_MHD_OS_KNOWN_SOCKET_SHUTDOWN_TRIGGER],[dnl
[#] On Linux shutdown of listen socket always trigger select().
[#] On Windows select() always ignore shutdown of listen socket.
[#] On other paltforms result may vary depending on platform version.
  AS_CASE([[$host_os]],
    [[linux | linux-* | *-linux | *-linux-*]], [$1='yes'],
    [[mingw*]], [$1='no'],
    [[cygwin* | msys*]], [$1='no'],
    [[winnt* | interix*]], [$1='no'],
    [[mks]], [$1='no'],
    [[uwin]], [$1='no'],
    [$1='maybe']
  )
  ]
)

#
# _MHD_RUN_CHECK_SOCKET_SHUTDOWN_TRIGGER(VAR)
#
# Sets VAR to 'yes', 'no' or 'guessing no'.

AC_DEFUN([_MHD_RUN_CHECK_SOCKET_SHUTDOWN_TRIGGER],[dnl
  AC_LANG_PUSH([C])
  MHD_CST_SAVE_CC="$CC"
  MHD_CST_SAVE_CFLAGS="$CFLAGS"
  MHD_CST_SAVE_LIBS="$LIBS"
  CC="$PTHREAD_CC"
  CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
  LIBS="$PTHREAD_LIBS $LIBS"
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#  include <time.h>
#endif
#ifdef HAVE_STRING_H
#  include <string.h>
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#  ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif
#  ifdef HAVE_SYS_SOCKET_H
#    include <sys/socket.h>
#  endif
#  ifdef HAVE_NETINET_IN_H
#    include <netinet/in.h>
#  endif
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  endif
#  ifdef HAVE_SYS_SELECT_H
#    include <sys/select.h>
#  endif
#  ifdef HAVE_NETINET_TCP_H
#    include <netinet/tcp.h>
#  endif
   typedef int MHD_socket;
#  define MHD_INVALID_SOCKET (-1)
#  define MHD_POSIX_SOCKETS 1
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
   typedef SOCKET MHD_socket;
#  define MHD_INVALID_SOCKET (INVALID_SOCKET)
#  define MHD_WINSOCK_SOCKETS 1
#endif

#include <pthread.h>

   #ifndef SHUT_RD
#  define SHUT_RD 0
#endif
#ifndef SHUT_WR
#  define SHUT_WR 1
#endif
#ifndef SHUT_RDWR
#  define SHUT_RDWR 2
#endif

#ifndef NULL
#  define NULL ((void*)0)
#endif

#ifdef HAVE_GETTIMEOFDAY
#  if defined(_WIN32) && !defined(__CYGWIN__)
#    undef HAVE_GETTIMEOFDAY
#  endif
#endif


#ifdef HAVE_NANOSLEEP
static const struct timespec sm_tmout = {0, 1000};
#  define short_sleep() nanosleep(&sm_tmout, NULL)
#elif defined(HAVE_USLEEP)
#  define short_sleep() usleep(1)
#else
#  define short_sleep() (void)0
#endif

static volatile int going_select = 0;
static volatile int select_ends = 0;
static volatile int gerror = 0;
static int timeout_mils;

#ifndef HAVE_GETTIMEOFDAY
static volatile long long select_elapsed_time = 0;

static long long time_chk(void)
{
  long long ret = time(NULL);
  if (-1 == ret)
    gerror = 4;
  return ret;
}
#endif


static void* select_thrd_func(void* param)
{
#ifndef HAVE_GETTIMEOFDAY
  long long start, stop;
#endif
  fd_set rs;
  struct timeval tmot = {0, 0};
  MHD_socket fd = *((MHD_socket*)param);

  FD_ZERO(&rs);
  FD_SET(fd, &rs);
  tmot.tv_usec = timeout_mils * 1000;
#ifndef HAVE_GETTIMEOFDAY
  start = time_chk();
#endif
  going_select = 1;
  if (0 > select ((int)(fd) + 1, &rs, NULL, NULL, &tmot))
    gerror = 5;
#ifndef HAVE_GETTIMEOFDAY
  stop = time_chk();
  select_elapsed_time = stop - start;
#endif
  select_ends = 1;
  return NULL;
}


static MHD_socket create_socket(void)
{ return socket (AF_INET, SOCK_STREAM, 0); }

static void close_socket(MHD_socket fd)
{
#ifdef MHD_POSIX_SOCKETS
  close(fd);
#else
  closesocket(fd);
#endif
}

static MHD_socket
create_socket_listen(int port)
{
  MHD_socket fd;
  struct sockaddr_in sock_addr;
  fd = create_socket();
  if (MHD_INVALID_SOCKET == fd)
    return fd;

  memset (&sock_addr, 0, sizeof (struct sockaddr_in));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons(port);
  sock_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind (fd, (const struct sockaddr*) &sock_addr, sizeof(sock_addr)) < 0 ||
      listen(fd, SOMAXCONN) < 0)
    {
      close_socket(fd);
      return MHD_INVALID_SOCKET;
    }
  return fd;
}

#ifdef HAVE_GETTIMEOFDAY
#define diff_time(tv1, tv2) ((long long)(tv1.tv_sec-tv2.tv_sec)*10000 + (long long)(tv1.tv_usec-tv2.tv_usec)/100)
#else
#define diff_time(tv1, tv2) ((long long)(tv1-tv2))
#endif

static long long test_run_select(int timeout_millsec, int use_shutdown, long long delay_before_shutdown)
{
  pthread_t select_thrd;
  MHD_socket fd;
#ifdef HAVE_GETTIMEOFDAY
  struct timeval start, stop;
#else
  long long start;
#endif

  fd = create_socket_listen(0);
  if (MHD_INVALID_SOCKET == fd)
    return -7;
  going_select = 0;
  select_ends = 0;
  gerror = 0;
  timeout_mils = timeout_millsec;
  if (0 != pthread_create (&select_thrd, NULL, select_thrd_func, (void*)&fd))
    return -8;
  while (!going_select) {short_sleep();}
#ifdef HAVE_GETTIMEOFDAY
  gettimeofday (&start, NULL);
#else
  start = time_chk();
#endif
  if (use_shutdown)
    {
#ifdef HAVE_GETTIMEOFDAY
      struct timeval current;
      do {short_sleep(); gettimeofday(&current, NULL); } while (delay_before_shutdown > diff_time(current, start));
#else
      while (delay_before_shutdown > time_chk() - start) {short_sleep();}
#endif
      shutdown(fd, SHUT_RDWR);
    }
#ifdef HAVE_GETTIMEOFDAY
  while (!select_ends) {short_sleep();}
  gettimeofday (&stop, NULL);
#endif
  if (0 != pthread_join(select_thrd, NULL))
    return -9;
  close_socket(fd);
  if (gerror)
    return -10;
#ifdef HAVE_GETTIMEOFDAY
  return (long long)diff_time(stop, start);
#else
  return select_elapsed_time;
#endif
}

static int test_it(void)
{
  long long duration2;
#ifdef HAVE_GETTIMEOFDAY
  long long duration0, duration1;
  duration0 = test_run_select(0, 0, 0);
  if (0 > duration0)
    return -duration0;

  duration1 = test_run_select(50, 0, 0);
  if (0 > duration1)
    return -duration1 + 20;

  duration2 = test_run_select(500, 1, (duration0 + duration1) / 2);
  if (0 > duration2)
    return -duration2 + 40;

  if (duration1 * 2 > duration2)
    { /* Check second time to be sure. */
      duration2 = test_run_select(500, 1, (duration0 + duration1) / 2);
      if (0 > duration2)
        return -duration2 + 60;
      if (duration1 * 2 > duration2)
        return 0;
    }
#else
  duration2 = test_run_select(5000, 1, 2);
  if (0 > duration2)
    return -duration2 + 80;

  if (4 > duration2)
    { /* Check second time to be sure. */
      duration2 = test_run_select(5000, 1, 2);
      if (0 > duration2)
      return -duration2 + 100;
      if (4 > duration2)
        return 0;
    }
#endif
  return 1;
}


static int init(void)
{
#ifdef MHD_WINSOCK_SOCKETS
  WSADATA wsa_data;

  if (0 != WSAStartup(MAKEWORD(2, 2), &wsa_data) || MAKEWORD(2, 2) != wsa_data.wVersion)
    {
      WSACleanup();
      return 0;
    }
#endif /* MHD_WINSOCK_SOCKETS */
  return 1;
}

static void cleanup(void)
{
#ifdef MHD_WINSOCK_SOCKETS
  WSACleanup();
#endif /* MHD_WINSOCK_SOCKETS */
}

int main(void)
{
  int res;
  if (!init())
    return 19;

  res = test_it();

  cleanup();
  if (gerror)
    return gerror;

  return res;
}
]])], [$1='yes'], [$1='no'], [$1='guessing no'])
  CC="$MHD_CST_SAVE_CC"
  CFLAGS="$MHD_CST_SAVE_CFLAGS"
  LIBS="$MHD_CST_SAVE_LIBS"
  AS_UNSET([[MHD_CST_SAVE_CC]])
  AS_UNSET([[MHD_CST_SAVE_CFLAGS]])
  AS_UNSET([[MHD_CST_SAVE_LIBS]])
  AC_LANG_POP([C])
  ]
)
