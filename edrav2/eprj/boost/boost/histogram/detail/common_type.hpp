// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_COMMON_TYPE_HPP
#define BOOST_HISTOGRAM_DETAIL_COMMON_TYPE_HPP

#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>
#include <tuple>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {
// clang-format off
template <class T, class U>
using common_axes = mp11::mp_cond<
  is_tuple<T>, T,
  is_tuple<U>, U,
  is_sequence_of_axis<T>, T,
  is_sequence_of_axis<U>, U,
  std::true_type, T
>;

template <class T, class U>
using common_container = mp11::mp_cond<
  is_array_like<T>, T,
  is_array_like<U>, U,
  is_vector_like<T>, T,
  is_vector_like<U>, U,
  std::true_type, T
>;
// clang-format on

template <class T>
using type_score = mp11::mp_size_t<((!std::is_pod<T>::value) * 1000 +
                                    std::is_floating_point<T>::value * 50 + sizeof(T))>;

template <class T, class U>
struct common_storage_impl;

template <class T, class U>
struct common_storage_impl<storage_adaptor<T>, storage_adaptor<U>> {
  using type =
      mp11::mp_if_c<(type_score<typename storage_adaptor<T>::value_type>::value >=
                     type_score<typename storage_adaptor<U>::value_type>::value),
                    storage_adaptor<T>, storage_adaptor<U>>;
};

template <class T, class A>
struct common_storage_impl<storage_adaptor<T>, unlimited_storage<A>> {
  using type =
      mp11::mp_if_c<(type_score<typename storage_adaptor<T>::value_type>::value >=
                     type_score<typename unlimited_storage<A>::value_type>::value),
                    storage_adaptor<T>, unlimited_storage<A>>;
};

template <class C, class A>
struct common_storage_impl<unlimited_storage<A>, storage_adaptor<C>>
    : common_storage_impl<storage_adaptor<C>, unlimited_storage<A>> {};

template <class A1, class A2>
struct common_storage_impl<unlimited_storage<A1>, unlimited_storage<A2>> {
  using type = unlimited_storage<A1>;
};

template <class A, class B>
using common_storage = typename common_storage_impl<A, B>::type;
} // namespace detail
} // namespace histogram
} // namespace boost

#endif
