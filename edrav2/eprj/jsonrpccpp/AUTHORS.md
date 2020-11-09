Peter Spiess-Knafl <dev@spiessknafl.at>
+ core library
+ cmake build system
+ stubgenerator

*Notes about this list:* I try to update this list regularly, but sometimes I just forget. If I happen to oversee someones contributions, please raise an issue for that and sorry in advance. For an always up to date genarated list, see at the [GitHub contributors list](https://github.com/cinemast/libjson-rpc-cpp/graphs/contributors).

Feature contributions (chronological order)
===========================================

Joegen Baclor <joegen@ossapp.com>
+ automake/autoconf support
+ RPM packaging

Bertrand Cachet <bertrand.cachet@gmail.com>
+ adapted build file for opt out of HTTP connectors
+ various bugfixes and support

Marc Trudel <mtrudel@wizcorp.jp>
+ extended HTTP client connector

Yuriy Syrovetskiy <cblp@cblp.su>
+ added user supplied error blocks

Veselin Rachev <veselin.raychev@gmail.com>
+ added HTTP OPTIONS request support

Marek Kotewicz <marek.kotewicz@gmail.com>
+ msvc support

Alexandre Poirot <alexandre.poirot@gmail.com>
+ added client and server connectors that use Unix Domain Sockets
+ adapted build file to generate pkg-config file for this lib
+ added client and server connectors that use Tcp Sockets on Linux and Windows (uses native socket and thread API on each OS)

Trevor Vannoy <trevor.vannoy@flukecal.com>
+ python client stub generator

Jean-Daniel Michaud <jean.daniel,michaud@gmail.com>
+ added server/client file descriptor support

Jacques Software <software@jacques.com.au>
+ added support for redis connector

Bugfixes (chronological order)
==============================

Emilien Kenler <hello@emilienkenler.com>
+ bugfixes in HTTP client connector

kaidokert
+ bugfixes in parameter handling

Nozomu Kaneko <nozom.kaneko@gmail.com>
+ bugfixes in batch requests

caktoux
+ bugfixes in build system

Julien Jorge <julien.jorge@stuff-o-matic.com>
+ bugfixes in batch requests

Pascal Heijnsdijk <pascal@heijnsdijk.nl>
+ Htttp Client timeout bugfix and memory leak.

Kasper Laudrup <laudrup@stacktrace.dk>
+ Code reviews

Erik Lundin
+ bugfix in cpp-server stubgen
+ bugfix for gcc 4.7 compatibility

Michał Górny <mgorny@gentoo.org>
+ bugfixes in the build system

Trevor Vannoy <trevor.vannoy@flukecal.com>
+ fixed tcp socket client compile issues
