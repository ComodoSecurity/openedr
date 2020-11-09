// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <benchmark/benchmark.h>
#include <boost/histogram/axis.hpp>

using namespace boost::histogram;

template <bool include_extra_bins>
static void null(benchmark::State& state) {
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i)
      volatile int j = 0;
  }
}

template <bool include_extra_bins>
static void regular(benchmark::State& state) {
  volatile auto start = 0;
  volatile auto stop = 10;
  auto a = axis::regular<>(stop - start, start, stop);
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i) {
      volatile int j;
      benchmark::DoNotOptimize(a.index(i));
    }
  }
}

template <bool include_extra_bins>
static void circular(benchmark::State& state) {
  volatile auto start = 0;
  volatile auto stop = 10;
  auto a = axis::circular<>(stop - start, start, stop);
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i) {
      benchmark::DoNotOptimize(a.index(i));
    }
  }
}

template <bool include_extra_bins>
static void integer_int(benchmark::State& state) {
  volatile auto start = 0;
  volatile auto stop = 10;
  auto a = axis::integer<int>(start, stop);
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i) {
      volatile int j;
      benchmark::DoNotOptimize(a.index(i));
    }
  }
}

template <bool include_extra_bins>
static void integer_double(benchmark::State& state) {
  volatile auto start = 0;
  volatile auto stop = 10;
  auto a = axis::integer<double>(start, stop);
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i) {
      benchmark::DoNotOptimize(a.index(i));
    }
  }
}

template <bool include_extra_bins>
static void variable(benchmark::State& state) {
  auto a = axis::variable<>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i)
      benchmark::DoNotOptimize(a.index(i));
  }
}

template <bool include_extra_bins>
static void category(benchmark::State& state) {
  auto a = axis::category<int>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i)
      benchmark::DoNotOptimize(a.index(i));
  }
}

template <bool include_extra_bins>
static void variant(benchmark::State& state) {
  auto a = axis::variant<axis::regular<>>(axis::regular<>(10, 0, 10));
  for (auto _ : state) {
    for (int i = 0 - include_extra_bins; i < 10 + include_extra_bins; ++i)
      benchmark::DoNotOptimize(a.index(i));
  }
}

BENCHMARK_TEMPLATE(null, false);
BENCHMARK_TEMPLATE(null, true);
BENCHMARK_TEMPLATE(regular, false);
BENCHMARK_TEMPLATE(regular, true);
BENCHMARK_TEMPLATE(circular, false);
BENCHMARK_TEMPLATE(circular, true);
BENCHMARK_TEMPLATE(integer_int, false);
BENCHMARK_TEMPLATE(integer_int, true);
BENCHMARK_TEMPLATE(integer_double, false);
BENCHMARK_TEMPLATE(integer_double, true);
BENCHMARK_TEMPLATE(variable, false);
BENCHMARK_TEMPLATE(variable, true);
BENCHMARK_TEMPLATE(category, false);
BENCHMARK_TEMPLATE(category, true);
