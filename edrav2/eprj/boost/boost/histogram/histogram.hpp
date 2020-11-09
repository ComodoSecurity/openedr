// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_HISTOGRAM_HPP
#define BOOST_HISTOGRAM_HISTOGRAM_HPP

#include <boost/histogram/detail/axes.hpp>
#include <boost/histogram/detail/common_type.hpp>
#include <boost/histogram/detail/linearize.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/list.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace boost {
namespace histogram {

/** Central class of the histogram library.

  Histogram uses the call operator to insert data, like the
  [Boost.Accumulators](https://www.boost.org/doc/libs/develop/doc/html/accumulators.html).

  Use factory functions (see
  [make_histogram.hpp](histogram/reference.html#header.boost.histogram.make_histogram_hpp)
  and
  [make_profile.hpp](histogram/reference.html#header.boost.histogram.make_profile_hpp)) to
  conveniently create histograms rather than calling the ctors directly.

  Use the [indexed](boost/histogram/indexed.html) range generator to iterate over filled
  histograms, which is convenient and faster than hand-written loops for multi-dimensional
  histograms.

  @tparam Axes std::tuple of axis types OR std::vector of an axis type or axis::variant
  @tparam Storage class that implements the storage interface
 */
template <class Axes, class Storage>
class histogram {
public:
  static_assert(mp11::mp_size<Axes>::value > 0, "at least one axis required");
  static_assert(std::is_same<detail::remove_cvref_t<Storage>, Storage>::value,
                "Storage may not be a reference or const or volatile");

public:
  using axes_type = Axes;
  using storage_type = Storage;
  using value_type = typename storage_type::value_type;
  // typedefs for boost::range_iterator
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;

  histogram() = default;
  histogram(const histogram& rhs) = default;
  histogram(histogram&& rhs) = default;
  histogram& operator=(const histogram& rhs) = default;
  histogram& operator=(histogram&& rhs) = default;

  template <class A, class S>
  explicit histogram(histogram<A, S>&& rhs) : storage_(std::move(rhs.storage_)) {
    detail::axes_assign(axes_, rhs.axes_);
  }

  template <class A, class S>
  explicit histogram(const histogram<A, S>& rhs) : storage_(rhs.storage_) {
    detail::axes_assign(axes_, rhs.axes_);
  }

  template <class A, class S>
  histogram& operator=(histogram<A, S>&& rhs) {
    detail::axes_assign(axes_, std::move(rhs.axes_));
    storage_ = std::move(rhs.storage_);
    return *this;
  }

  template <class A, class S>
  histogram& operator=(const histogram<A, S>& rhs) {
    detail::axes_assign(axes_, rhs.axes_);
    storage_ = rhs.storage_;
    return *this;
  }

  template <class A, class S>
  histogram(A&& a, S&& s) : axes_(std::forward<A>(a)), storage_(std::forward<S>(s)) {
    storage_.reset(detail::bincount(axes_));
  }

  template <class A, class = detail::requires_axes<A>>
  explicit histogram(A&& a) : histogram(std::forward<A>(a), storage_type()) {}

  /// Number of axes (dimensions).
  constexpr unsigned rank() const noexcept {
    return static_cast<unsigned>(detail::get_size(axes_));
  }

  /// Total number of bins (including underflow/overflow).
  std::size_t size() const noexcept { return storage_.size(); }

  /// Reset all bins to default initialized values.
  void reset() { storage_.reset(storage_.size()); }

  /// Get N-th axis using a compile-time number.
  /// This version is more efficient than the one accepting a run-time number.
  template <unsigned N = 0>
  decltype(auto) axis(std::integral_constant<unsigned, N> = {}) const {
    detail::axis_index_is_valid(axes_, N);
    return detail::axis_get<N>(axes_);
  }

  /// Get N-th axis with run-time number.
  /// Use version that accepts a compile-time number, if possible.
  decltype(auto) axis(unsigned i) const {
    detail::axis_index_is_valid(axes_, i);
    return detail::axis_get(axes_, i);
  }

  /// Apply unary functor/function to each axis.
  template <class Unary>
  auto for_each_axis(Unary&& unary) const {
    return detail::for_each_axis(axes_, std::forward<Unary>(unary));
  }

  /** Fill histogram with values, an optional weight, and/or a sample.

   Arguments are passed in order to the axis objects. Passing an argument type that is
   not convertible to the value type accepted by the axis or passing the wrong number
   of arguments causes a throw of `std::invalid_argument`.

   __Axis with multiple arguments__

   If the histogram contains an axis which accepts a `std::tuple` of arguments, the
   arguments for that axis need to passed as a `std::tuple`, for example,
   `std::make_tuple(1.2, 2.3)`. If the histogram contains only this axis and no other,
   the arguments can be passed directly.

   __Optional weight__

   An optional weight can be passed as the first or last argument
   with the [weight](boost/histogram/weight.html) helper function. Compilation fails if
   the storage elements do not support weights.

   __Samples__

   If the storage elements accept samples, pass them with the sample helper function
   in addition to the axis arguments, which can be the first or last argument. The
   [sample](boost/histogram/sample.html) helper function can pass one or more arguments to
   the storage element. If samples and weights are used together, they can be passed in
   any order at the beginning or end of the argument list.
  */
  template <class... Ts>
  auto operator()(const Ts&... ts) {
    return operator()(std::make_tuple(ts...));
  }

  /// Fill histogram with values, an optional weight, and/or a sample from a `std::tuple`.
  template <class... Ts>
  auto operator()(const std::tuple<Ts...>& t) {
    return detail::fill(storage_, axes_, t);
  }

  /// Access cell value at integral indices.
  /**
    You can pass indices as individual arguments, as a std::tuple of integers, or as an
    interable range of integers. Passing the wrong number of arguments causes a throw of
    std::invalid_argument. Passing an index which is out of bounds causes a throw of
    std::out_of_range.
  */
  template <class... Ts>
  decltype(auto) at(axis::index_type t, Ts... ts) {
    return at(std::forward_as_tuple(t, ts...));
  }

  /// Access cell value at integral indices (read-only).
  /// @copydoc at(axis::index_type t, Ts... ts)
  template <class... Ts>
  decltype(auto) at(axis::index_type t, Ts... ts) const {
    return at(std::forward_as_tuple(t, ts...));
  }

  /// Access cell value at integral indices stored in `std::tuple`.
  /// @copydoc at(axis::index_type t, Ts... ts)
  template <typename... Ts>
  decltype(auto) at(const std::tuple<Ts...>& t) {
    const auto idx = detail::at(axes_, t);
    if (!idx) BOOST_THROW_EXCEPTION(std::out_of_range("indices out of bounds"));
    return storage_[*idx];
  }

  /// Access cell value at integral indices stored in `std::tuple` (read-only).
  /// @copydoc at(axis::index_type t, Ts... ts)
  template <typename... Ts>
  decltype(auto) at(const std::tuple<Ts...>& t) const {
    const auto idx = detail::at(axes_, t);
    if (!idx) BOOST_THROW_EXCEPTION(std::out_of_range("indices out of bounds"));
    return storage_[*idx];
  }

  /// Access cell value at integral indices stored in iterable.
  /// @copydoc at(axis::index_type t, Ts... ts)
  template <class Iterable, class = detail::requires_iterable<Iterable>>
  decltype(auto) at(const Iterable& c) {
    const auto idx = detail::at(axes_, c);
    if (!idx) BOOST_THROW_EXCEPTION(std::out_of_range("indices out of bounds"));
    return storage_[*idx];
  }

  /// Access cell value at integral indices stored in iterable (read-only).
  /// @copydoc at(axis::index_type t, Ts... ts)
  template <class Iterable, class = detail::requires_iterable<Iterable>>
  decltype(auto) at(const Iterable& c) const {
    const auto idx = detail::at(axes_, c);
    if (!idx) BOOST_THROW_EXCEPTION(std::out_of_range("indices out of bounds"));
    return storage_[*idx];
  }

  /// Access value at index (number for rank = 1, else `std::tuple` or iterable).
  template <class T>
  decltype(auto) operator[](const T& t) {
    return at(t);
  }

  /// @copydoc operator[]
  template <class T>
  decltype(auto) operator[](const T& t) const {
    return at(t);
  }

  /// Equality operator, tests equality for all axes and the storage.
  template <class A, class S>
  bool operator==(const histogram<A, S>& rhs) const noexcept {
    return detail::axes_equal(axes_, rhs.axes_) && storage_ == rhs.storage_;
  }

  /// Negation of the equality operator.
  template <class A, class S>
  bool operator!=(const histogram<A, S>& rhs) const noexcept {
    return !operator==(rhs);
  }

  /// Add values of another histogram.
  template <class A, class S>
  histogram& operator+=(const histogram<A, S>& rhs) {
    static_assert(detail::has_operator_radd<value_type,
                                            typename histogram<A, S>::value_type>::value,
                  "cell value does not support adding value of right-hand-side");
    if (!detail::axes_equal(axes_, rhs.axes_))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = rhs.storage_.begin();
    std::for_each(storage_.begin(), storage_.end(), [&rit](auto&& x) { x += *rit++; });
    return *this;
  }

  /// Subtract values of another histogram.
  template <class A, class S>
  histogram& operator-=(const histogram<A, S>& rhs) {
    static_assert(detail::has_operator_rsub<value_type,
                                            typename histogram<A, S>::value_type>::value,
                  "cell value does not support subtracting value of right-hand-side");
    if (!detail::axes_equal(axes_, rhs.axes_))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = rhs.storage_.begin();
    std::for_each(storage_.begin(), storage_.end(), [&rit](auto&& x) { x -= *rit++; });
    return *this;
  }

  /// Multiply by values of another histogram.
  template <class A, class S>
  histogram& operator*=(const histogram<A, S>& rhs) {
    static_assert(detail::has_operator_rmul<value_type,
                                            typename histogram<A, S>::value_type>::value,
                  "cell value does not support multiplying by value of right-hand-side");
    if (!detail::axes_equal(axes_, rhs.axes_))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = rhs.storage_.begin();
    std::for_each(storage_.begin(), storage_.end(), [&rit](auto&& x) { x *= *rit++; });
    return *this;
  }

  /// Divide by values of another histogram.
  template <class A, class S>
  histogram& operator/=(const histogram<A, S>& rhs) {
    static_assert(detail::has_operator_rdiv<value_type,
                                            typename histogram<A, S>::value_type>::value,
                  "cell value does not support dividing by value of right-hand-side");
    if (!detail::axes_equal(axes_, rhs.axes_))
      BOOST_THROW_EXCEPTION(std::invalid_argument("axes of histograms differ"));
    auto rit = rhs.storage_.begin();
    std::for_each(storage_.begin(), storage_.end(), [&rit](auto&& x) { x /= *rit++; });
    return *this;
  }

  /// Multiply all values with a scalar.
  template <class V = value_type,
            class = std::enable_if_t<detail::has_operator_rmul<V, double>::value>>
  histogram& operator*=(const double x) {
    // use special implementation of scaling if available
    detail::static_if<detail::has_operator_rmul<storage_type, double>>(
        [](storage_type& s, auto x) { s *= x; },
        [](storage_type& s, auto x) {
          for (auto&& si : s) si *= x;
        },
        storage_, x);
    return *this;
  }

  /// Divide all values by a scalar.
  template <class V = value_type,
            class = std::enable_if_t<detail::has_operator_rmul<V, double>::value>>
  histogram& operator/=(const double x) {
    return operator*=(1.0 / x);
  }

  /// Return value iterator to the beginning of the histogram.
  iterator begin() noexcept { return storage_.begin(); }

  /// Return value iterator to the end in the histogram.
  iterator end() noexcept { return storage_.end(); }

  /// Return value iterator to the beginning of the histogram (read-only).
  const_iterator begin() const noexcept { return storage_.begin(); }

  /// Return value iterator to the end in the histogram (read-only).
  const_iterator end() const noexcept { return storage_.end(); }

  /// Return value iterator to the beginning of the histogram (read-only).
  const_iterator cbegin() const noexcept { return storage_.begin(); }

  /// Return value iterator to the end in the histogram (read-only).
  const_iterator cend() const noexcept { return storage_.end(); }

private:
  axes_type axes_;
  storage_type storage_;

  template <class A, class S>
  friend class histogram;
  friend struct unsafe_access;
};

template <class A1, class S1, class A2, class S2>
auto operator+(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r += b;
}

template <class A1, class S1, class A2, class S2>
auto operator*(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r *= b;
}

template <class A1, class S1, class A2, class S2>
auto operator-(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r -= b;
}

template <class A1, class S1, class A2, class S2>
auto operator/(const histogram<A1, S1>& a, const histogram<A2, S2>& b) {
  auto r = histogram<detail::common_axes<A1, A2>, detail::common_storage<S1, S2>>(a);
  return r /= b;
}

template <class A, class S>
auto operator*(const histogram<A, S>& h, double x) {
  auto r = histogram<A, detail::common_storage<S, dense_storage<double>>>(h);
  return r *= x;
}

template <class A, class S>
auto operator*(double x, const histogram<A, S>& h) {
  return h * x;
}

template <class A, class S>
auto operator/(const histogram<A, S>& h, double x) {
  return h * (1.0 / x);
}

#if __cpp_deduction_guides >= 201606

template <class Axes>
histogram(Axes&& axes)->histogram<detail::remove_cvref_t<Axes>, default_storage>;

template <class Axes, class Storage>
histogram(Axes&& axes, Storage&& storage)
    ->histogram<detail::remove_cvref_t<Axes>, detail::remove_cvref_t<Storage>>;

#endif

/// Helper function to mark argument as weight.
/// @param t argument to be forward to the histogram.
template <typename T>
auto weight(T&& t) noexcept {
  return weight_type<T>{std::forward<T>(t)};
}

/// Helper function to mark arguments as sample.
/// @param ts arguments to be forwarded to the accumulator.
template <typename... Ts>
auto sample(Ts&&... ts) noexcept {
  return sample_type<std::tuple<Ts...>>{std::forward_as_tuple(std::forward<Ts>(ts)...)};
}

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
template <class T>
struct weight_type {
  T value;
};

template <class T>
struct sample_type {
  T value;
};
#endif

} // namespace histogram
} // namespace boost

#endif
