# This Makefile.am is in the public domain
AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/lib

AM_CFLAGS = $(HIDDEN_VISIBILITY_CFLAGS)

# Call "libmicrohttpd2" for now, but only while under
# development. Once we have 'compat' working, this should be changed!
noinst_LTLIBRARIES = \
  libmicrohttpd2.la

noinst_DATA =
MOSTLYCLEANFILES =

if W32_SHARED_LIB_EXP
W32_MHD_LIB_LDFLAGS = -Wl,--output-def,$(lt_cv_objdir)/libmicrohttpd2.def -XCClinker -static-libgcc
noinst_DATA += $(lt_cv_objdir)/libmicrohttpd2.lib $(lt_cv_objdir)/libmicrohttpd2.def $(lt_cv_objdir)/libmicrohttpd2.exp
MOSTLYCLEANFILES += $(lt_cv_objdir)/libmicrohttpd2.lib $(lt_cv_objdir)/libmicrohttpd2.def $(lt_cv_objdir)/libmicrohttpd2.exp

$(lt_cv_objdir)/libmicrohttpd2.def: libmicrohttpd2.la

$(lt_cv_objdir)/libmicrohttpd2.exp: $(lt_cv_objdir)/libmicrohttpd2.lib

$(lt_cv_objdir)/libmicrohttpd2.lib: $(lt_cv_objdir)/libmicrohttpd2.def libmicrohttpd2.la $(libmicrohttpd2_la_OBJECTS)
if USE_MS_LIB_TOOL
	@echo Creating $@ and libmicrohttpd2.exp by $(MS_LIB_TOOL)... && \
	dll_name=`$(EGREP) -o dlname=\'.+\' libmicrohttpd2.la` && \
	dll_name=$${dll_name#*\'} && dll_name=$${dll_name%\'} && test -n "$$dll_name" && \
	echo Creating $$dll_name by $(MS_LIB_TOOL).. && cd "$(lt_cv_objdir)" && \
	$(MS_LIB_TOOL) -def:libmicrohttpd2.def -name:$$dll_name -out:libmicrohttpd2.lib $(libmicrohttpd2_la_OBJECTS:.lo=.o) && cd ..
else
	@echo Creating $@ and libmicrohttpd2.exp by $(DLLTOOL)... && \
	dll_name=`$(EGREP) -o dlname=\'.+\' libmicrohttpd2.la` && \
	dll_name=$${dll_name#*\'} && dll_name=$${dll_name%\'} && test -n "$$dll_name" && \
	echo Creating $$dll_name by $(DLLTOOL).. && cd "$(lt_cv_objdir)" && \
	$(DLLTOOL) -d ./libmicrohttpd2.def -D $$dll_name -l libmicrohttpd2.lib $(libmicrohttpd2_la_OBJECTS:.lo=.o) -e ./libmicrohttpd2.exp && cd .. &&\
	echo Created libmicrohttpd2.exp and libmicrohttpd2.lib.
endif
else
  W32_MHD_LIB_LDFLAGS =
endif

if W32_STATIC_LIB
noinst_DATA += $(lt_cv_objdir)/libmicrohttpd2-static.lib
MOSTLYCLEANFILES += $(lt_cv_objdir)/libmicrohttpd2-static.lib

$(lt_cv_objdir)/libmicrohttpd2-static.lib: libmicrohttpd2.la $(libmicrohttpd2_la_OBJECTS)
if USE_MS_LIB_TOOL
	$(MS_LIB_TOOL) -out:$@ $(libmicrohttpd2_la_OBJECTS:.lo=.o)
else
	cp $(lt_cv_objdir)/libmicrohttpd2.a $@
endif
endif


libmicrohttpd2_la_SOURCES = \
  action_continue.c \
  action_from_response.c \
  action_parse_post.c \
  action_process_upload.c \
  action_suspend.c \
  connection_add.c connection_add.h \
  connection_call_handlers.c connection_call_handlers.h \
  connection_cleanup.c connection_cleanup.h \
  connection_close.c connection_close.h \
  connection_finish_forward.c connection_finish_forward.h \
  connection_info.c \
  connection_options.c \
  connection_update_last_activity.c connection_update_last_activity.h \
  daemon_close_all_connections.c daemon_close_all_connections.h \
  daemon_create.c \
  daemon_destroy.c \
  daemon_epoll.c daemon_epoll.h \
  daemon_get_timeout.c \
  daemon_info.c \
  daemon_ip_limit.c daemon_ip_limit.h \
  daemon_options.c \
  daemon_poll.c daemon_poll.h \
  daemon_run.c \
  daemon_select.c daemon_select.h \
  daemon_start.c \
  daemon_quiesce.c \
  init.c init.h \
  internal.c internal.h \
  memorypool.c memorypool.h \
  mhd_assert.h \
  mhd_byteorder.h \
  mhd_compat.c mhd_compat.h \
  mhd_itc.c mhd_itc.h mhd_itc_types.h \
  mhd_limits.h \
  mhd_locks.h \
  mhd_mono_clock.c mhd_mono_clock.h \
  mhd_str.c mhd_str.h \
  mhd_sockets.c mhd_sockets.h \
  mhd_threads.c mhd_threads.h \
  response.c \
  response_for_upgrade.c \
  response_from_buffer.c \
  response_from_callback.c \
  response_from_fd.c \
  response_options.c \
  reason_phrase.c \
  request.c \
  request_info.c \
  request_resume.c \
  request_resume.h \
  sysfdsetsize.c sysfdsetsize.h \
  upgrade_process.c upgrade_process.h \
  panic.c \
  version.c

libmicrohttpd2_la_CPPFLAGS = \
  $(AM_CPPFLAGS) $(MHD_LIB_CPPFLAGS) $(MHD_TLS_LIB_CPPFLAGS) \
  -DBUILDING_MHD_LIB=1
libmicrohttpd2_la_CFLAGS = \
  $(AM_CFLAGS) $(MHD_LIB_CFLAGS) $(MHD_TLS_LIB_CFLAGS)
libmicrohttpd2_la_LDFLAGS = \
  $(MHD_LIB_LDFLAGS) \
  $(W32_MHD_LIB_LDFLAGS) $(MHD_TLS_LIB_LDFLAGS) \
  -version-info 0:0:0 # FIXME: fix once closer to release...
if MHD_HAVE_TLS_PLUGIN
libmicrohttpd2_la_LDFLAGS += \
  -ldl
endif

libmicrohttpd2_la_LIBADD = \
  $(MHD_LIBDEPS) $(MHD_TLS_LIBDEPS)

if HAVE_W32
MHD_DLL_RES_SRC = ../microhttpd/microhttpd_dll_res.rc
MHD_DLL_RES_LO = libmicrohttpd2_la-$(MHD_DLL_RES_SRC:.rc=.lo)

EXTRA_libmicrohttpd2_la_DEPENDENCIES = $(MHD_DLL_RES_LO)
libmicrohttpd2_la_LIBADD += $(MHD_DLL_RES_LO)

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
	RC_CPP_FLAGS=" $(DEFAULT_INCLUDES) $(INCLUDES) $(libmicrohttpd2_la_CPPFLAGS) $(CPPFLAGS) " && \
	$(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(RC) $(RCFLAGS) $(DEFS) $${RC_CPP_FLAGS// -isystem / -I } $< -o $@ && \
	echo > $@-empty.c && $(CC) $(AM_CFLAGS) $(CFLAGS) -c $@-empty.c -o $(@:.lo=.o) && rm -f $@-empty.c
endif

if USE_COVERAGE
  AM_CFLAGS += --coverage
endif

if !MHD_HAVE_TSEARCH
libmicrohttpd2_la_SOURCES += \
  tsearch.c tsearch.h
endif

# TBD!
if HAVE_POSTPROCESSOR
#libmicrohttpd2_la_SOURCES += \
#  postprocessor.c
endif

# TBD!
if ENABLE_DAUTH
#libmicrohttpd2_la_SOURCES += \
#  digestauth.c \
#  md5.c md5.h
endif

# TBD!
if ENABLE_BAUTH
#libmicrohttpd2_la_SOURCES += \
#  basicauth.c \
#  base64.c base64.h
endif
