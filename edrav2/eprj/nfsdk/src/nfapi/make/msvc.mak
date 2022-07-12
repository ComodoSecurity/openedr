OUTNAME	= nfapi

!IF "$(DEBUG)" == "1"  
CONFIGDIR   = debug
!ELSE
CONFIGDIR   = release
!ENDIF

!IF "$(C_API)" == "1"
CONFIGDIR = $(CONFIGDIR)_c_api
!ENDIF

!IF "$(STATIC)" == "1"
CONFIGDIR = $(CONFIGDIR)_static
!ENDIF

!IF "$(LOGS)" == "1"
CONFIGDIR = $(CONFIGDIR)_logs
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

INCLUDES = -I../include

CC = cl
CFLAGS = /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "NFAPI_EXPORTS" /D "_WINDLL" /D "_MBCS" /EHsc /W3 /nologo /c /TP /errorReport:prompt /MP
LINK = link
LIBSDIR = 
LINKFLAGS = /DLL /SUBSYSTEM:WINDOWS /ERRORREPORT:PROMPT
PCH = stdafx.pch

!IF "$(DEBUG)" == "1"  
CFLAGS   = $(CFLAGS) /D_DEBUG /Od /Zi /RTC1 /MDd  
!ELSE  
CFLAGS   = $(CFLAGS) /DNDEBUG /Ox /MT /Zi
!ENDIF  

!IF "$(C_API)" == "1"
CFLAGS = $(CFLAGS) /D "_C_API"
!ENDIF

!IF "$(STATIC)" == "1"
CFLAGS = $(CFLAGS) /D "_NFAPI_STATIC_LIB"
!ENDIF

!IF "$(LOGS)" == "1"
CFLAGS = $(CFLAGS) /D "_RELEASE_LOG"
!ENDIF

OBJS = \
$(TMPDIR)/hashtable.obj \
$(TMPDIR)/mempools.obj \
$(TMPDIR)/nfapi.obj \
$(TMPDIR)/nfscm.obj \
$(TMPDIR)/nfapi.res

all: $(OUTDIR) $(OUTLIBDIR) $(TMPDIR) $(OUTNAME)

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
  lib.exe $(LIBSDIR) /OUT:"$(OUTLIBDIR)\$(OUTNAME).lib" $(TMPDIR)/*.obj
!ELSE
$(OUTNAME): $(OBJS) $(PCH)
	$(LINK) $(LINKFLAGS) $(linkdebug) $(LIBSDIR) /OUT:"$(OUTDIR)\$(OUTNAME).dll" /IMPLIB:"$(OUTLIBDIR)\$(OUTNAME).lib" /PDB:"$(TMPDIR)\$(OUTNAME).pdb" $(TMPDIR)/*.obj $(TMPDIR)/*.res kernel32.lib user32.lib gdi32.lib advapi32.lib
!ENDIF

clean: 
!IF "$(STATIC)" == "1"
	-del /Q $(OUTLIBDIR)\$(OUTNAME).lib
!ELSE
	-del /Q $(OUTDIR)\$(OUTNAME).dll
!ENDIF
	-del /Q $(TMPDIR)\*.*
