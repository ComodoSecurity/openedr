// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_AXES_HPP
#define BOOST_HISTOGRAM_DETAIL_AXES_HPP

#include <boost/assert.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <unsigned N, class... Ts>
decltype(auto) axis_get(std::tuple<Ts...>& axes) {
  return std::get<N>(axes);
}

template <unsigned N, class... Ts>
decltype(auto) axis_get(const std::tuple<Ts...>& axes) {
  return std::get<N>(axes);
}

template <unsigned N, class T>
decltype(auto) axis_get(T& axes) {
  return axes[N];
}

template <unsigned N, class T>
decltype(auto) axis_get(const T& axes) {
  return axes[N];
}

template <class... Ts>
decltype(auto) axis_get(std::tuple<Ts...>& axes, unsigned i) {
  return mp11::mp_with_index<sizeof...(Ts)>(
      i, [&](auto I) { return axis::variant<Ts&...>(std::get<I>(axes)); });
}

template <class... Ts>
decltype(auto) axis_get(const std::tuple<Ts...>& axes, unsigned i) {
  return mp11::mp_with_index<sizeof...(Ts)>(
      i, [&](auto I) { return axis::variant<const Ts&...>(std::get<I>(axes)); });
}

template <class T>
decltype(auto) axis_get(T& axes, unsigned i) {
  return axes.at(i);
}

template <class T>
decltype(auto) axis_get(const T& axes, unsigned i) {
  return axes.at(i);
}

template <class... Ts, class... Us>
bool axes_equal(const std::tuple<Ts...>& ts, const std::tuple<Us...>& us) {
  return static_if<std::is_same<mp11::mp_list<Ts...>, mp11::mp_list<Us...>>>(
      [](const auto& ts, const auto& us) {
        using N = mp11::mp_size<remove_cvref_t<decltype(ts)>>;
        bool equal = true;
        mp11::mp_for_each<mp11::mp_iota<N>>(
            [&](auto I) { equal &= relaxed_equal(std::get<I>(ts), std::get<I>(us)); });
        return equal;
      },
      [](const auto&, const auto&) { return false; }, ts, us);
}

template <class... Ts, class U>
bool axes_equal(const std::tuple<Ts...>& t, const U& u) {
  if (sizeof...(Ts) != u.size()) return false;
  bool equal = true;
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Ts)>>(
      [&](auto I) { equal &= u[I] == std::get<I>(t); });
  return equal;
}

template <class T, class... Us>
bool axes_equal(const T& t, const std::tuple<Us...>& u) {
  return axes_equal(u, t);
}

template <class T, class U>
bool axes_equal(const T& t, const U& u) {
  if (t.size() != u.size()) return false;
  return std::equal(t.begin(), t.end(), u.begin());
}

template <class... Ts, class... Us>
void axes_assign(std::tuple<Ts...>& t, const std::tuple<Us...>& u) {
  static_if<std::is_same<mp11::mp_list<Ts...>, mp11::mp_list<Us...>>>(
      [](auto& a, const auto& b) { a = b; },
      [](auto&, const auto&) {
        BOOST_THROW_EXCEPTION(
            std::invalid_argument("cannot assign axes, types do not match"));
      },
      t, u);
}

template <class... Ts, class U>
void axes_assign(std::tuple<Ts...>& t, const U& u) {
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Ts)>>([&](auto I) {
    using T = mp11::mp_at_c<std::tuple<Ts...>, I>;
    std::get<I>(t) = axis::get<T>(u[I]);
  });
}

template <class T, class... Us>
void axes_assign(T& t, const std::tuple<Us...>& u) {
  // resize instead of reserve, because t may not be empty and we want exact capacity
  t.resize(sizeof...(Us));
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Us)>>(
      [&](auto I) { t[I] = std::get<I>(u); });
}

template <typename T, typename U>
void axes_assign(T& t, const U& u) {
  t.assign(u.begin(), u.end());
}

template <typename T>
void axis_index_is_valid(const T& axes, const unsigned N) {
  BOOST_ASSERT_MSG(N < get_size(axes), "index out of range");
}

template <typename F, typename T>
void for_each_axis_impl(std::true_type, const T& axes, F&& f) {
  for (const auto& x : axes) { axis::visit(std::forward<F>(f), x); }
}

template <typename F, typename T>
void for_each_axis_impl(std::false_type, const T& axes, F&& f) {
  for (const auto& x : axes) std::forward<F>(f)(x);
}

template <typename F, typename T>
void for_each_axis(const T& axes, F&& f) {
  using U = mp11::mp_first<T>;
  for_each_axis_impl(is_axis_variant<U>(), axes, std::forward<F>(f));
}

template <typename F, typename... Ts>
void for_each_axis(const std::tuple<Ts...>& axes, F&& f) {
  mp11::tuple_for_each(axes, std::forward<F>(f));
}

template <typename T>
std::size_t bincount(const T& axes) {
  std::size_t n = 1;
  for_each_axis(axes, [&n](const auto& a) {
    const auto old = n;
    const auto s = axis::traits::extent(a);
    n *= s;
    if (s > 0 && n < old) BOOST_THROW_EXCEPTION(std::overflow_error("bincount overflow"));
  });
  return n;
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
