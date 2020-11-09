// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_TRAITS_HPP
#define BOOST_HISTOGRAM_AXIS_TRAITS_HPP

#include <boost/core/typeinfo.hpp>
#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/detail/cat.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <utility>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
using static_options_impl = axis::option::bitset<T::options()>;

template <class FIntArg, class FDoubleArg, class T>
decltype(auto) value_method_switch(FIntArg&& iarg, FDoubleArg&& darg, const T& t) {
  return static_if<has_method_value<T>>(
      [](FIntArg&& iarg, FDoubleArg&& darg, const auto& t) {
        using A = remove_cvref_t<decltype(t)>;
        return static_if<std::is_same<arg_type<decltype(&A::value), 0>, int>>(
            std::forward<FIntArg>(iarg), std::forward<FDoubleArg>(darg), t);
      },
      [](FIntArg&&, FDoubleArg&&, const auto& t) -> double {
        using A = remove_cvref_t<decltype(t)>;
        BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
            boost::core::demangled_name(BOOST_CORE_TYPEID(A)), " has no value method")));
#ifndef _MSC_VER // msvc warns about unreachable return
        return double{};
#endif
      },
      std::forward<FIntArg>(iarg), std::forward<FDoubleArg>(darg), t);
}

template <class R1, class R2, class FIntArg, class FDoubleArg, class T>
R2 value_method_switch_with_return_type(FIntArg&& iarg, FDoubleArg&& darg, const T& t) {
  return static_if<has_method_value_with_convertible_return_type<T, R1>>(
      [](FIntArg&& iarg, FDoubleArg&& darg, const auto& t) -> R2 {
        using A = remove_cvref_t<decltype(t)>;
        return static_if<std::is_same<arg_type<decltype(&A::value), 0>, int>>(
            std::forward<FIntArg>(iarg), std::forward<FDoubleArg>(darg), t);
      },
      [](FIntArg&&, FDoubleArg&&, const auto&) -> R2 {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            detail::cat(boost::core::demangled_name(BOOST_CORE_TYPEID(T)),
                        " has no value method or return type is not convertible to ",
                        boost::core::demangled_name(BOOST_CORE_TYPEID(R1)))));
#ifndef _MSC_VER // msvc warns about unreachable return
        // conjure a value out of thin air to satisfy syntactic requirement
        return *reinterpret_cast<R2*>(0);
#endif
      },
      std::forward<FIntArg>(iarg), std::forward<FDoubleArg>(darg), t);
}
} // namespace detail

namespace axis {
namespace traits {

/** Returns reference to metadata of an axis.

  If the expression x.metadata() for an axis instance `x` (maybe const) is valid, return
  the result. Otherwise, return a reference to a static instance of
  boost::histogram::axis::null_type.

  @param axis any axis instance
*/
template <class Axis>
decltype(auto) metadata(Axis&& axis) noexcept {
  return detail::static_if<
      detail::has_method_metadata<const detail::remove_cvref_t<Axis>>>(
      [](auto&& a) -> decltype(auto) { return a.metadata(); },
      [](auto &&) -> detail::copy_qualifiers<Axis, null_type> {
        static null_type m;
        return m;
      },
      std::forward<Axis>(axis));
}

/** Generates static axis option type for axis type.

  WARNING: Doxygen does not render the synopsis correctly. This is a templated using
  directive, which accepts axis type and returns boost::histogram::axis::option::bitset.

  If Axis::options() is valid and constexpr, return the corresponding option type.
  Otherwise, return boost::histogram::axis::option::growth_t, if the axis has a method
  `update`, else return boost::histogram::axis::option::none_t.

  @tparam Axis axis type
*/
template <class Axis>
using static_options = detail::mp_eval_or<
    mp11::mp_if<detail::has_method_update<detail::remove_cvref_t<Axis>>, option::growth_t,
                option::none_t>,
    detail::static_options_impl, detail::remove_cvref_t<Axis>>;

/** Returns axis options as unsigned integer.

  If axis.options() is valid, return the result. Otherwise, return
  boost::histogram::axis::traits::static_options<decltype(axis)>::value.

  @param axis any axis instance
*/
template <class Axis>
constexpr unsigned options(const Axis& axis) noexcept {
  // cannot reuse static_options here, because this should also work for axis::variant
  return detail::static_if<detail::has_method_options<Axis>>(
      [](const auto& a) { return a.options(); },
      [](const auto&) { return static_options<Axis>::value; }, axis);
}

/** Returns axis size plus any extra bins for under- and overflow.

  @param axis any axis instance
*/
template <class Axis>
constexpr index_type extent(const Axis& axis) noexcept {
  const auto opt = options(axis);
  return axis.size() + (opt & option::underflow ? 1 : 0) +
         (opt & option::overflow ? 1 : 0);
}

/** Returns axis value for index.

  If the axis has no `value` method, throw std::runtime_error. If the method exists and
  accepts a floating point index, pass the index and return the result. If the method
  exists but accepts only integer indices, cast the floating point index to int, pass this
  index and return the result.

  @param axis any axis instance
  @param index floating point axis index
*/
template <class Axis>
decltype(auto) value(const Axis& axis, real_index_type index) {
  return detail::value_method_switch(
      [index](const auto& a) { return a.value(static_cast<int>(index)); },
      [index](const auto& a) { return a.value(index); }, axis);
}

/** Returns axis value for index if it is convertible to target type or throws.

  Like boost::histogram::axis::traits::value, but converts the result into the requested
  return type. If the conversion is not possible, throws std::runtime_error.

  @tparam Result requested return type
  @tparam Axis axis type
  @param axis any axis instance
  @param index floating point axis index
*/
template <class Result, class Axis>
Result value_as(const Axis& axis, real_index_type index) {
  return detail::value_method_switch_with_return_type<Result, Result>(
      [index](const auto& a) {
        return static_cast<Result>(a.value(static_cast<int>(index)));
      },
      [index](const auto& a) { return static_cast<Result>(a.value(index)); }, axis);
}

/** Returns axis index for value.

  Throws std::invalid_argument if the value argument is not implicitly convertible.

  @param axis any axis instance
  @param value argument to be passed to `index` method
*/
template <class Axis, class U>
auto index(const Axis& axis, const U& value) {
  using V = detail::arg_type<decltype(&Axis::index)>;
  return detail::static_if<std::is_convertible<U, V>>(
      [&value](const auto& axis) {
        using A = detail::remove_cvref_t<decltype(axis)>;
        using V2 = detail::arg_type<decltype(&A::index)>;
        return axis.index(static_cast<V2>(value));
      },
      [](const Axis&) -> index_type {
        BOOST_THROW_EXCEPTION(std::invalid_argument(
            detail::cat(boost::core::demangled_name(BOOST_CORE_TYPEID(Axis)),
                        ": cannot convert argument of type ",
                        boost::core::demangled_name(BOOST_CORE_TYPEID(U)), " to ",
                        boost::core::demangled_name(BOOST_CORE_TYPEID(V)))));
#ifndef _MSC_VER // msvc warns about unreachable return
        return index_type{};
#endif
      },
      axis);
}

/// @copydoc index(const Axis&, const U& value)
template <class... Ts, class U>
auto index(const variant<Ts...>& axis, const U& value) {
  return axis.index(value);
}

/** Returns pair of axis index and shift for the value argument.

  Throws `std::invalid_argument` if the value argument is not implicitly convertible to
  the argument expected by the `index` method. If the result of
  boost::histogram::axis::traits::static_options<decltype(axis)> has the growth flag set,
  call `update` method with the argument and return the result. Otherwise, call `index`
  and return the pair of the result and a zero shift.

  @param axis any axis instance
  @param value argument to be passed to `update` or `index` method
*/
template <class Axis, class U>
std::pair<int, int> update(Axis& axis, const U& value) {
  using V = detail::arg_type<decltype(&Axis::index)>;
  return detail::static_if<std::is_convertible<U, V>>(
      [&value](auto& a) {
        using A = detail::remove_cvref_t<decltype(a)>;
        return detail::static_if_c<static_options<A>::test(option::growth)>(
            [&value](auto& a) { return a.update(value); },
            [&value](auto& a) { return std::make_pair(a.index(value), 0); }, a);
      },
      [](Axis&) -> std::pair<index_type, index_type> {
        BOOST_THROW_EXCEPTION(std::invalid_argument(
            detail::cat(boost::core::demangled_name(BOOST_CORE_TYPEID(Axis)),
                        ": cannot convert argument of type ",
                        boost::core::demangled_name(BOOST_CORE_TYPEID(U)), " to ",
                        boost::core::demangled_name(BOOST_CORE_TYPEID(V)))));
#ifndef _MSC_VER // msvc warns about unreachable return
        return std::make_pair(index_type{}, index_type{});
#endif
      },
      axis);
}

/// @copydoc update(Axis& axis, const U& value)
template <class... Ts, class U>
auto update(variant<Ts...>& axis, const U& value) {
  return axis.update(value);
}

/** Returns bin width at axis index.

  If the axis has no `value` method, throw std::runtime_error. If the method exists and
  accepts a floating point index, return the result of `axis.value(index + 1) -
  axis.value(index)`. If the method exists but accepts only integer indices, return 0.

  @param axis any axis instance
  @param index bin index
 */
template <class Axis>
decltype(auto) width(const Axis& axis, index_type index) {
  return detail::value_method_switch(
      [](const auto&) { return 0; },
      [index](const auto& a) { return a.value(index + 1) - a.value(index); }, axis);
}

/** Returns bin width at axis index.

  Like boost::histogram::axis::traits::width, but converts the result into the requested
  return type. If the conversion is not possible, throw std::runtime_error.

  @param axis any axis instance
  @param index bin index
 */
template <class Result, class Axis>
Result width_as(const Axis& axis, index_type index) {
  return detail::value_method_switch_with_return_type<Result, Result>(
      [](const auto&) { return Result{}; },
      [index](const auto& a) {
        return static_cast<Result>(a.value(index + 1) - a.value(index));
      },
      axis);
}

} // namespace traits
} // namespace axis
} // namespace histogram
} // namespace boost

#endif
