# This Makefile.am is in the public domain
AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/microhttpd

AM_CFLAGS = $(HIDDEN_VISIBILITY_CFLAGS)

lib_LTLIBRARIES = \
  libmicrohttpd.la

noinst_DATA =
MOSTLYCLEANFILES =

if W32_SHARED_LIB_EXP
W32_MHD_LIB_LDFLAGS = -Wl,--output-def,$(lt_cv_objdir)/libmicrohttpd.def -XCClinker -static-libgcc
noinst_DATA += $(lt_cv_objdir)/libmicrohttpd.lib $(lt_cv_objdir)/libmicrohttpd.def $(lt_cv_objdir)/libmicrohttpd.exp
MOSTLYCLEANFILES += $(lt_cv_objdir)/libmicrohttpd.lib $(lt_cv_objdir)/libmicrohttpd.def $(lt_cv_objdir)/libmicrohttpd.exp

$(lt_cv_objdir)/libmicrohttpd.def: libmicrohttpd.la

$(lt_cv_objdir)/libmicrohttpd.exp: $(lt_cv_objdir)/libmicrohttpd.lib

$(lt_cv_objdir)/libmicrohttpd.lib: $(lt_cv_objdir)/libmicrohttpd.def libmicrohttpd.la $(libmicrohttpd_la_OBJECTS)
if USE_MS_LIB_TOOL
	@echo Creating $@ and libmicrohttpd.exp by $(MS_LIB_TOOL)... && \
	dll_name=`$(EGREP) -o dlname=\'.+\' libmicrohttpd.la` && \
	dll_name=$${dll_name#*\'} && dll_name=$${dll_name%\'} && test -n "$$dll_name" && \
	echo Creating $$dll_name by $(MS_LIB_TOOL).. && cd "$(lt_cv_objdir)" && \
	$(MS_LIB_TOOL) -def:libmicrohttpd.def -name:$$dll_name -out:libmicrohttpd.lib $(libmicrohttpd_la_OBJECTS:.lo=.o) && cd ..
else
	@echo Creating $@ and libmicrohttpd.exp by $(DLLTOOL)... && \
	dll_name=`$(EGREP) -o dlname=\'.+\' libmicrohttpd.la` && \
	dll_name=$${dll_name#*\'} && dll_name=$${dll_name%\'} && test -n "$$dll_name" && \
	echo Creating $$dll_name by $(DLLTOOL).. && cd "$(lt_cv_objdir)" && \
	$(DLLTOOL) -d ./libmicrohttpd.def -D $$dll_name -l libmicrohttpd.lib $(libmicrohttpd_la_OBJECTS:.lo=.o) -e ./libmicrohttpd.exp && cd .. &&\
	echo Created libmicrohttpd.exp and libmicrohttpd.lib.
endif
else
  W32_MHD_LIB_LDFLAGS =
endif

if W32_STATIC_LIB
noinst_DATA += $(lt_cv_objdir)/libmicrohttpd-static.lib
MOSTLYCLEANFILES += $(lt_cv_objdir)/libmicrohttpd-static.lib

$(lt_cv_objdir)/libmicrohttpd-static.lib: libmicrohttpd.la $(libmicrohttpd_la_OBJECTS)
if USE_MS_LIB_TOOL
	$(MS_LIB_TOOL) -out:$@ $(libmicrohttpd_la_OBJECTS:.lo=.o)
else
	cp $(lt_cv_objdir)/libmicrohttpd.a $@
endif
endif


libmicrohttpd_la_SOURCES = \
  connection.c connection.h \
  reason_phrase.c \
  daemon.c  \
  internal.c internal.h \
  memorypool.c memorypool.h \
  mhd_mono_clock.c mhd_mono_clock.h \
  mhd_limits.h mhd_byteorder.h \
  sysfdsetsize.c sysfdsetsize.h \
  mhd_str.c mhd_str.h \
  mhd_assert.h \
  mhd_sockets.c mhd_sockets.h \
  mhd_itc.c mhd_itc.h mhd_itc_types.h \
  mhd_compat.c mhd_compat.h \
  response.c response.h

if USE_POSIX_THREADS
libmicrohttpd_la_SOURCES += \
  mhd_threads.c mhd_threads.h \
  mhd_locks.h
endif
if USE_W32_THREADS
libmicrohttpd_la_SOURCES += \
  mhd_threads.c mhd_threads.h \
  mhd_locks.h
endif

libmicrohttpd_la_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_LIB_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS) \
  -DBUILDING_MHD_LIB=1
libmicrohttpd_la_CFLAGS = \
  $(AM_CFLAGS) $(MHD_LIB_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
libmicrohttpd_la_LDFLAGS = \
  $(MHD_LIB_LDFLAGS) $(MHD_TLS_LIB_LDFLAGS) \
  $(W32_MHD_LIB_LDFLAGS) \
  -version-info @LIB_VERSION_CURRENT@:@LIB_VERSION_REVISION@:@LIB_VERSION_AGE@
libmicrohttpd_la_LIBADD = \
  $(MHD_LIBDEPS) $(MHD_TLS_LIBDEPS)

if HAVE_W32
MHD_DLL_RES_SRC = microhttpd_dll_res.rc
MHD_DLL_RES_LO = libmicrohttpd_la-$(MHD_DLL_RES_SRC:.rc=.lo)

EXTRA_libmicrohttpd_la_DEPENDENCIES = $(MHD_DLL_RES_LO)
libmicrohttpd_la_LIBADD += $(MHD_DLL_RES_LO)

# General rule is not required, but keep it just in case
.rc.lo:
	$(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) $(RCFLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $< -o $@

# To add dll resource only to .dll file and exclude it form static
# lib, a little trick was used. Allow libtool to create file.lo,
# file.o and .libs/file.lo, .libs/file.o files, then overwrite file.o
# by empty object generated from empty c-file. Later libtool will
# use .libs/file.o for shared lib and empty file.o for static lib.
# This implementation is based on trick found in liblzma.
# Note: windres does not understand '-isystem' flag, so all
# possible '-isystem' flags are replaced by simple '-I' flags.
$(MHD_DLL_RES_LO): $(MHD_DLL_RES_SRC)
	RC_CPP_FLAGS=" $(DEFAULT_INCLUDES) $(INCLUDES) $(libmicrohttpd_la_CPPFLAGS) $(CPPFLAGS) " && \
	$(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) $(RCFLAGS) $(DEFS) $${RC_CPP_FLAGS// -isystem / -I } $< -o $@ && \
	echo > $@-empty.c && $(CC) $(AM_CFLAGS) $(CFLAGS) -c $@-empty.c -o $(@:.lo=.o) && rm -f $@-empty.c
endif

if USE_COVERAGE
  AM_CFLAGS += --coverage
endif

if !MHD_HAVE_TSEARCH
libmicrohttpd_la_SOURCES += \
  tsearch.c tsearch.h
endif

if HAVE_POSTPROCESSOR
libmicrohttpd_la_SOURCES += \
  postprocessor.c
endif

if ENABLE_DAUTH
libmicrohttpd_la_SOURCES += \
  digestauth.c \
  mhd_bithelpers.h \
  md5.c md5.h \
  sha256.c sha256.h
endif

if ENABLE_BAUTH
libmicrohttpd_la_SOURCES += \
  basicauth.c \
  base64.c base64.h
endif

if ENABLE_HTTPS
libmicrohttpd_la_SOURCES += \
  connection_https.c connection_https.h
endif



check_PROGRAMS = \
  test_str_compare \
  test_str_to_value \
  test_str_token \
  test_http_reasons \
  test_md5 \
  test_sha256 \
  test_start_stop \
  test_daemon \
  test_options

if HAVE_POSIX_THREADS
if ENABLE_UPGRADE
if USE_POSIX_THREADS
  check_PROGRAMS += test_upgrade
endif
if USE_W32_THREADS
  check_PROGRAMS += test_upgrade
endif
if ENABLE_HTTPS
if USE_POSIX_THREADS
check_PROGRAMS += test_upgrade_tls
endif
if USE_W32_THREADS
check_PROGRAMS += test_upgrade_tls
endif
endif
endif
endif

if HAVE_POSTPROCESSOR
check_PROGRAMS += \
  test_postprocessor \
  test_postprocessor_large \
  test_postprocessor_amp
endif

TESTS = $(check_PROGRAMS)

# Do not test trigger of select by shutdown of listen socket
# on Cygwin as this ability is deliberately ignored on Cygwin
# to improve compatibility with core OS.
if !CYGWIN_TARGET
if HAVE_POSIX_THREADS
if HAVE_LISTEN_SHUTDOWN
check_PROGRAMS += \
  test_shutdown_select \
  test_shutdown_poll
else
check_PROGRAMS += \
  test_shutdown_select_ignore \
  test_shutdown_poll_ignore
endif
endif
endif

test_start_stop_SOURCES = \
  test_start_stop.c
test_start_stop_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la

test_daemon_SOURCES = \
  test_daemon.c
test_daemon_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la

test_upgrade_SOURCES = \
  test_upgrade.c test_helpers.h mhd_sockets.h
test_upgrade_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS)
test_upgrade_CFLAGS = \
  $(AM_CFLAGS) $(PTHREAD_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
test_upgrade_LDFLAGS = \
  $(MHD_TLS_LIB_LDFLAGS)
test_upgrade_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) \
  $(PTHREAD_LIBS)

test_upgrade_tls_SOURCES = \
  test_upgrade.c test_helpers.h mhd_sockets.h
test_upgrade_tls_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS)
test_upgrade_tls_CFLAGS = \
  $(AM_CFLAGS) $(PTHREAD_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
test_upgrade_tls_LDFLAGS = \
  $(MHD_TLS_LIB_LDFLAGS)
test_upgrade_tls_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la \
  $(MHD_TLS_LIB_LDFLAGS) $(MHD_TLS_LIBDEPS) \
  $(PTHREAD_LIBS)

test_postprocessor_SOURCES = \
  test_postprocessor.c
test_postprocessor_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS)
test_postprocessor_CFLAGS = \
  $(AM_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
test_postprocessor_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la

test_postprocessor_amp_SOURCES = \
  test_postprocessor_amp.c
test_postprocessor_amp_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS)
test_postprocessor_amp_CFLAGS = \
  $(AM_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
test_postprocessor_amp_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la

test_postprocessor_large_SOURCES = \
  test_postprocessor_large.c
test_postprocessor_large_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS)
test_postprocessor_large_CFLAGS = \
  $(AM_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
test_postprocessor_large_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la

test_shutdown_select_SOURCES = \
  test_shutdown_select.c
if USE_POSIX_THREADS
test_shutdown_select_CFLAGS = \
  $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_shutdown_select_LDADD = \
  $(PTHREAD_LIBS)
endif

test_shutdown_poll_SOURCES = \
  test_shutdown_select.c mhd_threads.h
if USE_POSIX_THREADS
test_shutdown_poll_CFLAGS = \
  $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_shutdown_poll_LDADD = \
  $(PTHREAD_LIBS)
endif

test_shutdown_select_ignore_SOURCES = \
  test_shutdown_select.c
if USE_POSIX_THREADS
test_shutdown_select_ignore_CFLAGS = \
  $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_shutdown_select_ignore_LDADD = \
  $(PTHREAD_LIBS)
endif

test_shutdown_poll_ignore_SOURCES = \
  test_shutdown_select.c mhd_threads.h
if USE_POSIX_THREADS
test_shutdown_poll_ignore_CFLAGS = \
  $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_shutdown_poll_ignore_LDADD = \
  $(PTHREAD_LIBS)
endif

test_str_compare_SOURCES = \
  test_str.c test_helpers.h mhd_str.c

test_str_to_value_SOURCES = \
  test_str.c test_helpers.h mhd_str.c

test_str_token_SOURCES = \
  test_str_token.c mhd_str.c

test_http_reasons_SOURCES = \
  test_http_reasons.c \
  reason_phrase.c mhd_str.c mhd_str.h

test_md5_SOURCES = \
  test_md5.c test_helpers.h \
  md5.c md5.h mhd_bithelpers.h

test_sha256_SOURCES = \
  test_sha256.c test_helpers.h \
  sha256.c sha256.h mhd_bithelpers.h

test_options_SOURCES = \
  test_options.c
test_options_LDADD = \
  $(top_builddir)/src/microhttpd/libmicrohttpd.la
