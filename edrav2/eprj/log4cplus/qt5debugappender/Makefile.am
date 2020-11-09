if QT5
lib_LTLIBRARIES += liblog4cplusqt5debugappender.la

liblog4cplusqt5debugappender_la_cppflags = \
	$(AM_CPPFLAGS) \
	-DINSIDE_LOG4CPLUS_QT5DEBUGAPPENDER \
	@QT5_CFLAGS@
liblog4cplusqt5debugappender_la_CPPFLAGS = \
	$(liblog4cplusqt5debugappender_la_cppflags)

liblog4cplusqt5debugappender_la_sources = \
	$(INCLUDES_SRC_PATH)/qt5debugappender.h \
	qt5debugappender/qt5debugappender.cxx
liblog4cplusqt5debugappender_la_SOURCES = \
	$(liblog4cplusqt5debugappender_la_sources)

liblog4cplusqt5debugappender_la_LIBADD = $(liblog4cplus_la_file)

liblog4cplusqt5debugappender_la_ldflags = -no-undefined \
	$(LOG4CPLUS_VERSION_LT_FLAGS) \
	@QT5_LIBS@ @LOG4CPLUS_PROFILING_LDFLAGS@
liblog4cplusqt5debugappender_la_LDFLAGS = \
	$(liblog4cplusqt5debugappender_la_ldflags)

if BUILD_WITH_WCHAR_T_SUPPORT
lib_LTLIBRARIES += liblog4cplusqt5debugappenderU.la
liblog4cplusqt5debugappenderU_la_CPPFLAGS = \
	$(liblog4cplusqt5debugappender_la_cppflags) -DUNICODE=1 -D_UNICODE=1
liblog4cplusqt5debugappenderU_la_SOURCES = \
	$(liblog4cplusqt5debugappender_la_sources)
liblog4cplusqt5debugappenderU_la_LIBADD = $(liblog4cplusU_la_file)
liblog4cplusqt5debugappenderU_la_LDFLAGS = \
	$(liblog4cplusqt5debugappender_la_ldflags)
endif

endif
