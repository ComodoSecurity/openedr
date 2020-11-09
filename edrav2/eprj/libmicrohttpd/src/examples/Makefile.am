# This Makefile.am is in the public domain
SUBDIRS  = .

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -DDATA_DIR=\"$(top_srcdir)/src/datadir/\"

AM_CFLAGS = @LIBGCRYPT_CFLAGS@

CPU_COUNT_DEF = -DCPU_COUNT=$(CPU_COUNT)

if USE_COVERAGE
  AM_CFLAGS += --coverage
endif


# example programs
noinst_PROGRAMS = \
 benchmark \
 benchmark_https \
 chunked_example \
 minimal_example \
 dual_stack_example \
 minimal_example_comet \
 querystring_example \
 timeout \
 fileserver_example \
 fileserver_example_dirs \
 fileserver_example_external_select \
 refuse_post_example

if MHD_HAVE_EPOLL
noinst_PROGRAMS += \
  suspend_resume_epoll
endif

EXTRA_DIST = msgs_i18n.c
noinst_EXTRA_DIST = msgs_i18n.c

if ENABLE_HTTPS
noinst_PROGRAMS += https_fileserver_example
endif
if HAVE_POSTPROCESSOR
noinst_PROGRAMS += \
  post_example
if HAVE_POSIX_THREADS
noinst_PROGRAMS += demo
if ENABLE_HTTPS
noinst_PROGRAMS += demo_https
endif
endif
endif

if ENABLE_DAUTH
noinst_PROGRAMS += \
 digest_auth_example
endif

if ENABLE_BAUTH
noinst_PROGRAMS += \
 authorization_example
endif

if HAVE_POSIX_THREADS
if ENABLE_UPGRADE
noinst_PROGRAMS += \
 upgrade_example
endif
endif

if HAVE_ZLIB
noinst_PROGRAMS += \
 http_compression \
 http_chunked_compression
endif

if HAVE_W32
AM_CFLAGS += -DWINDOWS
endif

minimal_example_SOURCES = \
 minimal_example.c
minimal_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

upgrade_example_SOURCES = \
 upgrade_example.c
upgrade_example_CFLAGS = \
  $(PTHREAD_CFLAGS) $(AM_CFLAGS)
upgrade_example_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(PTHREAD_LIBS)

timeout_SOURCES = \
 timeout.c
timeout_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

chunked_example_SOURCES = \
 chunked_example.c
chunked_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

demo_SOURCES = \
 demo.c
demo_CFLAGS = \
 $(PTHREAD_CFLAGS) $(AM_CFLAGS)
demo_CPPFLAGS = \
 $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
demo_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la  \
 $(PTHREAD_LIBS)
if HAVE_LIBMAGIC
demo_LDADD += -lmagic
endif

demo_https_SOURCES = \
 demo_https.c
demo_https_CFLAGS = \
 $(PTHREAD_CFLAGS) $(AM_CFLAGS)
demo_https_CPPFLAGS = \
 $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
demo_https_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la  \
 $(PTHREAD_LIBS)
if HAVE_LIBMAGIC
demo_https_LDADD += -lmagic
endif

benchmark_SOURCES = \
 benchmark.c
benchmark_CPPFLAGS = \
 $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
benchmark_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

suspend_resume_epoll_SOURCES = \
 suspend_resume_epoll.c
suspend_resume_epoll_CPPFLAGS = \
 $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
suspend_resume_epoll_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

benchmark_https_SOURCES = \
 benchmark_https.c
benchmark_https_CPPFLAGS = \
 $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
benchmark_https_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

dual_stack_example_SOURCES = \
 dual_stack_example.c
dual_stack_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

post_example_SOURCES = \
 post_example.c
post_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

minimal_example_comet_SOURCES = \
 minimal_example_comet.c
minimal_example_comet_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

authorization_example_SOURCES = \
 authorization_example.c
authorization_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

digest_auth_example_SOURCES = \
 digest_auth_example.c
digest_auth_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

refuse_post_example_SOURCES = \
 refuse_post_example.c
refuse_post_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

querystring_example_SOURCES = \
 querystring_example.c
querystring_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

fileserver_example_SOURCES = \
 fileserver_example.c
fileserver_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

fileserver_example_dirs_SOURCES = \
 fileserver_example_dirs.c
fileserver_example_dirs_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

fileserver_example_external_select_SOURCES = \
 fileserver_example_external_select.c
fileserver_example_external_select_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

https_fileserver_example_SOURCES = \
https_fileserver_example.c
https_fileserver_example_CPPFLAGS = \
 $(AM_CPPFLAGS) $(GNUTLS_CPPFLAGS)
https_fileserver_example_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la

http_compression_SOURCES = \
 http_compression.c
http_chunked_compression_SOURCES = \
 http_chunked_compression.c
http_compression_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la
http_chunked_compression_LDADD = \
 $(top_builddir)/src/microhttpd/libmicrohttpd.la
if HAVE_ZLIB
 http_compression_LDADD += -lz
 http_chunked_compression_LDADD += -lz
endif