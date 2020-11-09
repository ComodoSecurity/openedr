lib_LTLIBRARIES = liblog4cplus.la

SINGLE_THREADED_SRC = \
	%D%/appenderattachableimpl.cxx \
	%D%/appender.cxx \
	%D%/asyncappender.cxx \
        %D%/callbackappender.cxx \
	%D%/clogger.cxx \
	%D%/configurator.cxx \
	%D%/connectorthread.cxx \
	%D%/consoleappender.cxx \
	%D%/cygwin-win32.cxx \
	%D%/env.cxx \
	%D%/factory.cxx \
	%D%/fileappender.cxx \
	%D%/fileinfo.cxx \
	%D%/filter.cxx \
	%D%/global-init.cxx \
	%D%/hierarchy.cxx \
	%D%/hierarchylocker.cxx \
	%D%/layout.cxx \
	%D%/log4judpappender.cxx \
	%D%/lockfile.cxx \
	%D%/logger.cxx \
	%D%/loggerimpl.cxx \
	%D%/loggingevent.cxx \
	%D%/loggingmacros.cxx \
	%D%/loglevel.cxx \
	%D%/loglog.cxx \
	%D%/mdc.cxx \
	%D%/ndc.cxx \
	%D%/nullappender.cxx \
	%D%/nteventlogappender.cxx \
	%D%/objectregistry.cxx \
	%D%/patternlayout.cxx \
	%D%/pointer.cxx \
	%D%/property.cxx \
	%D%/queue.cxx \
	%D%/rootlogger.cxx \
	%D%/snprintf.cxx \
	%D%/socketappender.cxx \
	%D%/socketbuffer.cxx \
	%D%/socket.cxx \
	%D%/socket-unix.cxx \
	%D%/socket-win32.cxx \
	%D%/stringhelper.cxx \
	%D%/stringhelper-clocale.cxx \
	%D%/stringhelper-cxxlocale.cxx \
	%D%/stringhelper-iconv.cxx \
	%D%/syncprims.cxx \
	%D%/syslogappender.cxx \
	%D%/threads.cxx \
	%D%/timehelper.cxx \
	%D%/tls.cxx \
	%D%/version.cxx \
	%D%/win32consoleappender.cxx \
	%D%/win32debugappender.cxx

LIB_SRC = $(SINGLE_THREADED_SRC)

common_liblog4cplus_la_cppflags = $(AM_CPPFLAGS) -DINSIDE_LOG4CPLUS
liblog4cplus_la_CPPFLAGS = $(common_liblog4cplus_la_cppflags)

common_liblog4cplus_la_sources = $(LIB_SRC)
liblog4cplus_la_SOURCES = $(common_liblog4cplus_la_sources)

if USE_LOG4CPLUS_EXPORT_SYMBOLS_REGEX
LOG4CPLUS_EXPORT_SYMBOLS_REGEX = -export-symbols-regex 'log4cplus'
else
LOG4CPLUS_EXPORT_SYMBOLS_REGEX =
endif

common_liblog4cplus_la_ldflags = -no-undefined \
	$(LOG4CPLUS_VERSION_LT_FLAGS) \
	$(LOG4CPLUS_PROFILING_LDFLAGS) \
        $(LOG4CPLUS_LTO_LDFLAGS) \
	$(LOG4CPLUS_AIX_XLC_LDFLAGS) \
	$(LOG4CPLUS_EXPORT_SYMBOLS_REGEX)
liblog4cplus_la_LDFLAGS = $(common_liblog4cplus_la_ldflags)

if BUILD_WITH_WCHAR_T_SUPPORT
lib_LTLIBRARIES += liblog4cplusU.la

liblog4cplusU_la_CPPFLAGS = $(common_liblog4cplus_la_cppflags) -DUNICODE=1 -D_UNICODE=1

liblog4cplusU_la_SOURCES = $(common_liblog4cplus_la_sources)

liblog4cplusU_la_LDFLAGS = $(common_liblog4cplus_la_ldflags)
endif
