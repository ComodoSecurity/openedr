#pragma once

#include <libcore/inc/libcore.h>
#include <libcloud/inc/libcloud.h>
#include <libsyswin/inc/libsyswin.h>

#include <map>
#include <tuple>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

#define BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN
#include <boost/beast/http/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <catch2/catch.hpp>
#include <libcore/test/common.h>
