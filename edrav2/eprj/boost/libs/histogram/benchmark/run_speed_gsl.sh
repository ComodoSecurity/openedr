#!/bin/sh
##
## Copyright 2015-2016 Hans Dembinski
##
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt
## or copy at http://www.boost.org/LICENSE_1_0.txt)

cd `dirname $0`
g++ -std=c++11 -O3 `gsl-config --cflags` speed_gsl.cpp `gsl-config --libs` -lstdc++ -o /tmp/speed_gsl && /tmp/speed_gsl
