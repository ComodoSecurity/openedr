// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_VARIANT_HPP
#define BOOST_HISTOGRAM_AXIS_VARIANT_HPP

#include <boost/core/typeinfo.hpp>
#include <boost/histogram/axis/iterator.hpp>
#include <boost/histogram/axis/polymorphic_bin.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/detail/cat.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/bind.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/list.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace boost {
namespace histogram {
namespace axis {

template <class T, class... Us>
T* get_if(variant<Us...>* v);

template <class T, class... Us>
const T* get_if(const variant<Us...>* v);

/// Polymorphic axis type
template <class... Ts>
class variant : public iterator_mixin<variant<Ts...>> {
  using impl_type = boost::variant<Ts...>;
  using raw_types = mp11::mp_transform<detail::remove_cvref_t, impl_type>;

  template <class T>
  using is_bounded_type = mp11::mp_contains<raw_types, detail::remove_cvref_t<T>>;

  template <typename T>
  using requires_bounded_type = std::enable_if_t<is_bounded_type<T>::value>;

  // maybe metadata_type or const metadata_type, if bounded type is const
  using metadata_type = std::remove_reference_t<decltype(
      traits::metadata(std::declval<mp11::mp_first<impl_type>>()))>;

public:
  // cannot import ctors with using directive, it breaks gcc and msvc
  variant() = default;
  variant(const variant&) = default;
  variant& operator=(const variant&) = default;
  variant(variant&&) = default;
  variant& operator=(variant&&) = default;

  template <typename T, typename = requires_bounded_type<T>>
  variant(T&& t) : impl(std::forward<T>(t)) {}

  template <typename T, typename = requires_bounded_type<T>>
  variant& operator=(T&& t) {
    impl = std::forward<T>(t);
    return *this;
  }

  template <class... Us>
  variant(const variant<Us...>& u) {
    this->operator=(u);
  }

  template <class... Us>
  variant& operator=(const variant<Us...>& u) {
    visit(
        [this](const auto& u) {
          using U = detail::remove_cvref_t<decltype(u)>;
          detail::static_if<is_bounded_type<U>>(
              [this](const auto& u) { this->operator=(u); },
              [](const auto&) {
                BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
                    boost::core::demangled_name(BOOST_CORE_TYPEID(U)),
                    " is not convertible to a bounded type of ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(variant)))));
              },
              u);
        },
        u);
    return *this;
  }

  /// Return size of axis.
  index_type size() const {
    return visit([](const auto& x) { return x.size(); }, *this);
  }

  /// Return options of axis or option::none_t if axis has no options.
  unsigned options() const {
    return visit([](const auto& x) { return axis::traits::options(x); }, *this);
  }

  /// Return reference to const metadata or instance of null_type if axis has no
  /// metadata.
  const metadata_type& metadata() const {
    return visit(
        [](const auto& a) -> const metadata_type& {
          using M = decltype(traits::metadata(a));
          return detail::static_if<std::is_same<M, const metadata_type&>>(
              [](const auto& a) -> const metadata_type& { return traits::metadata(a); },
              [](const auto&) -> const metadata_type& {
                BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
                    "cannot return metadata of type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(M)),
                    " through axis::variant interface which uses type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(metadata_type)),
                    "; use boost::histogram::axis::get to obtain a reference "
                    "of this axis type")));
              },
              a);
        },
        *this);
  }

  /// Return reference to metadata or instance of null_type if axis has no
  /// metadata.
  metadata_type& metadata() {
    return visit(
        [](auto&& a) -> metadata_type& {
          using M = decltype(traits::metadata(a));
          return detail::static_if<std::is_same<M, metadata_type&>>(
              [](auto& a) -> metadata_type& { return traits::metadata(a); },
              [](auto&) -> metadata_type& {
                BOOST_THROW_EXCEPTION(std::runtime_error(detail::cat(
                    "cannot return metadata of type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(M)),
                    " through axis::variant interface which uses type ",
                    boost::core::demangled_name(BOOST_CORE_TYPEID(metadata_type)),
                    "; use boost::histogram::axis::get to obtain a reference "
                    "of this axis type")));
              },
              a);
        },
        *this);
  }

  /// Return index for value argument.
  /// Throws std::invalid_argument if axis has incompatible call signature.
  template <class U>
  index_type index(const U& u) const {
    return visit([&u](const auto& a) { return traits::index(a, u); }, *this);
  }

  /// Return value for index argument.
  /// Only works for axes with value method that returns something convertible
  /// to double and will throw a runtime_error otherwise, see
  /// axis::traits::value().
  double value(real_index_type idx) const {
    return visit([idx](const auto& a) { return traits::value_as<double>(a, idx); },
                 *this);
  }

  /// Return bin for index argument.
  /// Only works for axes with value method that returns something convertible
  /// to double and will throw a runtime_error otherwise, see
  /// axis::traits::value().
  auto bin(index_type idx) const {
    return visit(
        [idx](const auto& a) {
          return detail::value_method_switch_with_return_type<double,
                                                              polymorphic_bin<double>>(
              [idx](const auto& a) { // axis is discrete
                const auto x = a.value(idx);
                return polymorphic_bin<double>(x, x);
              },
              [idx](const auto& a) { // axis is continuous
                return polymorphic_bin<double>(a.value(idx), a.value(idx + 1));
              },
              a);
        },
        *this);
  }

  template <class... Us>
  bool operator==(const variant<Us...>& u) const {
    return visit([&u](const auto& x) { return u == x; }, *this);
  }

  template <class T>
  bool operator==(const T& t) const {
    // boost::variant::operator==(T) implemented only to fail, cannot use it
    auto tp = get_if<T>(this);
    return tp && detail::relaxed_equal(*tp, t);
  }

  template <class T>
  bool operator!=(const T& t) const {
    return !operator==(t);
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned);

  template <class Visitor, class Variant>
  friend auto visit(Visitor&&, Variant &&)
      -> detail::visitor_return_type<Visitor, Variant>;

  template <class T, class... Us>
  friend T& get(variant<Us...>& v);

  template <class T, class... Us>
  friend const T& get(const variant<Us...>& v);

  template <class T, class... Us>
  friend T&& get(variant<Us...>&& v);

  template <class T, class... Us>
  friend T* get_if(variant<Us...>* v);

  template <class T, class... Us>
  friend const T* get_if(const variant<Us...>* v);

private:
  boost::variant<Ts...> impl;
};

/// Apply visitor to variant.
template <class Visitor, class Variant>
auto visit(Visitor&& vis, Variant&& var)
    -> detail::visitor_return_type<Visitor, Variant> {
  return boost::apply_visitor(std::forward<Visitor>(vis), var.impl);
}

/// Return lvalue reference to T, throws unspecified exception if type does not
/// match.
template <class T, class... Us>
T& get(variant<Us...>& v) {
  return boost::get<T>(v.impl);
}

/// Return rvalue reference to T, throws unspecified exception if type does not
/// match.
template <class T, class... Us>
T&& get(variant<Us...>&& v) {
  return boost::get<T>(std::move(v.impl));
}

/// Return const reference to T, throws unspecified exception if type does not
/// match.
template <class T, class... Us>
const T& get(const variant<Us...>& v) {
  return boost::get<T>(v.impl);
}

/// Returns pointer to T in variant or null pointer if type does not match.
template <class T, class... Us>
T* get_if(variant<Us...>* v) {
  return boost::relaxed_get<T>(&(v->impl));
}

/// Returns pointer to const T in variant or null pointer if type does not
/// match.
template <class T, class... Us>
const T* get_if(const variant<Us...>* v) {
  return boost::relaxed_get<T>(&(v->impl));
}

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
// pass-through version of get for generic programming
template <class T, class U>
decltype(auto) get(U&& u) {
  return static_cast<detail::copy_qualifiers<U, T>>(u);
}

// pass-through version of get_if for generic programming
template <class T, class U>
T* get_if(U* u) {
  return std::is_same<T, detail::remove_cvref_t<U>>::value ? reinterpret_cast<T*>(u)
                                                           : nullptr;
}

// pass-through version of get_if for generic programming
template <class T, class U>
const T* get_if(const U* u) {
  return std::is_same<T, detail::remove_cvref_t<U>>::value ? reinterpret_cast<const T*>(u)
                                                           : nullptr;
}
#endif

} // namespace axis
} // namespace histogram
} // namespace boost

#endif
