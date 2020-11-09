// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_META_HPP
#define BOOST_HISTOGRAM_DETAIL_META_HPP

/* Most of the histogram code is generic and works for any number of axes. Buffers with a
 * fixed maximum capacity are used in some places, which have a size equal to the rank of
 * a histogram. The buffers are statically allocated to improve performance, which means
 * that they need a preset maximum capacity. 32 seems like a safe upper limit for the rank
 * (you can nevertheless increase it here if necessary): the simplest non-trivial axis has
 * 2 bins; even if counters are used which need only a byte of storage per bin, this still
 * corresponds to 4 GB of storage.
 */
#ifndef BOOST_HISTOGRAM_DETAIL_AXES_LIMIT
#define BOOST_HISTOGRAM_DETAIL_AXES_LIMIT 32
#endif

#include <boost/config/workaround.hpp>
#if BOOST_WORKAROUND(BOOST_GCC, >= 60000)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>
#if BOOST_WORKAROUND(BOOST_GCC, >= 60000)
#pragma GCC diagnostic pop
#endif
#include <array>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/integer_sequence.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>
#include <functional>
#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T, class U>
using convert_integer = mp11::mp_if<std::is_integral<remove_cvref_t<T>>, U, T>;

// to be replaced by official version from mp11
template <class E, template <class...> class F, class... Ts>
using mp_eval_or = mp11::mp_eval_if_c<!(mp11::mp_valid<F, Ts...>::value), E, F, Ts...>;

template <class T1, class T2>
using copy_qualifiers = mp11::mp_if<
    std::is_rvalue_reference<T1>, T2&&,
    mp11::mp_if<std::is_lvalue_reference<T1>,
                mp11::mp_if<std::is_const<typename std::remove_reference<T1>::type>,
                            const T2&, T2&>,
                mp11::mp_if<std::is_const<T1>, const T2, T2>>>;

template <class L>
using mp_last = mp11::mp_at_c<L, (mp11::mp_size<L>::value - 1)>;

template <class T, class Args = boost::callable_traits::args_t<T>>
using args_type =
    mp11::mp_if<std::is_member_function_pointer<T>, mp11::mp_pop_front<Args>, Args>;

template <class T, std::size_t N = 0>
using arg_type = typename mp11::mp_at_c<args_type<T>, N>;

template <class T>
using return_type = typename boost::callable_traits::return_type<T>::type;

template <class F, class V,
          class T = copy_qualifiers<V, mp11::mp_first<remove_cvref_t<V>>>>
using visitor_return_type = decltype(std::declval<F>()(std::declval<T>()));

template <bool B, typename T, typename F, typename... Ts>
constexpr decltype(auto) static_if_c(T&& t, F&& f, Ts&&... ts) {
  return std::get<(B ? 0 : 1)>(std::forward_as_tuple(
      std::forward<T>(t), std::forward<F>(f)))(std::forward<Ts>(ts)...);
}

template <typename B, typename... Ts>
constexpr decltype(auto) static_if(Ts&&... ts) {
  return static_if_c<B::value>(std::forward<Ts>(ts)...);
}

template <typename T>
constexpr T lowest() {
  return std::numeric_limits<T>::lowest();
}

template <>
constexpr double lowest() {
  return -std::numeric_limits<double>::infinity();
}

template <>
constexpr float lowest() {
  return -std::numeric_limits<float>::infinity();
}

template <typename T>
constexpr T highest() {
  return std::numeric_limits<T>::max();
}

template <>
constexpr double highest() {
  return std::numeric_limits<double>::infinity();
}

template <>
constexpr float highest() {
  return std::numeric_limits<float>::infinity();
}

template <std::size_t I, class T, std::size_t... N>
decltype(auto) tuple_slice_impl(T&& t, mp11::index_sequence<N...>) {
  return std::forward_as_tuple(std::get<(N + I)>(std::forward<T>(t))...);
}

template <std::size_t I, std::size_t N, class T>
decltype(auto) tuple_slice(T&& t) {
  static_assert(I + N <= mp11::mp_size<remove_cvref_t<T>>::value,
                "I and N must describe a slice");
  return tuple_slice_impl<I>(std::forward<T>(t), mp11::make_index_sequence<N>{});
}

#define BOOST_HISTOGRAM_DETECT(name, cond)   \
  template <class T, class = decltype(cond)> \
  struct name##_impl {};                     \
  template <class T>                         \
  using name = typename mp11::mp_valid<name##_impl, T>

#define BOOST_HISTOGRAM_DETECT_BINARY(name, cond)     \
  template <class T, class U, class = decltype(cond)> \
  struct name##_impl {};                              \
  template <class T, class U = T>                     \
  using name = typename mp11::mp_valid<name##_impl, T, U>

BOOST_HISTOGRAM_DETECT(has_method_metadata, (std::declval<T&>().metadata()));

// resize has two overloads, trying to get pmf in this case always fails
BOOST_HISTOGRAM_DETECT(has_method_resize, (std::declval<T&>().resize(0)));

BOOST_HISTOGRAM_DETECT(has_method_size, &T::size);

BOOST_HISTOGRAM_DETECT(has_method_clear, &T::clear);

BOOST_HISTOGRAM_DETECT(has_method_lower, &T::lower);

BOOST_HISTOGRAM_DETECT(has_method_value, &T::value);

BOOST_HISTOGRAM_DETECT(has_method_update, (&T::update));

BOOST_HISTOGRAM_DETECT(has_method_reset, (std::declval<T>().reset(0)));

template <typename T>
using get_value_method_return_type_impl = decltype(std::declval<T&>().value(0));

template <typename T, typename R>
using has_method_value_with_convertible_return_type =
    typename std::is_convertible<mp_eval_or<void, get_value_method_return_type_impl, T>,
                                 R>::type;

BOOST_HISTOGRAM_DETECT(has_method_options, (&T::options));

BOOST_HISTOGRAM_DETECT(has_allocator, &T::get_allocator);

BOOST_HISTOGRAM_DETECT(is_indexable, (std::declval<T&>()[0]));

BOOST_HISTOGRAM_DETECT(is_transform, (&T::forward, &T::inverse));

BOOST_HISTOGRAM_DETECT(is_indexable_container,
                       (std::declval<T>()[0], &T::size, std::begin(std::declval<T>()),
                        std::end(std::declval<T>())));

BOOST_HISTOGRAM_DETECT(is_vector_like,
                       (std::declval<T>()[0], &T::size, std::declval<T>().resize(0),
                        std::begin(std::declval<T>()), std::end(std::declval<T>())));

BOOST_HISTOGRAM_DETECT(is_array_like,
                       (std::declval<T>()[0], &T::size, std::tuple_size<T>::value,
                        std::begin(std::declval<T>()), std::end(std::declval<T>())));

BOOST_HISTOGRAM_DETECT(is_map_like,
                       (std::declval<typename T::key_type>(),
                        std::declval<typename T::mapped_type>(),
                        std::begin(std::declval<T>()), std::end(std::declval<T>())));

// ok: is_axis is false for axis::variant, operator() is templated
BOOST_HISTOGRAM_DETECT(is_axis, (&T::size, &T::index));

BOOST_HISTOGRAM_DETECT(is_iterable,
                       (std::begin(std::declval<T&>()), std::end(std::declval<T&>())));

BOOST_HISTOGRAM_DETECT(is_iterator,
                       (typename std::iterator_traits<T>::iterator_category()));

BOOST_HISTOGRAM_DETECT(is_streamable,
                       (std::declval<std::ostream&>() << std::declval<T&>()));

BOOST_HISTOGRAM_DETECT(is_incrementable, (++std::declval<T&>()));

BOOST_HISTOGRAM_DETECT(has_operator_preincrement, (++std::declval<T&>()));

BOOST_HISTOGRAM_DETECT_BINARY(has_operator_equal,
                              (std::declval<const T&>() == std::declval<const U&>()));

BOOST_HISTOGRAM_DETECT_BINARY(has_operator_radd,
                              (std::declval<T&>() += std::declval<U&>()));

BOOST_HISTOGRAM_DETECT_BINARY(has_operator_rsub,
                              (std::declval<T&>() -= std::declval<U&>()));

BOOST_HISTOGRAM_DETECT_BINARY(has_operator_rmul,
                              (std::declval<T&>() *= std::declval<U&>()));

BOOST_HISTOGRAM_DETECT_BINARY(has_operator_rdiv,
                              (std::declval<T&>() /= std::declval<U&>()));

template <typename T>
using is_storage =
    mp11::mp_bool<(is_indexable_container<T>::value && has_method_reset<T>::value)>;

template <typename T>
struct is_tuple_impl : std::false_type {};

template <typename... Ts>
struct is_tuple_impl<std::tuple<Ts...>> : std::true_type {};

template <typename T>
using is_tuple = typename is_tuple_impl<T>::type;

template <typename T>
struct is_axis_variant_impl : std::false_type {};

template <typename... Ts>
struct is_axis_variant_impl<axis::variant<Ts...>> : std::true_type {};

template <typename T>
using is_axis_variant = typename is_axis_variant_impl<T>::type;

template <typename T>
using is_any_axis = mp11::mp_or<is_axis<T>, is_axis_variant<T>>;

template <typename T>
using is_sequence_of_axis = mp11::mp_and<is_iterable<T>, is_axis<mp11::mp_first<T>>>;

template <typename T>
using is_sequence_of_axis_variant =
    mp11::mp_and<is_iterable<T>, is_axis_variant<mp11::mp_first<T>>>;

template <typename T>
using is_sequence_of_any_axis =
    mp11::mp_and<is_iterable<T>, is_any_axis<mp11::mp_first<T>>>;

template <typename T>
struct is_weight_impl : std::false_type {};

template <typename T>
struct is_weight_impl<weight_type<T>> : std::true_type {};

template <typename T>
using is_weight = is_weight_impl<remove_cvref_t<T>>;

template <typename T>
struct is_sample_impl : std::false_type {};

template <typename T>
struct is_sample_impl<sample_type<T>> : std::true_type {};

template <typename T>
using is_sample = is_sample_impl<remove_cvref_t<T>>;

// poor-mans concept checks
template <class T, class = std::enable_if_t<is_iterator<remove_cvref_t<T>>::value>>
struct requires_iterator {};

template <class T, class = std::enable_if_t<is_iterable<remove_cvref_t<T>>::value>>
struct requires_iterable {};

template <class T, class = std::enable_if_t<is_axis<remove_cvref_t<T>>::value>>
struct requires_axis {};

template <class T, class = std::enable_if_t<is_any_axis<remove_cvref_t<T>>::value>>
struct requires_any_axis {};

template <class T,
          class = std::enable_if_t<is_sequence_of_axis<remove_cvref_t<T>>::value>>
struct requires_sequence_of_axis {};

template <class T,
          class = std::enable_if_t<is_sequence_of_axis_variant<remove_cvref_t<T>>::value>>
struct requires_sequence_of_axis_variant {};

template <class T,
          class = std::enable_if_t<is_sequence_of_any_axis<remove_cvref_t<T>>::value>>
struct requires_sequence_of_any_axis {};

template <class T,
          class = std::enable_if_t<is_any_axis<mp11::mp_first<remove_cvref_t<T>>>::value>>
struct requires_axes {};

template <class T, class U, class = std::enable_if_t<std::is_convertible<T, U>::value>>
struct requires_convertible {};

template <class T>
auto make_default(const T& t) {
  return static_if<has_allocator<T>>([](const auto& t) { return T(t.get_allocator()); },
                                     [](const auto&) { return T(); }, t);
}

template <class T>
using tuple_size_t = typename std::tuple_size<T>::type;

template <class T>
constexpr std::size_t get_size_impl(std::true_type, const T&) noexcept {
  return std::tuple_size<T>::value;
}

template <class T>
std::size_t get_size_impl(std::false_type, const T& t) noexcept {
  using std::begin;
  using std::end;
  return static_cast<std::size_t>(std::distance(begin(t), end(t)));
}

template <class T>
std::size_t get_size(const T& t) noexcept {
  return get_size_impl(mp11::mp_valid<tuple_size_t, T>(), t);
}

template <class T>
using buffer_size =
    mp_eval_or<std::integral_constant<std::size_t, BOOST_HISTOGRAM_DETAIL_AXES_LIMIT>,
               tuple_size_t, T>;

template <class T, std::size_t N>
class sub_array : public std::array<T, N> {
public:
  explicit sub_array(std::size_t s) : size_(s) {}
  sub_array(std::size_t s, T value) : size_(s) { std::array<T, N>::fill(value); }

  // need to override both versions of std::array
  auto end() noexcept { return std::array<T, N>::begin() + size_; }
  auto end() const noexcept { return std::array<T, N>::begin() + size_; }

  auto size() const noexcept { return size_; }

private:
  std::size_t size_ = N;
};

template <class U, class T>
using stack_buffer = sub_array<U, buffer_size<T>::value>;

template <class U, class T, class... Ts>
auto make_stack_buffer(const T& t, Ts&&... ts) {
  return stack_buffer<U, T>(get_size(t), std::forward<Ts>(ts)...);
}

template <class T>
constexpr bool relaxed_equal(const T& a, const T& b) noexcept {
  return static_if<has_operator_equal<T>>(
      [](const auto& a, const auto& b) { return a == b; },
      [](const auto&, const auto&) { return true; }, a, b);
}

template <class T>
using get_scale_type_helper = typename T::value_type;

template <class T>
using get_scale_type = mp_eval_or<T, detail::get_scale_type_helper, T>;

struct one_unit {};

template <class T>
T operator*(T&& t, const one_unit&) {
  return std::forward<T>(t);
}

template <class T>
T operator/(T&& t, const one_unit&) {
  return std::forward<T>(t);
}

template <class T>
using get_unit_type_helper = typename T::unit_type;

template <class T>
using get_unit_type = mp_eval_or<one_unit, detail::get_unit_type_helper, T>;

template <class T, class R = get_scale_type<T>>
R get_scale(const T& t) {
  return t / get_unit_type<T>();
}

template <class T, class Default>
using replace_default = mp11::mp_if<std::is_same<T, use_default>, Default, T>;

template <class T, class U>
using is_convertible_helper =
    mp11::mp_apply<mp11::mp_all, mp11::mp_transform<std::is_convertible, T, U>>;

template <class T, class U>
using is_convertible = mp_eval_or<std::false_type, is_convertible_helper, T, U>;

template <class T>
auto make_unsigned_impl(std::true_type, const T t) noexcept {
  return static_cast<typename std::make_unsigned<T>::type>(t);
}

template <class T>
auto make_unsigned_impl(std::false_type, const T t) noexcept {
  return t;
}

template <class T>
auto make_unsigned(const T t) noexcept {
  return make_unsigned_impl(std::is_integral<T>{}, t);
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
