DIRS = zlib\~ openssl\~

OUTNAME	= protocolfilters

!IF "$(DEBUG)" == "1"  
CONFIGDIR   = debug
!ELSE
CONFIGDIR   = release
!ENDIF

!IF "$(C_API)" == "1"
CONFIGDIR = $(CONFIGDIR)_c_api
!ENDIF

!IF "$(NO_SSL)" == "1"
CONFIGDIR = $(CONFIGDIR)_no_ssl
!ENDIF

!IF "$(STATIC)" == "1"
CONFIGDIR = $(CONFIGDIR)_static
!ENDIF

!IF "$(STATIC_SSL)" == "1"
CONFIGDIR = $(CONFIGDIR)_static_ssl
!ENDIF

!IF "$(LOGS)" == "1"
CONFIGDIR = $(CONFIGDIR)_logs
!ENDIF

!IF "$(DEMO)" == "1"
CONFIGDIR = $(CONFIGDIR)_demo
!ENDIF

!IF "$(PLATFORM)" == "x64"  
CONFIGDIR = $(CONFIGDIR)\x64
!ELSE
CONFIGDIR = $(CONFIGDIR)\win32
!ENDIF

BASEDIR	=.
BINDIR	=..\bin
LIBDIR	=..\lib
BUILDDIR = build
OUTDIR	=$(BINDIR)\$(CONFIGDIR)
OUTLIBDIR	=$(LIBDIR)\$(CONFIGDIR)
TMPDIR	=$(BUILDDIR)\$(CONFIGDIR)
SRCDIR	=$(BASEDIR)

INCLUDES = -I../include -Iinclude -Iopenssl/include -Iopenssl

CC = cl
CFLAGS = /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "PROTOCOLFILTERS_EXPORTS" /D "_WINDLL" /D "_MBCS" /EHsc /W3 /nologo /c /TP /errorReport:prompt /MP
LINK = link
LIBSDIR = 
LINKFLAGS = /DLL /SUBSYSTEM:WINDOWS /ERRORREPORT:PROMPT
PCH = stdafx.pch


!IF "$(PLATFORM)" == "x64"  
!IF "$(STATIC_SSL)" == "1"
OPENSSL_DIR = /LIBPATH:openssl\out64
!ELSE
OPENSSL_DIR = /LIBPATH:openssl\out64dll
!ENDIF
!ELSE
!IF "$(STATIC_SSL)" == "1"
OPENSSL_DIR = /LIBPATH:openssl\out32
!ELSE
OPENSSL_DIR = /LIBPATH:openssl\out32dll
!ENDIF
!ENDIF

!IF "$(DEBUG)" == "1"  
OPENSSL_DIR = $(OPENSSL_DIR).dbg
!ENDIF

!IF "$(DEBUG)" == "1"  
ZLIB_DIR = /LIBPATH:zlib/build/debug
!ELSE
ZLIB_DIR = /LIBPATH:zlib/build/release
!ENDIF

!IF "$(PLATFORM)" == "x64"  
ZLIB_DIR = $(ZLIB_DIR)/x64
!ELSE
ZLIB_DIR = $(ZLIB_DIR)/win32
!ENDIF

LIBSDIR = $(LIBSDIR) $(OPENSSL_DIR) $(ZLIB_DIR)

!IF "$(DEBUG)" == "1"  
CFLAGS   = $(CFLAGS) /D_DEBUG /Od /Zi /RTC1 /MDd  
!ELSE  
CFLAGS   = $(CFLAGS) /DNDEBUG /Ox /MT /Zi
!ENDIF  

!IF "$(C_API)" == "1"
CFLAGS = $(CFLAGS) /D "_C_API"
!ENDIF

!IF "$(NO_SSL)" == "1"
CFLAGS = $(CFLAGS) /D "PF_NO_SSL_FILTER"
!ENDIF

!IF "$(STATIC)" == "1"
CFLAGS = $(CFLAGS) /D "_PF_STATIC_LIB" /D "_NFAPI_STATIC_LIB"
!ENDIF

!IF "$(DEMO)" == "1"
CFLAGS = $(CFLAGS) /D "_DEMO"
!ENDIF

!IF "$(LOGS)" == "1"
CFLAGS = $(CFLAGS) /D "_RELEASE_LOG"
!ENDIF

OBJS = \
$(TMPDIR)/auxutil.obj \
$(TMPDIR)/certimp.obj \
$(TMPDIR)/certimp_nix.obj \
$(TMPDIR)/FileStream.obj \
$(TMPDIR)/FTPDataFilter.obj \
$(TMPDIR)/FTPFilter.obj \
$(TMPDIR)/HTTPFilter.obj \
$(TMPDIR)/HTTPParser.obj \
$(TMPDIR)/ICQFilter.obj \
$(TMPDIR)/IOB.obj \
$(TMPDIR)/MemoryStream.obj \
$(TMPDIR)/MFStream.obj \
$(TMPDIR)/NNTPFilter.obj \
$(TMPDIR)/PFHeader_c.obj \
$(TMPDIR)/PFObjectImpl.obj \
$(TMPDIR)/PFObjectImpl_c.obj \
$(TMPDIR)/PFStream_c.obj \
$(TMPDIR)/POP3Filter.obj \
$(TMPDIR)/ProtocolFilters.obj \
$(TMPDIR)/ProtocolFilters_c.obj \
$(TMPDIR)/Proxy.obj \
$(TMPDIR)/ProxyFilter.obj \
$(TMPDIR)/ProxySession.obj \
$(TMPDIR)/RawFilter.obj \
$(TMPDIR)/Settings.obj \
$(TMPDIR)/SMTPFilter.obj \
$(TMPDIR)/SSLDataProvider.obj \
$(TMPDIR)/SSLFilter.obj \
$(TMPDIR)/SSLSessionData.obj \
$(TMPDIR)/TSProcessInfoHelper.obj \
$(TMPDIR)/UnzipStream.obj \
$(TMPDIR)/XMPPFilter.obj \
$(TMPDIR)/ProtocolFilters.res

!IF "$(STATIC)" != "1" && "$(STATIC_SSL)" != "1" && "$(NO_SSL)" != "1" && "$(LOGS)" != "1"
OPENSSL_BIN_DIR = openssl

!IF "$(PLATFORM)" == "x64"  
OPENSSL_BIN_DIR = $(OPENSSL_BIN_DIR)\out64dll
!ELSE
OPENSSL_BIN_DIR = $(OPENSSL_BIN_DIR)\out32dll
!ENDIF

!IF "$(DEBUG)" == "1"  
OPENSSL_BIN_DIR = $(OPENSSL_BIN_DIR).dbg
!ENDIF

COPY_OPENSSL_CMD = "copy $(OPENSSL_BIN_DIR)\lib*.dll $(OUTDIR)"

!ELSE
COPY_OPENSSL_CMD = ""
!ENDIF


all: $(DIRS) $(OUTDIR) $(OUTLIBDIR) $(TMPDIR) $(OUTNAME)

bin: $(OUTDIR) $(OUTLIBDIR) $(TMPDIR) $(OUTNAME)

$(OUTDIR):
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

$(OUTLIBDIR):
    if not exist "$(OUTLIBDIR)/$(NULL)" mkdir "$(OUTLIBDIR)"

$(TMPDIR):
    if not exist "$(TMPDIR)/$(NULL)" mkdir "$(TMPDIR)"

.cpp{$(TMPDIR)}.obj:: 
  	$(CC) $(CFLAGS) $(INCLUDES) /Yu"stdafx.h" /Fo"$(TMPDIR)/" /Fp"$(TMPDIR)/$(PCH)" /Fd"$(TMPDIR)/$(OUTNAME).pdb" -c $<

.rc{$(TMPDIR)}.res:: 
  	$(RC) $(RFLAGS) /Fo"$(TMPDIR)/$(OUTNAME).res" $<

$(PCH) : stdafx.h 
	$(CC) $(CFLAGS) $(INCLUDES) /Yc"stdafx.h" /Fo"$(TMPDIR)/" /Fp"$(TMPDIR)/$(PCH)" /Fd"$(TMPDIR)/$(OUTNAME).pdb" stdafx.cpp

!IF "$(STATIC)" == "1"
$(OUTNAME): $(OBJS) $(PCH)
  lib.exe $(LIBSDIR) /OUT:"$(OUTLIBDIR)\$(OUTNAME).lib" $(TMPDIR)/*.obj zlib.lib
!ELSE
$(OUTNAME): $(OBJS) $(PCH)
	$(LINK) $(LINKFLAGS) $(linkdebug) $(LIBSDIR) /OUT:"$(OUTDIR)\$(OUTNAME).dll" /IMPLIB:"$(OUTLIBDIR)\$(OUTNAME).lib" /PDB:"$(TMPDIR)\$(OUTNAME).pdb" $(TMPDIR)/*.obj $(TMPDIR)/*.res zlib.lib kernel32.lib user32.lib gdi32.lib advapi32.lib
	$(COPY_OPENSSL_CMD)
!ENDIF

$(DIRS):
	@echo Building $(@D)...
	@IF EXIST $(@D)\msvc.mak <<nmaketmp.bat
	@cd $(@D)
	@$(MAKE) -nologo /$(MAKEFLAGS) $(MAKEOPTS) -f msvc.mak DEBUG=$(DEBUG) PLATFORM=$(PLATFORM) STATIC=$(STATIC)
	@cd ..
	@echo Building $(@D) completed.
<<

clean: 
!IF "$(STATIC)" == "1"
	-del /Q $(OUTLIBDIR)\$(OUTNAME).lib
!ELSE
	-del /Q $(OUTDIR)\$(OUTNAME).dll
!ENDIF
	-del /Q $(TMPDIR)\*.*
	cd zlib
	-$(MAKE) -nologo -f msvc.mak DEBUG=$(DEBUG) PLATFORM=$(PLATFORM) STATIC=$(STATIC) clean
	cd ..
	cd openssl
	-$(MAKE) -nologo -f msvc.mak DEBUG=$(DEBUG) PLATFORM=$(PLATFORM) STATIC=$(STATIC) clean
	cd ..

clean_bin: 
!IF "$(STATIC)" == "1"
	-del /Q $(OUTLIBDIR)\$(OUTNAME).lib
!ELSE
	-del /Q $(OUTDIR)\$(OUTNAME).dll
!ENDIF
	-del /Q $(TMPDIR)\*.*
