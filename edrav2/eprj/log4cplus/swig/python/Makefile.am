if WITH_PYTHON
PYTHON_WRAP_CXX = python_wrap.cxx
BUILT_SOURCES = $(PYTHON_WRAP_CXX)

pkgpython_PYTHON = log4cplus.py
pkgpyexec_LTLIBRARIES = _log4cplus.la
_log4cplus_la_SOURCES = $(PYTHON_WRAP_CXX) $(SWIG_SOURCES)
_log4cplus_la_CPPFLAGS = $(AM_CPPFLAGS) $(SWIG_PYTHON_CPPFLAGS) \
  $(PYTHON_CPPFLAGS) -DSWIG_TYPE_TABLE=log4cplus \
  "-Dregister=/*register*/"
_log4cplus_la_LDFLAGS = -no-undefined -shared -module -avoid-version \
  $(PYTHON_LDFLAGS) $(AM_LDFLAGS)
_log4cplus_la_LIBADD = $(liblog4cplus_la_file)

$(PYTHON_WRAP_CXX): $(SWIG_SOURCES)
	$(SWIG) $(SWIG_FLAGS) -c++ -python $(SWIG_PYTHON_OPT) \
	-I$(top_srcdir)/include -I$(top_builddir)/include \
	-I$(top_srcdir)/swig -o $(PYTHON_WRAP_CXX) \
	$(top_srcdir)/swig/log4cplus.swg

if BUILD_WITH_WCHAR_T_SUPPORT
PYTHON_WRAPU_CXX = python_wrapU.cxx
BUILT_SOURCES += $(PYTHON_WRAPU_CXX)

pkgpython_PYTHON += log4cplusU.py
pkgpyexec_LTLIBRARIES += _log4cplusU.la
_log4cplusU_la_SOURCES = $(PYTHON_WRAPU_CXX) $(SWIG_SOURCES)
_log4cplusU_la_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE=1 \
  $(SWIG_PYTHON_CPPFLAGS) $(PYTHON_CPPFLAGS) -DSWIG_TYPE_TABLE=log4cplus \
  "-Dregister=/*register*/"
_log4cplusU_la_LDFLAGS = -no-undefined -shared -module -avoid-version \
  $(PYTHON_LDFLAGS) $(AM_LDFLAGS)
_log4cplusU_la_LIBADD = $(liblog4cplusU_la_file)

$(PYTHON_WRAPU_CXX): $(SWIG_SOURCES)
	$(SWIG) -DUNICODE=1 -D_UNICODE=1 $(SWIG_FLAGS) -c++ -python $(SWIG_PYTHON_OPT) \
	-I$(top_srcdir)/include -I$(top_builddir)/include \
	-I$(top_srcdir)/swig -o $(PYTHON_WRAPU_CXX) \
	$(top_srcdir)/swig/log4cplus.swg
endif

endif
