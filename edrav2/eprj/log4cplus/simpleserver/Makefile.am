if MULTI_THREADED
noinst_PROGRAMS += loggingserver
loggingserver_sources = simpleserver/loggingserver.cxx
loggingserver_SOURCES = $(loggingserver_sources)
loggingserver_LDADD = $(liblog4cplus_la_file)

if BUILD_WITH_WCHAR_T_SUPPORT
noinst_PROGRAMS += loggingserverU
loggingserverU_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE=1 -D_UNICODE=1
loggingserverU_SOURCES = $(loggingserver_sources)
loggingserverU_LDADD = $(liblog4cplusU_la_file)
endif

endif
