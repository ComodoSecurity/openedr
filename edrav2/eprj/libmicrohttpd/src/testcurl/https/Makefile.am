# This Makefile.am is in the public domain
SUBDIRS = .

if USE_COVERAGE
  AM_CFLAGS = --coverage
endif

if HAVE_GNUTLS_SNI
  TEST_HTTPS_SNI = test_https_sni
endif

if HAVE_POSIX_THREADS
  HTTPS_PARALLEL_TESTS = \
    test_https_get_parallel \
    test_https_get_parallel_threads
endif

CPU_COUNT_DEF = -DCPU_COUNT=$(CPU_COUNT)

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/microhttpd \
  -I$(top_srcdir)/src/platform \
  $(LIBCURL_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS)

THREAD_ONLY_TESTS = \
  test_tls_options \
  test_tls_authentication \
  $(HTTPS_PARALLEL_TESTS) \
  $(TEST_HTTPS_SNI) \
  test_https_session_info \
  test_https_time_out \
  test_https_multi_daemon \
  test_https_get \
  test_empty_response

check_PROGRAMS = \
  test_https_get_select 

if USE_POSIX_THREADS
check_PROGRAMS += \
  $(THREAD_ONLY_TESTS)
endif
if USE_W32_THREADS
check_PROGRAMS += \
  $(THREAD_ONLY_TESTS)
endif

EXTRA_DIST = \
  cert.pem key.pem \
  host1.crt host1.key \
  host2.crt host2.key

TESTS = \
  $(check_PROGRAMS)


test_https_time_out_SOURCES = \
  test_https_time_out.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_time_out_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_tls_options_SOURCES = \
  test_tls_options.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_tls_options_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_https_get_parallel_SOURCES = \
  test_https_get_parallel.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_get_parallel_CPPFLAGS = \
  $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
test_https_get_parallel_CFLAGS = \
  $(PTHREAD_CFLAGS) $(AM_CFLAGS)
test_https_get_parallel_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(PTHREAD_LIBS) $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_empty_response_SOURCES = \
  test_empty_response.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_empty_response_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_https_get_parallel_threads_SOURCES = \
  test_https_get_parallel_threads.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_get_parallel_threads_CPPFLAGS = \
  $(AM_CPPFLAGS) $(CPU_COUNT_DEF)
test_https_get_parallel_threads_CFLAGS = \
  $(PTHREAD_CFLAGS) $(AM_CFLAGS)
test_https_get_parallel_threads_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(PTHREAD_LIBS) $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_tls_authentication_SOURCES = \
  test_tls_authentication.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_tls_authentication_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_https_session_info_SOURCES = \
  test_https_session_info.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_session_info_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_https_multi_daemon_SOURCES = \
  test_https_multi_daemon.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_multi_daemon_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

test_https_get_SOURCES = \
  test_https_get.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_get_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

if HAVE_GNUTLS_SNI
test_https_sni_SOURCES = \
  test_https_sni.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_sni_CPPFLAGS = \
  $(AM_CPPFLAGS) \
  -DABS_SRCDIR=\"$(abs_srcdir)\"
test_https_sni_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@
endif

test_https_get_select_SOURCES = \
  test_https_get_select.c \
  tls_test_keys.h \
  tls_test_common.h \
  tls_test_common.c
test_https_get_select_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) @LIBGCRYPT_LIBS@ @LIBCURL@

