// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <benchmark/benchmark.h>
#include <atomic>
#include <boost/histogram.hpp>
#include <boost/histogram/algorithm/sum.hpp>
#include <chrono>
#include <functional>
#include <numeric>
#include <thread>
#include <vector>

namespace bh = boost::histogram;

template <typename T>
class copyable_atomic : public std::atomic<T> {
public:
  using std::atomic<T>::atomic;

  // zero-initialize the atomic T
  copyable_atomic() noexcept : std::atomic<T>(T()) {}

  // this is potentially not thread-safe, see below
  copyable_atomic(const copyable_atomic& rhs) : std::atomic<T>() { this->operator=(rhs); }

  // this is potentially not thread-safe, see below
  copyable_atomic& operator=(const copyable_atomic& rhs) {
    if (this != &rhs) { std::atomic<T>::store(rhs.load()); }
    return *this;
  }
};

template <class Histogram>
void fill_histogram(Histogram& h, unsigned ndata) {
  using namespace std::chrono_literals;
  for (unsigned i = 0; i < ndata; ++i) {
    std::this_thread::sleep_for(10ns); // simulate some work
    h(double(i) / ndata);
  }
}

static void NoThreads(benchmark::State& state) {
  auto h =
      bh::make_histogram_with(std::vector<unsigned>(), bh::axis::regular<>(100, 0, 1));
  const unsigned ndata = state.range(0);
  for (auto _ : state) {
    state.PauseTiming();
    h.reset();
    state.ResumeTiming();
    fill_histogram(h, ndata);
    state.PauseTiming();
    assert(ndata == std::accumulate(h.begin(), h.end(), 0.0));
    state.ResumeTiming();
  }
}

static void AtomicStorage(benchmark::State& state) {
  auto h = bh::make_histogram_with(std::vector<copyable_atomic<unsigned>>(),
                                   bh::axis::regular<>(100, 0, 1));

  const unsigned nthreads = state.range(0);
  const unsigned ndata = state.range(1);
  for (auto _ : state) {
    state.PauseTiming();
    h.reset();
    std::vector<std::thread> pool;
    pool.reserve(nthreads);
    state.ResumeTiming();
    for (unsigned i = 0; i < nthreads; ++i)
      pool.emplace_back(fill_histogram<decltype(h)>, std::ref(h), ndata / nthreads);
    for (auto&& t : pool) t.join();
    state.PauseTiming();
    assert(ndata == std::accumulate(h.begin(), h.end(), 0.0));
    state.ResumeTiming();
  }
}

BENCHMARK(NoThreads)->RangeMultiplier(2)->Range(8 << 3, 8 << 5);
BENCHMARK(AtomicStorage)->RangeMultiplier(2)->Ranges({{1, 4}, {8 << 3, 8 << 5}});
