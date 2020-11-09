// Copyright 2018-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/workaround.hpp>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4459) // declaration of 'sum' hides global
#endif
#if BOOST_WORKAROUND(BOOST_GCC, >= 50000)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if BOOST_WORKAROUND(BOOST_GCC, >= 60000)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#if BOOST_WORKAROUND(BOOST_CLANG, >= 1)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#if BOOST_WORKAROUND(BOOST_CLANG, >= 1)
#pragma clang diagnostic pop
#endif
#if BOOST_WORKAROUND(BOOST_GCC, >= 60000)
#pragma GCC diagnostic pop
#endif
#if BOOST_WORKAROUND(BOOST_GCC, >= 50000)
#pragma GCC diagnostic pop
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <boost/core/lightweight_test.hpp>
#include <boost/histogram.hpp>

namespace ba = boost::accumulators;

int main() {
  using namespace boost::histogram;

  // mean
  {
    using mean = ba::accumulator_set<double, ba::stats<ba::tag::mean>>;

    auto h = make_histogram_with(dense_storage<mean>(), axis::integer<>(0, 2));
    h(0, sample(1));
    h(0, sample(2));
    h(0, sample(3));
    h(1, sample(2));
    h(1, sample(3));
    BOOST_TEST_EQ(ba::count(h[0]), 3);
    BOOST_TEST_EQ(ba::mean(h[0]), 2);
    BOOST_TEST_EQ(ba::count(h[1]), 2);
    BOOST_TEST_EQ(ba::mean(h[1]), 2.5);
    BOOST_TEST_EQ(ba::count(h[2]), 0);

    auto h2 = h; // copy ok
    BOOST_TEST_EQ(ba::count(h2[0]), 3);
    BOOST_TEST_EQ(ba::mean(h2[0]), 2);
    BOOST_TEST_EQ(ba::count(h2[1]), 2);
    BOOST_TEST_EQ(ba::mean(h2[1]), 2.5);
    BOOST_TEST_EQ(ba::count(h2[2]), 0);
  }
  return boost::report_errors();
}
