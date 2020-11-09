// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_STORAGE_ADAPTOR_HPP
#define BOOST_HISTOGRAM_STORAGE_ADAPTOR_HPP

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/histogram/detail/cat.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/mp11/utility.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
struct vector_impl : T {
  vector_impl() = default;

  explicit vector_impl(const T& t) : T(t) {}
  explicit vector_impl(typename T::allocator_type a) : T(a) {}

  template <class U, class = requires_iterable<U>>
  explicit vector_impl(const U& u) {
    T::reserve(u.size());
    for (auto&& x : u) T::emplace_back(x);
  }

  template <class U, class = requires_iterable<U>>
  vector_impl& operator=(const U& u) {
    T::resize(u.size());
    auto it = T::begin();
    for (auto&& x : u) *it++ = x;
    return *this;
  }

  void reset(std::size_t n) {
    using value_type = typename T::value_type;
    const auto old_size = T::size();
    T::resize(n, value_type());
    std::fill_n(T::begin(), std::min(n, old_size), value_type());
  }
};

template <class T>
struct array_impl : T {
  array_impl() = default;

  explicit array_impl(const T& t) : T(t) {}
  template <class U, class = requires_iterable<U>>
  explicit array_impl(const U& u) : size_(u.size()) {
    std::size_t i = 0;
    for (auto&& x : u) T::operator[](i++) = x;
  }

  template <class U, class = requires_iterable<U>>
  array_impl& operator=(const U& u) {
    size_ = u.size();
    if (size_ > T::max_size()) // for std::array
      BOOST_THROW_EXCEPTION(std::length_error(
          detail::cat("size ", size_, " exceeds maximum capacity ", T::max_size())));
    auto it = T::begin();
    for (auto&& x : u) *it++ = x;
    return *this;
  }

  void reset(std::size_t n) {
    using value_type = typename T::value_type;
    if (n > T::max_size()) // for std::array
      BOOST_THROW_EXCEPTION(std::length_error(
          detail::cat("size ", n, " exceeds maximum capacity ", T::max_size())));
    std::fill_n(T::begin(), n, value_type());
    size_ = n;
  }

  typename T::iterator end() noexcept { return T::begin() + size_; }
  typename T::const_iterator end() const noexcept { return T::begin() + size_; }

  std::size_t size() const noexcept { return size_; }

  std::size_t size_ = 0;
};

template <class T>
struct map_impl : T {
  static_assert(std::is_same<typename T::key_type, std::size_t>::value,
                "requires std::size_t as key_type");

  using value_type = typename T::mapped_type;
  using const_reference = const value_type&;

  struct reference {
    reference(map_impl* m, std::size_t i) noexcept : map(m), idx(i) {}
    reference(const reference&) noexcept = default;
    reference operator=(reference o) {
      if (this != &o) operator=(static_cast<const_reference>(o));
      return *this;
    }

    operator const_reference() const noexcept {
      return static_cast<const map_impl*>(map)->operator[](idx);
    }

    template <class U, class = requires_convertible<U, value_type>>
    reference& operator=(const U& u) {
      auto it = map->find(idx);
      if (u == value_type()) {
        if (it != static_cast<T*>(map)->end()) map->erase(it);
      } else {
        if (it != static_cast<T*>(map)->end())
          it->second = u;
        else {
          map->emplace(idx, u);
        }
      }
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_radd<V, U>::value>>
    reference operator+=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end())
        it->second += u;
      else
        map->emplace(idx, u);
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_rsub<V, U>::value>>
    reference operator-=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end())
        it->second -= u;
      else
        map->emplace(idx, -u);
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_rmul<V, U>::value>>
    reference operator*=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end()) it->second *= u;
      return *this;
    }

    template <class U, class V = value_type,
              class = std::enable_if_t<has_operator_rdiv<V, U>::value>>
    reference operator/=(const U& u) {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end())
        it->second /= u;
      else if (!(value_type{} / u == value_type{}))
        map->emplace(idx, value_type{} / u);
      return *this;
    }

    template <class V = value_type,
              class = std::enable_if_t<has_operator_preincrement<V>::value>>
    reference operator++() {
      auto it = map->find(idx);
      if (it != static_cast<T*>(map)->end())
        ++it->second;
      else
        map->emplace(idx, 1);
      return *this;
    }

    template <class V = value_type,
              class = std::enable_if_t<has_operator_preincrement<V>::value>>
    value_type operator++(int) {
      const value_type tmp = operator const_reference();
      operator++();
      return tmp;
    }

    template <class... Ts>
    decltype(auto) operator()(Ts&&... args) {
      return map->operator[](idx)(std::forward<Ts>(args)...);
    }

    map_impl* map;
    std::size_t idx;
  };

  template <class Value, class Reference, class MapPtr>
  struct iterator_t
      : boost::iterator_adaptor<iterator_t<Value, Reference, MapPtr>, std::size_t, Value,
                                std::random_access_iterator_tag, Reference,
                                std::ptrdiff_t> {
    iterator_t() = default;
    template <class V, class R, class M, class = requires_convertible<M, MapPtr>>
    iterator_t(const iterator_t<V, R, M>& it) noexcept : iterator_t(it.map_, it.base()) {}
    iterator_t(MapPtr m, std::size_t i) noexcept
        : iterator_t::iterator_adaptor_(i), map_(m) {}
    template <class V, class R, class M>
    bool equal(const iterator_t<V, R, M>& rhs) const noexcept {
      return map_ == rhs.map_ && iterator_t::base() == rhs.base();
    }
    decltype(auto) dereference() const { return (*map_)[iterator_t::base()]; }
    MapPtr map_ = nullptr;
  };

  using iterator = iterator_t<value_type, reference, map_impl*>;
  using const_iterator = iterator_t<const value_type, const_reference, const map_impl*>;

  map_impl() = default;

  explicit map_impl(const T& t) : T(t) {}
  explicit map_impl(typename T::allocator_type a) : T(a) {}

  template <class U, class = requires_iterable<U>>
  explicit map_impl(const U& u) : size_(u.size()) {
    using std::begin;
    using std::end;
    std::copy(begin(u), end(u), this->begin());
  }

  template <class U, class = requires_iterable<U>>
  map_impl& operator=(const U& u) {
    if (u.size() < size_)
      reset(u.size());
    else
      size_ = u.size();
    using std::begin;
    using std::end;
    std::copy(begin(u), end(u), this->begin());
    return *this;
  }

  void reset(std::size_t n) {
    T::clear();
    size_ = n;
  }

  reference operator[](std::size_t i) noexcept { return {this, i}; }
  const_reference operator[](std::size_t i) const noexcept {
    auto it = T::find(i);
    static const value_type null = value_type{};
    if (it == T::end()) return null;
    return it->second;
  }

  iterator begin() noexcept { return {this, 0}; }
  iterator end() noexcept { return {this, size_}; }

  const_iterator begin() const noexcept { return {this, 0}; }
  const_iterator end() const noexcept { return {this, size_}; }

  std::size_t size() const noexcept { return size_; }

  std::size_t size_ = 0;
};

template <typename T>
struct ERROR_type_passed_to_storage_adaptor_not_recognized;

template <typename T>
using storage_adaptor_impl = mp11::mp_if<
    is_vector_like<T>, vector_impl<T>,
    mp11::mp_if<is_array_like<T>, array_impl<T>,
                mp11::mp_if<is_map_like<T>, map_impl<T>,
                            ERROR_type_passed_to_storage_adaptor_not_recognized<T>>>>;

} // namespace detail

/// Turns any vector-like array-like, and map-like container into a storage type.
template <typename T>
class storage_adaptor : public detail::storage_adaptor_impl<T> {
  using base_type = detail::storage_adaptor_impl<T>;

public:
  using base_type::base_type;
  using base_type::operator=;

  template <class U, class = detail::requires_iterable<U>>
  bool operator==(const U& u) const {
    using std::begin;
    using std::end;
    return std::equal(this->begin(), this->end(), begin(u), end(u));
  }
};

} // namespace histogram
} // namespace boost

#endif
