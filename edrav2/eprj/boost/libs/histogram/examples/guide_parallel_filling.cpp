// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

//[ guide_parallel_filling

#include <atomic>
#include <boost/histogram.hpp>
#include <boost/histogram/algorithm/sum.hpp>
#include <cassert>
#include <functional>
#include <thread>
#include <vector>

// dummy fill function, to be executed in parallel by several threads
template <typename Histogram>
void fill(Histogram& h) {
  for (unsigned i = 0; i < 1000; ++i) { h(i % 10); }
}

/*
  std::atomic has deleted copy ctor, we need to wrap it in a type with a
  potentially unsafe copy ctor. It can be used in a thread-safe way if some
  rules are followed, see below.
*/
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

int main() {
  using namespace boost::histogram;

  /*
    Create histogram with container of atomic counters for parallel filling in
    several threads. You cannot use bare std::atomic here, because std::atomic
    types are not copyable. Using the copyable_atomic as a work-around is safe,
    if the storage does not change size while it is filled. This means that
    growing axis types are not allowed.
  */
  auto h = make_histogram_with(std::vector<copyable_atomic<std::size_t>>(),
                               axis::integer<>(0, 10));

  /*
    The histogram storage may not be resized in either thread.
    This is the case, if you do not use growing axis types.
  */
  auto fill_h = [&h]() { fill(h); };
  std::thread t1(fill_h);
  std::thread t2(fill_h);
  std::thread t3(fill_h);
  std::thread t4(fill_h);
  t1.join();
  t2.join();
  t3.join();
  t4.join();

  assert(algorithm::sum(h) == 4000);
}

//]
