// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_COMPRESSED_PAIR_HPP
#define BOOST_HISTOGRAM_DETAIL_COMPRESSED_PAIR_HPP

#include <type_traits>
#include <utility>

namespace boost {
namespace histogram {
namespace detail {

template <typename T1, typename T2, bool B>
class compressed_pair_impl;

template <typename T1, typename T2>
class compressed_pair_impl<T1, T2, true> : protected T2 {
public:
  template <typename U1, typename U2>
  compressed_pair_impl(U1&& u1, U2&& u2)
      : T2(std::forward<U2>(u2)), first_(std::forward<U1>(u1)) {}
  template <typename U1>
  compressed_pair_impl(U1&& u1) : first_(std::forward<U1>(u1)) {}
  compressed_pair_impl() = default;

  T1& first() { return first_; }
  T2& second() { return static_cast<T2&>(*this); }
  const T1& first() const { return first_; }
  const T2& second() const { return static_cast<const T2&>(*this); }

private:
  T1 first_;
};

template <typename T1, typename T2>
class compressed_pair_impl<T1, T2, false> {
public:
  template <typename U1, typename U2>
  compressed_pair_impl(U1&& u1, U2&& u2)
      : first_(std::forward<U1>(u1)), second_(std::forward<U2>(u2)) {}
  template <typename U1>
  compressed_pair_impl(U1&& u1) : first_(std::forward<U1>(u1)) {}
  compressed_pair_impl() = default;

  T1& first() { return first_; }
  T2& second() { return second_; }
  const T1& first() const { return first_; }
  const T2& second() const { return second_; }

private:
  T1 first_;
  T2 second_;
};

template <typename... Ts>
void swap(compressed_pair_impl<Ts...>& a, compressed_pair_impl<Ts...>& b) {
  using std::swap;
  swap(a.first(), b.first());
  swap(a.second(), b.second());
}

template <typename T1, typename T2>
using compressed_pair =
    compressed_pair_impl<T1, T2, (!std::is_final<T2>::value && std::is_empty<T2>::value)>;

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
