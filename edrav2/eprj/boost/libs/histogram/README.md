# Histogram

**Fast multi-dimensional histogram with convenient interface for C++14**

Coded with ❤. Powered by the [Boost community](https://www.boost.org) and the [Scikit-HEP Project](http://scikit-hep.org).

Branch  | Linux [1] and OSX [2] | Windows [3] | Coverage
------- | --------------------- |------------ | --------
develop | [![Build Status Travis](https://travis-ci.com/HDembinski/histogram.svg?branch=develop)](https://travis-ci.com/HDembinski/histogram?branch=develop) | [![Build status](https://ci.appveyor.com/api/projects/status/400lx25l3jdpk96b/branch/develop?svg=true)](https://ci.appveyor.com/project/HDembinski/histogram/branch/develop) | [![codecov](https://codecov.io/gh/HDembinski/histogram/branch/develop/graph/badge.svg)](https://codecov.io/gh/HDembinski/histogram/branch/develop)
master  | [![Build Status Travis](https://travis-ci.com/HDembinski/histogram.svg?branch=master)](https://travis-ci.com/HDembinski/histogram?branch=master) | [![Build status Appveyor](https://ci.appveyor.com/api/projects/status/400lx25l3jdpk96b/branch/master?svg=true)](https://ci.appveyor.com/project/HDembinski/histogram/branch/master) | [![codecov](https://codecov.io/gh/HDembinski/histogram/branch/master/graph/badge.svg)](https://codecov.io/gh/HDembinski/histogram/branch/master)

1. gcc-5.5.0, clang-5.0.0
2. Xcode 9.4
3. Visual Studio 15 2017

This **header-only** open-source library provides an easy-to-use state-of-the-art multi-dimensional [histogram](https://en.wikipedia.org/wiki/Histogram) class for the professional statistician and everyone who needs to count things. The histogram is easy to use for the casual user and yet very customizable. It does not restrict the power-user. The library offers a unique safety guarantee: cell counts *cannot overflow* or *be capped* in the standard configuration. And it can do more than counting. The histogram can be equipped with arbitrary accumulators to compute means, medians, and whatever you fancy in each cell. The library is very fast single-threaded (see benchmarks) and several parallelization options are provided for multi-threaded programming.

The project has passed Boost review in September 2018 and is going to be first released with [Boost-1.70](http://www.boost.org) in April. The source code is licensed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).

Check out the [full documentation](https://www.boost.org/doc/libs/develop/libs/histogram/doc/html/index.html). [Python bindings](https://github.com/hdembinski/histogram-python) to this library are available elsewhere.

## Code example

The following stripped-down example was taken from the [Getting started](https://www.boost.org/doc/libs/develop/libs/histogram/doc/html/histogram/getting_started.html) section in the documentation. Have a look into the docs to see the full version with comments and more examples.

Example: Fill a 1d-histogram

```cpp
#include <boost/histogram.hpp>
#include <boost/format.hpp> // used here for printing
#include <functional> // for std::ref

int main() {
    using namespace boost::histogram;

    auto h = make_histogram(
      axis::regular<>(6, -1.0, 2.0, "x")
    );

    // fill histogram
    auto data = { -10, -0.4, 1.1, 0.3, 1.3 };
    std::for_each(data.begin(), data.end(), std::ref(h));

    // iterate over bins
    for (auto x : indexed(h)) {
      std::cout << boost::format("bin %2i [%4.1f, %4.1f): %i\n")
        % x.index() % x.bin().lower() % x.bin().upper() % *x;
    }

    std::cout << std::flush;

    /* program output:

    bin -1 [-inf, -1.0): 1
    bin  0 [-1.0, -0.5): 0
    bin  1 [-0.5, -0.0): 1
    bin  2 [-0.0,  0.5): 1
    bin  3 [ 0.5,  1.0): 0
    bin  4 [ 1.0,  1.5): 2
    bin  5 [ 1.5,  2.0): 0
    bin  6 [ 2.0,  inf): 0
    */
}
```

## Features

* Extremely customisable multi-dimensional histogram
* Simple, convenient, STL and Boost-compatible interface
* Counters with high dynamic range, cannot overflow or be capped (1)
* Better performance than other libraries (see benchmarks for details)
* Efficient use of memory (1)
* Support for custom axis types: define how input values should map to indices
* Support for under-/overflow bins (can be disabled individually to reduce memory consumption)
* Support for axes that grow automatically with input values (2)
* Support for weighted increments
* Support for profiles and more generally, user-defined accumulators in cells (3)
* Support for completely stack-based histograms
* Support for adding and scaling histograms
* Support for custom allocators
* Support for programming with units (4)
* Optional serialization based on [Boost.Serialization](https://www.boost.org/doc/libs/release/libs/serialization/)

1. In the standard configuration, if you don't use weighted increments. The counter capacity is increased dynamically as the cell counts grow. When even the largest plain integral type would overflow, the storage switches to a multiprecision integer similar to those in [Boost.Multiprecision](https://www.boost.org/doc/libs/release/libs/multiprecision/), which is only limited by available memory.
2. An axis can be configured to grow when a value is encountered that is outside of its range. It then grows new bins towards this value so that the value ends up in the new highest or lowest bin.
3. The histogram can be configured to hold an arbitrary accumulator in each cell instead of a simple counter. Extra values can be passed to the histogram, for example, to compute the mean and variance of values which fall into the same cell. This can be used to compute variance estimates for each cell. These are useful when histograms are to be compared quantitatively and if a statistical model is fitted to the cell values.
4. Builtin axis types can configured to only accept dimensional quantities, like those from [Boost.Units](https://www.boost.org/doc/libs/release/libs/units/). This means you get an error if you try to fill a length when the histogram axis expects a time, for example.

## Benchmarks

Boost.Histogram is more flexible and faster than other C/C++ libraries. It was compared to:
 - [GNU Scientific Library](https://www.gnu.org/software/gsl)
 - [ROOT framework from CERN](https://root.cern.ch)

Details on the benchmark are given in the [documentation](https://www.boost.org/doc/libs/develop/libs/histogram/doc/html/histogram/benchmarks.html).

## What users say

**John Buonagurio** | Manager at [**E<sup><i>x</i></sup>ponent<sup>&reg;</sup>**](www.exponent.com)

*"I just wanted to say 'thanks' for your awesome Histogram library. I'm working on a software package for processing meteorology data and I'm using it to generate wind roses with the help of Qt and QwtPolar. Looks like you thought of just about everything here &ndash; the circular axis type was practically designed for this application, everything 'just worked'."*
