#!/bin/bash
set -evu

PREFIX=/usr/local

if [ "$OS" == "arch" ] || [ "$OS" == "fedora" ]
then
	PREFIX=/usr
fi

CLIENT_LIBS="-ljsoncpp -lcurl -ljsonrpccpp-common -ljsonrpccpp-client -lhiredis"
SERVER_LIBS="-ljsoncpp -lmicrohttpd -ljsonrpccpp-common -ljsonrpccpp-server -lhiredis"
mkdir -p build && cd build

echo "PREFIX: $PREFIX"
cmake -DCMAKE_INSTALL_PREFIX="$PREFIX" -DWITH_COVERAGE=YES -DCMAKE_BUILD_TYPE=Release \
	-DBUILD_STATIC_LIBS=ON -DTCP_SOCKET_SERVER=YES -DTCP_SOCKET_CLIENT=YES \
	-DBUILD_SHARED_LIBS=ON -DUNIX_DOMAIN_SOCKET_SERVER=NO -DUNIX_DOMAIN_SOCKET_CLIENT=NO ..

make -j$(nproc)

echo "Running test suite"
./bin/unit_testsuite --durations yes

if [ "$OS" != "osx" ]
then
 	make install
	ldconfig
else
	sudo make install
fi

cd ../src/examples
g++ -std=c++11 simpleclient.cpp $CLIENT_LIBS -o simpleclient
g++ -std=c++11 simpleserver.cpp $SERVER_LIBS -o simpleserver

mkdir -p gen && cd gen
jsonrpcstub ../spec.json --cpp-server=AbstractStubServer --cpp-client=StubClient
cd ..
g++ -std=c++11 stubclient.cpp $CLIENT_LIBS -o stubclient
g++ -std=c++11 stubserver.cpp $SERVER_LIBS -o stubserver

test -f simpleclient
test -f simpleserver
test -f stubserver
test -f stubclient

cd ../../build

if [ "$OS" != "osx" ]
then
	make uninstall
else
	sudo make uninstall
fi
