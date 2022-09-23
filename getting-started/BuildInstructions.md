# Build Instructions
You should have Microsoft Visual Studio 2019 to build the code
* Open Microsoft Visual Studio 2019 as Administrator under  /openedr/edrav2/build/vs2019/
* Open the edrav2.sln solutions file within the folder
* Build Release on edrav2.sln on Visual Studio
* Open Microsoft Visual Studio 2019 as Administrator under  /openedr/edrav2/iprj/installations/build/vs2019/
* Open installation.wixproj
* Compile installer.
```
As a default You should have these libraries in you Visual Studio 2019 but these are needed for build
## Libraries Used:
* AWS SDK AWS SDK for C++ : (https://aws.amazon.com/sdk-for-cpp/)
* Boost C++ Libraries (http://www.boost.org/)
* c-ares: asynchronous resolver library (https://github.com/c-ares/c-ares)
* Catch2: a unit testing framework for C++ (https://github.com/catchorg/Catch2)
* Clare : Command Line parcer for C++ (https://github.com/catchorg/Clara)
* Cli: cross-platform header-only C++14 library for interactive command line interfaces (https://cli.github.com/) 
* Crashpad: crash-reporting system (https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md)
* Curl: command-line tool for transferring data specified with URL syntax (https://curl.haxx.se/)
* Detours: for monitoring and instrumenting API calls on Windows. (https://github.com/microsoft/Detours)
* Google APIs: Google APIs (https://github.com/googleapis/googleapis)
* JsonCpp: C++ library that allows manipulating JSON values (https://github.com/open-source-parsers/jsoncpp)
* libjson-rpc-cpp: cross platform JSON-RPC (remote procedure call) support for C++  (https://github.com/cinemast/libjson-rpc-cpp)
* libmicrohttpd : C library that provides a compact API and implementation of an HTTP 1.1 web server (https://www.gnu.org/software/libmicrohttpd/)
* log4cplus: C++17 logging API (https://github.com/log4cplus/log4cplus)
* nlohmann JSON: JSON library for C++: (https://github.com/nlohmann/json)
* OpenSSL Toolkit (http://www.openssl.org/)
* Tiny AES in C: implementation of the AES ECB, CTR and CBC encryption algorithms written in C. (https://github.com/kokke/tiny-AES-c)
* Uri: C++ Network URI (http://www.boost.org/)
* UTF8-CPP: UTF-8 with C++ (http://utfcpp.sourceforge.net/)
* xxhash_cpp: xxHash library to C++17. (https://cyan4973.github.io/xxHash/)
* Zlib: Compression Libraries (https://zlib.net/)
