// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_TEST_UTILITY_META_HPP
#define BOOST_HISTOGRAM_TEST_UTILITY_META_HPP

#include <array>
#include <boost/core/is_same.hpp>
#include <boost/core/lightweight_test_trait.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/mp11/utility.hpp>
#include <ostream>
#include <vector>

namespace std {
// never add to std, we only do it here to get ADL working :(
template <typename T>
ostream& operator<<(ostream& os, const vector<T>& v) {
  os << "[ ";
  for (const auto& x : v) os << x << " ";
  os << "]";
  return os;
}

template <class... Ts>
ostream& operator<<(ostream& os, const std::tuple<Ts...>& t) {
  os << "[ ";
  ::boost::mp11::tuple_for_each(t, [&os](const auto& x) { os << x << " "; });
  os << "]";
  return os;
}

template <class T, class U>
ostream& operator<<(ostream& os, const std::pair<T, U>& t) {
  os << "[ " << t.first << " " << t.second << " ]";
  return os;
}
} // namespace std

#ifndef BOOST_TEST_TRAIT_SAME
// temporary macro implementation so that BOOST_TEST_TRAIT_SAME already works on travis
// and appveyor with old boost-1.69.0

#define BOOST_TEST_TRAIT_SAME(...) \
  BOOST_TEST_TRAIT_TRUE((::boost::core::is_same<__VA_ARGS__>))
#endif

#endif
