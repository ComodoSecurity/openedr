# This Makefile.am is in the public domain
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = contrib m4 src .

if HAVE_PO
SUBDIRS += po
endif

if BUILD_DOC
SUBDIRS += doc
endif


W32COMMON = w32/common/gen_dll_res.ps1 w32/common/microhttpd_dll_res_vc.rc.in w32/common/microhttpd_dll_res_vc.rc \
  w32/common/MHD_config.h w32/common/vs_dirs.props w32/common/common-build-settings.vcxproj \
  w32/common/libmicrohttpd-build-settings.vcxproj w32/common/libmicrohttpd-files.vcxproj w32/common/libmicrohttpd-filters.vcxproj \
  w32/common/hellobrowser-files.vcxproj w32/common/hellobrowser-filters.vcxproj \
  w32/common/apps-build-settings.vcxproj
W32VS2013 = w32/VS2013/libmicrohttpd.vcxproj w32/VS2013/libmicrohttpd.vcxproj.filters \
  w32/VS2013/hellobrowser.vcxproj w32/VS2013/hellobrowser.vcxproj.filters \
  w32/VS2013/simplepost.vcxproj w32/VS2013/largepost.vcxproj \
  w32/VS2013/libmicrohttpd.sln
W32VS2015 = w32/VS2015/libmicrohttpd.vcxproj w32/VS2015/libmicrohttpd.vcxproj.filters \
  w32/VS2015/hellobrowser.vcxproj w32/VS2015/hellobrowser.vcxproj.filters \
  w32/VS2015/simplepost.vcxproj w32/VS2015/largepost.vcxproj \
  w32/VS2015/libmicrohttpd.sln
W32VS2017 = w32/VS2017/libmicrohttpd.vcxproj w32/VS2017/libmicrohttpd.vcxproj.filters \
  w32/VS2017/hellobrowser.vcxproj w32/VS2017/hellobrowser.vcxproj.filters \
  w32/VS2017/simplepost.vcxproj w32/VS2017/largepost.vcxproj \
  w32/VS2017/libmicrohttpd.sln
W32VS2019 = w32/VS2019/libmicrohttpd.vcxproj w32/VS2019/libmicrohttpd.vcxproj.filters \
  w32/VS2019/hellobrowser.vcxproj w32/VS2019/hellobrowser.vcxproj.filters \
  w32/VS2019/simplepost.vcxproj w32/VS2019/largepost.vcxproj \
  w32/VS2019/libmicrohttpd.sln
EXTRA_DIST = \
  acinclude.m4 \
  libmicrohttpd.pc.in \
  $(W32COMMON) $(W32VS2013) $(W32VS2015) $(W32VS2017) $(W32VS2019)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmicrohttpd.pc
