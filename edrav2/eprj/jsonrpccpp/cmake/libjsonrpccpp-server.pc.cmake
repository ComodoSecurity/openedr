Name: libjsonrpccpp-server
Description: A C++ server implementation of json-rpc.
Version: ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}
Libs: -L${FULL_PATH_LIBDIR} -ljsoncpp -ljsonrpccpp-common -ljsonrpccpp-server -lmicrohttpd -lhiredis
Cflags: -I${FULL_PATH_INCLUDEDIR}
