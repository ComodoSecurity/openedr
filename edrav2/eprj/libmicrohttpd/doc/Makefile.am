# This Makefile.am is in the public domain
man_MANS = libmicrohttpd.3

SUBDIRS = . doxygen

if BUILD_EXAMPLES
SUBDIRS += examples
endif

DISTCLEANFILES = \
  libmicrohttpd.cps \
  libmicrohttpd.dvi \
  libmicrohttpd-tutorial.cps \
  libmicrohttpd-tutorial.dvi

info_TEXINFOS = \
  libmicrohttpd.texi \
  libmicrohttpd-tutorial.texi
microhttpd_TEXINFOS = \
  chapters/basicauthentication.inc \
  chapters/bibliography.inc \
  chapters/exploringrequests.inc \
  chapters/hellobrowser.inc \
  chapters/introduction.inc \
  chapters/largerpost.inc \
  chapters/processingpost.inc \
  chapters/responseheaders.inc \
  chapters/tlsauthentication.inc \
  chapters/sessions.inc \
  fdl-1.3.texi \
  lgpl.texi \
  ecos.texi

EXTRA_DIST = \
  $(man_MANS) \
  $(microhttpd_TEXINFOS) \
  libmicrohttpd_performance_data.png \
  libmicrohttpd_performance_data.eps

install-info-local:
	@echo " $(MKDIR_P) '$(DESTDIR)$(infodir)'"; \
	$(MKDIR_P) "$(DESTDIR)$(infodir)" || exit 1; \
	echo " $(INSTALL_DATA) libmicrohttpd_performance_data.png '$(DESTDIR)$(infodir)'"; \
	$(INSTALL_DATA) $(srcdir)/libmicrohttpd_performance_data.png "$(DESTDIR)$(infodir)" || exit 1;

install-html-local:
	@if test -n "$(htmldir)"; then \
	  echo " $(MKDIR_P) '$(DESTDIR)$(htmldir)/libmicrohttpd.html'"; \
	  $(MKDIR_P) "$(DESTDIR)$(htmldir)/libmicrohttpd.html" || exit 1; \
	  echo " $(INSTALL_DATA) libmicrohttpd_performance_data.png '$(DESTDIR)$(htmldir)/libmicrohttpd.html'"; \
	  $(INSTALL_DATA) $(srcdir)/libmicrohttpd_performance_data.png "$(DESTDIR)$(htmldir)/libmicrohttpd.html" || exit 1; \
	else : ; fi

uninstall-local:
	@if test -d "$(DESTDIR)$(infodir)"; then \
	  echo " rm -f '$(DESTDIR)$(infodir)/libmicrohttpd_performance_data.png'"; \
	  rm -f "$(DESTDIR)$(infodir)/libmicrohttpd_performance_data.png" \
	else : ; fi

# end of 'if BUILD_DOC'
