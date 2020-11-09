# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [v1.1.1] - 2018-10-31

### Fixed
- Build issue on RHEL7 (#244, #246)
- Build with empty install prefix (#226)
- GnuInstallDirs handling for library targets (#239)
- Disabled libcurl signal handlers (#210)
- Terrible performance for socket based connectors (#229)
- Library versioning error (1.1.0 release actually specified 1.0.0)

### Added
- Missing documentation about python stubgenerator (#222)
- Parameter to enable omitEndingLineFeed() (#213)
- Documenation in examples about throwing serverside errors (#249)

### Changed
- Updated CI images to use Ubuntu 18.04 instead of 17.04
- Disabled FileDescriptor connectors by default
- Removed custom FindCURL cmake module (#237)
- Parameter handling of procedurs without params in stubgenerator

## [v1.1.0] - 2018-01-04
### Fixed
- Fix missing hiredis libs when using only REDIS_CLIENT
- Fix running tests in parallel (#204)
- Fix fetching new version of catch, if it is not installed locally
- Disable UnixDomainSocket Connectors by default, they introduce flaky tests
- Merged MSVC related PR.

## [v1.0.0] - 2017-08-27
### Fixed
- Typo in ERROR_CLIENT_CONNECTOR exception
- Integration testsuite when run without HTTP
- dev/testcoverage.sh script which did not create the build directory
- Indentation in CMakeLists.txt files
- Positional parameters with more than 10 items
- C++11 deprecated dynamic excpetion specifiers have been removed
- libmicrohttpd legacy detection for `EPOLL`

### Added
- File descriptor client and server connector
- Redis client and server connector
- Docker based build system for testing on multiple distributions
- Python client stubgenerator
- CI Integration for OSX build
- `StreamReader` and `StreamWriter` classes to handle the buffering
- [Makefile](Makefile) for developer/contributor related functions

### Removed
- Method `BatchResponse::getResult(Json::Value& id)`
- Method `AbstractServerConnector::SendResponse()`
- Scripts dev/ci.sh, dev/createpackage.sh, dev/installdeps.sh
- `dev/coverage.sh` in favor of `make coverage`
- Windows support, which will hopefully come back soon

### Changed
- Migrated from coveralls.io to codecov.io
- Changed maintainer e-mail address
- Use libmicrohttpd's EPOLL where possible (lmhd >= 0.9.52)
- Added `set -e` to testcoverage.sh script
- Changelog format to [keepachangelog.com](http://keepachangelog.com/en/0.3.0/)
- Refactored all socket-based client and server connectors to reduce code duplication
- Changed interfaces for `AbstractServerConnector` to avoid the ugly `void *` backpointer

## [v0.7.0] - 2016-08-10
### Fixed
- armhf compatibility
- Invalid client id field handling (removed int only check)
- Security issues in unixdomainsocket connectors
- Missing CURL include directive
- Parallel build which failed due to failing CATCH dependency
- Handling 64-bit ids
- Invalid parameter check
- Invalid pointer handling in HTTP-Server

### Added
- HttpServer can now be configured to listen localhost only
- TCP Server + Client connectors

## Changed
- Requiring C++11 support (gcc >= 4.8)

## [v0.6.0] - 2015-06-27
### Added
- pkg-config files for all shared libraries
- UNIX Socket client + server connector
- multiarch support

### Changed
- unit testing framework to catch
- allow disabling shared library build
- split out shared/static library for stubgenerator

## [v0.5.0] - 2015-04-07
### Fixed
- building tests with examples disabled.
- unnecessary rebuilds of stubs on each `make` call.

### Added
- `--version` option to jsonrpcstub.
- msvc support.
- data field support for JsonRpcException.
- contributions guide: https://github.com/cinemast/libjson-rpc-cpp#contributions
- HttpClient uses Http Keep-Alive, which improves performance drastically.
- multiarch support.

### Changed
- Made static library build optional (via `BUILD_STATIC_LIBS`).

## [v0.4.2] - 2015-01-21
### Fixed
- Some spelling mistakes.
- HttpServer with Threading option in SSL startup.

### Changed
- Use CMAKE versioning in manpage.
- Improvied include scheme of jsoncpp.

## [v0.4.1] - 2014-12-01
### Added
- coverity scan support
- [API compatibility report](http://upstream.rosalinux.ru/versions/libjson-rpc-cpp.html)
- Stubgenerator option for protocol switches (JSON-RPC 1.0 & 2.0)

### Changed
- Improved manpage

## [v0.4] - 2014-11-21
### Fixed
- Memory leaks

### Added
- Full WIN32 build support
- JavaScript client stub support
- Improved test coverage (100% line coverage)

### Changed
- Switched Http Server to libmicrohttpd
- Removed TCP Client/Server implementation due to security and codestyle problems.
- Removed dirty pointer stuff in bindAndAddX() methods.
- Using call by value in generated stubs for primitive data types.

## [v0.3.2] - 2014-10-26
### Fixed
- Minor bugs

### Added
- Testcases for client + server -> higher testcoverage
- JSON-RPC 1 Client + Server support

### Changed
- Refactorings in server for JSON-RPC 1 support
- Hiding irrelevant API headers from installation
- Renamed AbstractClientConnector to IClientConnector (please regenearte your client stubs after upgrading)
- Reactivated dev/testcoverage.sh to measure testcoverage.

## [v0.3.1] - 2014-10-22
### Fixed
- Minor bugs

### Added
- Experimental Javascript client to stubgenerator

### Changed
- Changed SOVERSION
- Adapted HTTP Server to enable CORS.

## [v0.3] - 2014-10-19
### Fixed
- Renamed .so files to avoid collisions with makerbot's libjsonrpc.
- Invalid Batchcalls in Client and Server caused runtime exceptions.

### Added
- Namespace/package support for generated stub classes.
- CMake options to enable/disable Stubgenerator, Examples, Connectors and Testsuite.
- Boost-test based unit testing suite, which makes testing more flexible.

### Changed
- Split up server and client into separate libraries
- Lot's of refactorings in the build system and stubgenerator.
- Removed Autotools support (because of all the changes in this release).
- Removed embedded libjson-cpp.
- Simplified spec format: a procedure specification without `return` field is a notification.

# [v0.2.1] - 2013-07-27
### Added
- Support for positional parameters. (see at [example specification](https://github.com/cinemast/libjson-rpc-cpp/blob/master/src/example/spec.json) how to declare them)

## [0.2] - 2013-05-29
### Fixed
- Minor bugs

### Added
- Stub generator for client and server.
- SpecificationWriter to generate Specifications from RPC-Server definitions.
- SpecificationParser to parse a Specification file and generate Methods for the RPC-Server.
- Automated testing after build phase (using `make test`)

### Changed
- Refactored architecture.
- Removed mandatory configuration files (making it more compatible for embedded use cases).
- Updated JsonCPP library
- Update Mongoose library
- Enable SSL Support (provided by mongoose)
- Embedding dependent libraries (to avoid naming conflicts)

## [0.1] - 2013-02-07
### Added
- Initial release
