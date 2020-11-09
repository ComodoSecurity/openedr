# This Makefile.am is in the public domain
if HAVE_CURL
curltests = testcurl
if HAVE_ZZUF
if HAVE_SOCAT
zzuftests = testzzuf
endif
endif
endif

SUBDIRS = include microhttpd $(curltests) $(zzuftests) .

if BUILD_EXAMPLES
SUBDIRS += examples
endif

# Finally (last!) also build experimental lib...
if HAVE_EXPERIMENTAL
SUBDIRS += lib
endif

EXTRA_DIST = \
 datadir/cert-and-key.pem \
 datadir/cert-and-key-for-wireshark.pem 
