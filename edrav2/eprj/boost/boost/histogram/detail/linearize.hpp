// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_LINEARIZE_HPP
#define BOOST_HISTOGRAM_DETAIL_LINEARIZE_HPP

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/detail/axes.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/histogram/unsafe_access.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
struct is_accumulator_set : std::false_type {};

template <class T>
using has_underflow =
    decltype(axis::traits::static_options<T>::test(axis::option::underflow));

template <class T>
struct is_growing
    : decltype(axis::traits::static_options<T>::test(axis::option::growth)) {};

template <class... Ts>
struct is_growing<std::tuple<Ts...>> : mp11::mp_or<is_growing<Ts>...> {};

template <class... Ts>
struct is_growing<axis::variant<Ts...>> : mp11::mp_or<is_growing<Ts>...> {};

template <class T>
using has_growing_axis =
    mp11::mp_if<is_vector_like<T>, is_growing<mp11::mp_first<T>>, is_growing<T>>;

/// Index with an invalid state
struct optional_index {
  std::size_t idx = 0;
  std::size_t stride = 1;
  operator bool() const { return stride > 0; }
  std::size_t operator*() const { return idx; }
};

inline void linearize(optional_index& out, const axis::index_type extent,
                      const axis::index_type j) noexcept {
  // j is internal index shifted by +1 if axis has underflow bin
  out.idx += j * out.stride;
  // set stride to 0, if j is invalid
  out.stride *= (0 <= j && j < extent) * extent;
}

// for non-growing axis
template <class Axis, class Value>
void linearize_value(optional_index& o, const Axis& a, const Value& v) {
  using B = decltype(axis::traits::static_options<Axis>::test(axis::option::underflow));
  const auto j = axis::traits::index(a, v) + B::value;
  linearize(o, axis::traits::extent(a), j);
}

// for variant that does not contain any growing axis
template <class... Ts, class Value>
void linearize_value(optional_index& o, const axis::variant<Ts...>& a, const Value& v) {
  axis::visit([&o, &v](const auto& a) { linearize_value(o, a, v); }, a);
}

// for growing axis
template <class Axis, class Value>
void linearize_value(optional_index& o, axis::index_type& s, Axis& a, const Value& v) {
  axis::index_type j;
  std::tie(j, s) = axis::traits::update(a, v);
  j += has_underflow<Axis>::value;
  linearize(o, axis::traits::extent(a), j);
}

// for variant which contains at least one growing axis
template <class... Ts, class Value>
void linearize_value(optional_index& o, axis::index_type& s, axis::variant<Ts...>& a,
                     const Value& v) {
  axis::visit([&o, &s, &v](auto&& a) { linearize_value(o, s, a, v); }, a);
}

template <class A>
void linearize_index(optional_index& out, const A& axis, const axis::index_type j) {
  // A may be axis or variant, cannot use static option detection here
  const auto opt = axis::traits::options(axis);
  const auto shift = opt & axis::option::underflow ? 1 : 0;
  const auto n = axis.size() + (opt & axis::option::overflow ? 1 : 0);
  linearize(out, n + shift, j + shift);
}

template <class S, class A, class T>
void maybe_replace_storage(S& storage, const A& axes, const T& shifts) {
  bool update_needed = false;
  auto sit = shifts;
  for_each_axis(axes, [&](const auto&) { update_needed |= (*sit++ != 0); });
  if (!update_needed) return;
  struct item {
    axis::index_type idx, old_extent;
    std::size_t new_stride;
  } data[buffer_size<A>::value];
  sit = shifts;
  auto dit = data;
  std::size_t s = 1;
  for_each_axis(axes, [&](const auto& a) {
    const auto n = axis::traits::extent(a);
    *dit++ = {0, n - std::abs(*sit++), s};
    s *= n;
  });
  auto new_storage = make_default(storage);
  new_storage.reset(detail::bincount(axes));
  const auto dlast = data + get_size(axes) - 1;
  for (const auto& x : storage) {
    auto ns = new_storage.begin();
    sit = shifts;
    dit = data;
    for_each_axis(axes, [&](const auto& a) {
      using opt = axis::traits::static_options<decltype(a)>;
      if (opt::test(axis::option::underflow)) {
        if (dit->idx == 0) {
          // axis has underflow and we are in the underflow bin:
          // keep storage pointer unchanged
          ++dit;
          ++sit;
          return;
        }
      }
      if (opt::test(axis::option::overflow)) {
        if (dit->idx == dit->old_extent - 1) {
          // axis has overflow and we are in the overflow bin:
          // move storage pointer to corresponding overflow bin position
          ns += (axis::traits::extent(a) - 1) * dit->new_stride;
          ++dit;
          ++sit;
          return;
        }
      }
      // we are in a normal bin:
      // move storage pointer to index position, apply positive shifts
      ns += (dit->idx + std::max(*sit, 0)) * dit->new_stride;
      ++dit;
      ++sit;
    });
    // assign old value to new location
    *ns = x;
    // advance multi-dimensional index
    dit = data;
    ++dit->idx;
    while (dit != dlast && dit->idx == dit->old_extent) {
      dit->idx = 0;
      ++(++dit)->idx;
    }
  }
  storage = std::move(new_storage);
}

// special case: if histogram::operator()(tuple(1, 2)) is called on 1d histogram
// with axis that accepts 2d tuple, this should not fail
// - solution is to forward tuples of size > 1 directly to axis for 1d
// histograms
// - has nice side-effect of making histogram::operator(1, 2) work as well
// - cannot detect call signature of axis at compile-time in all configurations
//   (axis::variant provides generic call interface and hides concrete
//   interface), so we throw at runtime if incompatible argument is passed (e.g.
//   3d tuple)
// histogram has only non-growing axes
template <unsigned I, unsigned N, class S, class T, class U>
optional_index args_to_index(std::false_type, S&, const T& axes, const U& args) {
  optional_index idx;
  const auto rank = get_size(axes);
  if (rank == 1 && N > 1)
    linearize_value(idx, axis_get<0>(axes), tuple_slice<I, N>(args));
  else {
    if (rank != N)
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("number of arguments != histogram rank"));
    constexpr unsigned M = buffer_size<remove_cvref_t<decltype(axes)>>::value;
    mp11::mp_for_each<mp11::mp_iota_c<(N < M ? N : M)>>([&](auto J) {
      linearize_value(idx, axis_get<J>(axes), std::get<(J + I)>(args));
    });
  }
  return idx;
}

// histogram has growing axes
template <unsigned I, unsigned N, class S, class T, class U>
optional_index args_to_index(std::true_type, S& storage, T& axes, const U& args) {
  optional_index idx;
  axis::index_type shifts[buffer_size<T>::value];
  const auto rank = get_size(axes);
  if (rank == 1 && N > 1)
    linearize_value(idx, shifts[0], axis_get<0>(axes), tuple_slice<I, N>(args));
  else {
    if (rank != N)
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("number of arguments != histogram rank"));
    constexpr unsigned M = buffer_size<remove_cvref_t<decltype(axes)>>::value;
    mp11::mp_for_each<mp11::mp_iota_c<(N < M ? N : M)>>([&](auto J) {
      linearize_value(idx, shifts[J], axis_get<J>(axes), std::get<(J + I)>(args));
    });
  }
  maybe_replace_storage(storage, axes, shifts);
  return idx;
}

template <typename U>
constexpr auto weight_sample_indices() {
  if (is_weight<U>::value) return std::make_pair(0, -1);
  if (is_sample<U>::value) return std::make_pair(-1, 0);
  return std::make_pair(-1, -1);
}

template <typename U0, typename U1, typename... Us>
constexpr auto weight_sample_indices() {
  using L = mp11::mp_list<U0, U1, Us...>;
  const int n = sizeof...(Us) + 1;
  if (is_weight<mp11::mp_at_c<L, 0>>::value) {
    if (is_sample<mp11::mp_at_c<L, 1>>::value) return std::make_pair(0, 1);
    if (is_sample<mp11::mp_at_c<L, n>>::value) return std::make_pair(0, n);
    return std::make_pair(0, -1);
  }
  if (is_sample<mp11::mp_at_c<L, 0>>::value) {
    if (is_weight<mp11::mp_at_c<L, 1>>::value) return std::make_pair(1, 0);
    if (is_weight<mp11::mp_at_c<L, n>>::value) return std::make_pair(n, 0);
    return std::make_pair(-1, 0);
  }
  if (is_weight<mp11::mp_at_c<L, n>>::value) {
    // 0, n already covered
    if (is_sample<mp11::mp_at_c<L, (n - 1)>>::value) return std::make_pair(n, n - 1);
    return std::make_pair(n, -1);
  }
  if (is_sample<mp11::mp_at_c<L, n>>::value) {
    // n, 0 already covered
    if (is_weight<mp11::mp_at_c<L, (n - 1)>>::value) return std::make_pair(n - 1, n);
    return std::make_pair(-1, n);
  }
  return std::make_pair(-1, -1);
}

template <class T, class U>
void fill_storage(mp11::mp_int<-1>, mp11::mp_int<-1>, T&& t, U&&) {
  static_if<is_incrementable<remove_cvref_t<T>>>(
      [](auto&& t) { ++t; }, [](auto&& t) { t(); }, std::forward<T>(t));
}

template <class IW, class T, class U>
void fill_storage(IW, mp11::mp_int<-1>, T&& t, U&& args) {
  static_if<is_incrementable<remove_cvref_t<T>>>(
      [](auto&& t, const auto& w) { t += w; },
      [](auto&& t, const auto& w) {
#ifdef BOOST_HISTOGRAM_WITH_ACCUMULATORS_SUPPORT
        static_if<is_accumulator_set<remove_cvref_t<T>>>(
            [w](auto&& t) { t(::boost::accumulators::weight = w); },
            [w](auto&& t) { t(w); }, t);
#else
        t(w);
#endif
      },
      std::forward<T>(t), std::get<IW::value>(args).value);
}

template <class IS, class T, class U>
void fill_storage(mp11::mp_int<-1>, IS, T&& t, U&& args) {
  mp11::tuple_apply([&t](auto&&... args) { t(args...); },
                    std::get<IS::value>(args).value);
}

template <class IW, class IS, class T, class U>
void fill_storage(IW, IS, T&& t, U&& args) {
  mp11::tuple_apply(
      [&](auto&&... args2) { t(std::get<IW::value>(args).value, args2...); },
      std::get<IS::value>(args).value);
}

template <class S, class A, class... Us>
auto fill(S& storage, A& axes, const std::tuple<Us...>& args) {
  constexpr auto iws = weight_sample_indices<Us...>();
  constexpr unsigned n = sizeof...(Us) - (iws.first > -1) - (iws.second > -1);
  constexpr unsigned i = (iws.first == 0 || iws.second == 0)
                             ? (iws.first == 1 || iws.second == 1 ? 2 : 1)
                             : 0;
  optional_index idx = args_to_index<i, n>(has_growing_axis<A>(), storage, axes, args);
  if (idx) {
    fill_storage(mp11::mp_int<iws.first>(), mp11::mp_int<iws.second>(), storage[*idx],
                 args);
    return storage.begin() + *idx;
  }
  return storage.end();
}

template <typename A, typename... Us>
optional_index at(const A& axes, const std::tuple<Us...>& args) {
  if (get_size(axes) != sizeof...(Us))
    BOOST_THROW_EXCEPTION(std::invalid_argument("number of arguments != histogram rank"));
  optional_index idx;
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Us)>>([&](auto I) {
    // axes_get works with static and dynamic axes
    linearize_index(idx, axis_get<I>(axes),
                    static_cast<axis::index_type>(std::get<I>(args)));
  });
  return idx;
}

template <typename A, typename U>
optional_index at(const A& axes, const U& args) {
  if (get_size(axes) != get_size(args))
    BOOST_THROW_EXCEPTION(std::invalid_argument("number of arguments != histogram rank"));
  optional_index idx;
  using std::begin;
  auto it = begin(args);
  for_each_axis(axes, [&](const auto& a) {
    linearize_index(idx, a, static_cast<axis::index_type>(*it++));
  });
  return idx;
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
