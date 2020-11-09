// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <sstream>
#include <tuple>
#include <vector>
#include "utility_allocator.hpp"
#include "utility_meta.hpp"

using namespace boost::histogram;

int main() {
  // vector streaming
  {
    std::ostringstream os;
    std::vector<int> v = {1, 3, 2};
    os << v;
    BOOST_TEST_EQ(os.str(), std::string("[ 1 3 2 ]"));
  }

  // tuple streaming
  {
    std::ostringstream os;
    auto v = std::make_tuple(1, 2.5, "hi");
    os << v;
    BOOST_TEST_EQ(os.str(), std::string("[ 1 2.5 hi ]"));
  }

  // tracing_allocator
  {
    tracing_allocator_db db;
    tracing_allocator<char> a(db);
    auto p1 = a.allocate(2);
    a.deallocate(p1, 2);
    tracing_allocator<int> b(a);
    auto p2 = b.allocate(3);
    b.deallocate(p2, 3);
    BOOST_TEST_EQ(db.size(), 2);
    BOOST_TEST_EQ(db[&BOOST_CORE_TYPEID(char)].first, 2);
    BOOST_TEST_EQ(db[&BOOST_CORE_TYPEID(char)].second, 2);
    BOOST_TEST_EQ(db[&BOOST_CORE_TYPEID(int)].first, 3);
    BOOST_TEST_EQ(db[&BOOST_CORE_TYPEID(int)].second, 3);
  }

  return boost::report_errors();
}
