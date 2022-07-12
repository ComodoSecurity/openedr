OUTNAME	= libprotocolfilters.a
BASEDIR	=.
OUTDIR	=./build
TMPDIR	=./build_tmp
SRCDIR	=$(BASEDIR)

SRC_LIST	=$(shell find $(SRCDIR) -maxdepth 2 -name "*.cpp")
OBJ_LIST	=$(addprefix $(TMPDIR)/,$(notdir $(addsuffix .o, $(basename $(SRC_LIST)))))

MAKE = make

CC	= g++
INCLUDES	=$(BASEDIR) $(BASEDIR)/openssl/include $(BASEDIR)/openssl $(BASEDIR)/include $(BASEDIR)/../include 
CPPFLAGS	= -g -O3 -w $(addprefix -I,$(INCLUDES))

LIBS		= 
LIB_DIR		=

LINK		=ld
LINK_FLAGS	=-r
AR = ar
ARFLAGS = cru

ifeq ($(C_API),1)
CPPFLAGS += -D_C_API
endif

ifeq ($(NO_SSL),1)
CPPFLAGS += -DPF_NO_SSL_FILTER
endif

ifeq ($(LOGS),1)
CPPFLAGS += -D_RELEASE_LOG
endif


BUILD_CMD=  if [ -d "$$dir" ]; then \
	    (	cd $$dir && echo "making $$target in $$dir..." && \
		$$config && \
		$(MAKE) $$target \
	    ) || exit 1; \
	    fi
RECURSIVE_BUILD_CMD=for dir in $(DIRS); do $(BUILD_CMD); done

all: mkdirs build_zlib build_openssl $(OUTNAME)

build_zlib:
	dir=zlib; target=all; $(BUILD_CMD)

clean_zlib:
	dir=zlib; target=clean; $(BUILD_CMD)

build_openssl:
	chmod 755 openssl/config
	dir=openssl; config="./config enable-static-engine enable-weak-ssl-ciphers"; target=build_libs; $(BUILD_CMD)
	cp openssl/libssl.a $(OUTDIR)
	cp openssl/libcrypto.a $(OUTDIR)

clean_openssl:
	dir=openssl; target=clean; $(BUILD_CMD)

mkdirs:
	if !([ -d "$(OUTDIR)" ]); then \
		(  mkdir $(OUTDIR) ) || exit 1; \
	    fi
	if !([ -d "$(TMPDIR)" ]); then \
		(  mkdir $(TMPDIR) ) || exit 1; \
	    fi

clean: clean_zlib clean_openssl
	rm -f $(TMPDIR)/*.o
	rm -f $(OUTDIR)/*

$(OBJ_LIST): $(SRC_LIST)
	$(CC) $(CPPFLAGS) -o $@ -c $(filter %/$(notdir $(basename $@)).cpp,$(SRC_LIST)) 
	
$(OUTNAME): $(OBJ_LIST)
	$(AR) $(ARFLAGS) $(OUTDIR)/$@ $(OBJ_LIST) $(LIBS)

