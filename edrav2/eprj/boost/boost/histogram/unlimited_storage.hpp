// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_UNLIMTED_STORAGE_HPP
#define BOOST_HISTOGRAM_UNLIMTED_STORAGE_HPP

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/config/workaround.hpp>
#include <boost/cstdint.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <type_traits>

namespace boost {
namespace histogram {

namespace detail {

// version of std::equal_to<> which handles comparison of signed and unsigned
struct equal {
  template <class T, class U>
  bool operator()(const T& t, const U& u) const noexcept {
    return impl(std::is_signed<T>{}, std::is_signed<U>{}, t, u);
  }

  template <class T, class U>
  bool impl(std::false_type, std::false_type, const T& t, const U& u) const noexcept {
    return t == u;
  }

  template <class T, class U>
  bool impl(std::false_type, std::true_type, const T& t, const U& u) const noexcept {
    return u >= 0 && t == make_unsigned(u);
  }

  template <class T, class U>
  bool impl(std::true_type, std::false_type, const T& t, const U& u) const noexcept {
    return t >= 0 && make_unsigned(t) == u;
  }

  template <class T, class U>
  bool impl(std::true_type, std::true_type, const T& t, const U& u) const noexcept {
    return t == u;
  }
};

// version of std::less<> which handles comparison of signed and unsigned
struct less {
  template <class T, class U>
  bool operator()(const T& t, const U& u) const noexcept {
    return impl(std::is_signed<T>{}, std::is_signed<U>{}, t, u);
  }

  template <class T, class U>
  bool impl(std::false_type, std::false_type, const T& t, const U& u) const noexcept {
    return t < u;
  }

  template <class T, class U>
  bool impl(std::false_type, std::true_type, const T& t, const U& u) const noexcept {
    return u >= 0 && t < make_unsigned(u);
  }

  template <class T, class U>
  bool impl(std::true_type, std::false_type, const T& t, const U& u) const noexcept {
    return t < 0 || make_unsigned(t) < u;
  }

  template <class T, class U>
  bool impl(std::true_type, std::true_type, const T& t, const U& u) const noexcept {
    return t < u;
  }
};

// version of std::greater<> which handles comparison of signed and unsigned
struct greater {
  template <class T, class U>
  bool operator()(const T& t, const U& u) const noexcept {
    return impl(std::is_signed<T>{}, std::is_signed<U>{}, t, u);
  }

  template <class T, class U>
  bool impl(std::false_type, std::false_type, const T& t, const U& u) const noexcept {
    return t > u;
  }

  template <class T, class U>
  bool impl(std::false_type, std::true_type, const T& t, const U& u) const noexcept {
    return u < 0 || t > make_unsigned(u);
  }

  template <class T, class U>
  bool impl(std::true_type, std::false_type, const T& t, const U& u) const noexcept {
    return t >= 0 && make_unsigned(t) > u;
  }

  template <class T, class U>
  bool impl(std::true_type, std::true_type, const T& t, const U& u) const noexcept {
    return t > u;
  }
};

template <class Allocator>
struct mp_int;

template <class T>
struct is_unsigned_integral : mp11::mp_and<std::is_integral<T>, std::is_unsigned<T>> {};

template <class T>
bool safe_increment(T& t) {
  if (t < std::numeric_limits<T>::max()) {
    ++t;
    return true;
  }
  return false;
}

template <class T, class U>
bool safe_radd(T& t, const U& u) {
  static_assert(is_unsigned_integral<T>::value, "T must be unsigned integral type");
  static_assert(is_unsigned_integral<U>::value, "T must be unsigned integral type");
  if (static_cast<T>(std::numeric_limits<T>::max() - t) >= u) {
    t += static_cast<T>(u); // static_cast to suppress conversion warning
    return true;
  }
  return false;
}

// use boost.multiprecision.cpp_int in your own code, it is much more sophisticated
// than this implementation; we use it here to reduce coupling between boost libs
template <class Allocator>
struct mp_int {
  explicit mp_int(Allocator a = {}) : data(1, 0, std::move(a)) {}
  explicit mp_int(uint64_t v, Allocator a = {}) : data(1, v, std::move(a)) {}
  mp_int(const mp_int&) = default;
  mp_int& operator=(const mp_int&) = default;
  mp_int(mp_int&&) = default;
  mp_int& operator=(mp_int&&) = default;

  mp_int& operator=(uint64_t o) {
    data = decltype(data)(1, o);
    return *this;
  }

  mp_int& operator++() {
    BOOST_ASSERT(data.size() > 0u);
    std::size_t i = 0;
    while (!safe_increment(data[i])) {
      data[i] = 0;
      ++i;
      if (i == data.size()) {
        data.push_back(1);
        break;
      }
    }
    return *this;
  }

  mp_int& operator+=(const mp_int& o) {
    if (this == &o) {
      auto tmp{o};
      return operator+=(tmp);
    }
    bool carry = false;
    std::size_t i = 0;
    for (uint64_t oi : o.data) {
      auto& di = maybe_extend(i);
      if (carry) {
        if (safe_increment(oi))
          carry = false;
        else {
          ++i;
          continue;
        }
      }
      if (!safe_radd(di, oi)) {
        add_remainder(di, oi);
        carry = true;
      }
      ++i;
    }
    while (carry) {
      auto& di = maybe_extend(i);
      if (safe_increment(di)) break;
      di = 0;
      ++i;
    }
    return *this;
  }

  mp_int& operator+=(uint64_t o) {
    BOOST_ASSERT(data.size() > 0u);
    if (safe_radd(data[0], o)) return *this;
    add_remainder(data[0], o);
    // carry the one, data may grow several times
    std::size_t i = 1;
    while (true) {
      auto& di = maybe_extend(i);
      if (safe_increment(di)) break;
      di = 0;
      ++i;
    }
    return *this;
  }

  operator double() const noexcept {
    BOOST_ASSERT(data.size() > 0u);
    double result = static_cast<double>(data[0]);
    std::size_t i = 0;
    while (++i < data.size())
      result += static_cast<double>(data[i]) * std::pow(2.0, i * 64);
    return result;
  }

  // total ordering for mp_int, mp_int
  bool operator<(const mp_int& o) const noexcept {
    BOOST_ASSERT(data.size() > 0u);
    BOOST_ASSERT(o.data.size() > 0u);
    // no leading zeros allowed
    BOOST_ASSERT(data.size() == 1 || data.back() > 0u);
    BOOST_ASSERT(o.data.size() == 1 || o.data.back() > 0u);
    if (data.size() < o.data.size()) return true;
    if (data.size() > o.data.size()) return false;
    auto s = data.size();
    while (s > 0u) {
      --s;
      if (data[s] < o.data[s]) return true;
      if (data[s] > o.data[s]) return false;
    }
    return false; // args are equal
  }

  bool operator==(const mp_int& o) const noexcept {
    BOOST_ASSERT(data.size() > 0u);
    BOOST_ASSERT(o.data.size() > 0u);
    // no leading zeros allowed
    BOOST_ASSERT(data.size() == 1 || data.back() > 0u);
    BOOST_ASSERT(o.data.size() == 1 || o.data.back() > 0u);
    if (data.size() != o.data.size()) return false;
    return std::equal(data.begin(), data.end(), o.data.begin());
  }

  // copied from boost/operators.hpp
  friend bool operator>(const mp_int& x, const mp_int& y) { return y < x; }
  friend bool operator<=(const mp_int& x, const mp_int& y) { return !(y < x); }
  friend bool operator>=(const mp_int& x, const mp_int& y) { return !(x < y); }
  friend bool operator!=(const mp_int& x, const mp_int& y) { return !(x == y); }

  // total ordering for mp_int, uint64;  partial ordering for mp_int, double
  template <class U>
  bool operator<(const U& o) const noexcept {
    BOOST_ASSERT(data.size() > 0u);
    return static_if<is_unsigned_integral<U>>(
        [this](uint64_t o) { return data.size() == 1 && data[0] < o; },
        [this](double o) { return operator double() < o; }, o);
  }

  template <class U>
  bool operator>(const U& o) const noexcept {
    BOOST_ASSERT(data.size() > 0u);
    BOOST_ASSERT(data.back() > 0u); // no leading zeros allowed
    return static_if<is_unsigned_integral<U>>(
        [this](uint64_t o) { return data.size() > 1 || data[0] > o; },
        [this](double o) { return operator double() > o; }, o);
  }

  template <class U>
  bool operator==(const U& o) const noexcept {
    BOOST_ASSERT(data.size() > 0u);
    return static_if<is_unsigned_integral<U>>(
        [this](uint64_t o) { return data.size() == 1 && data[0] == o; },
        [this](double o) { return operator double() == o; }, o);
  }

  // adapted copy from boost/operators.hpp
  template <class U>
  friend bool operator<=(const mp_int& x, const U& y) {
    if (is_unsigned_integral<U>::value) return !(x > y);
    return (x < y) || (x == y);
  }
  template <class U>
  friend bool operator>=(const mp_int& x, const U& y) {
    if (is_unsigned_integral<U>::value) return !(x < y);
    return (x > y) || (x == y);
  }
  template <class U>
  friend bool operator>(const U& x, const mp_int& y) {
    if (is_unsigned_integral<U>::value) return y < x;
    return y < x;
  }
  template <class U>
  friend bool operator<(const U& x, const mp_int& y) {
    if (is_unsigned_integral<U>::value) return y > x;
    return y > x;
  }
  template <class U>
  friend bool operator<=(const U& x, const mp_int& y) {
    if (is_unsigned_integral<U>::value) return !(y < x);
    return (y > x) || (y == x);
  }
  template <class U>
  friend bool operator>=(const U& x, const mp_int& y) {
    if (is_unsigned_integral<U>::value) return !(y > x);
    return (y < x) || (y == x);
  }
  template <class U>
  friend bool operator==(const U& y, const mp_int& x) {
    return x == y;
  }
  template <class U>
  friend bool operator!=(const U& y, const mp_int& x) {
    return !(x == y);
  }
  template <class U>
  friend bool operator!=(const mp_int& y, const U& x) {
    return !(y == x);
  }

  uint64_t& maybe_extend(std::size_t i) {
    while (i >= data.size()) data.push_back(0);
    return data[i];
  }

  static void add_remainder(uint64_t& d, const uint64_t o) noexcept {
    BOOST_ASSERT(d > 0u);
    // in decimal system it would look like this:
    // 8 + 8 = 6 = 8 - (9 - 8) - 1
    // 9 + 1 = 0 = 9 - (9 - 1) - 1
    auto tmp = std::numeric_limits<uint64_t>::max();
    tmp -= o;
    --d -= tmp;
  }

  std::vector<uint64_t, Allocator> data;
};

template <class Allocator>
auto create_buffer(Allocator& a, std::size_t n) {
  using AT = std::allocator_traits<Allocator>;
  auto ptr = AT::allocate(a, n); // may throw
  static_assert(std::is_trivially_copyable<decltype(ptr)>::value,
                "ptr must be trivially copyable");
  auto it = ptr;
  const auto end = ptr + n;
  try {
    // this loop may throw
    while (it != end) AT::construct(a, it++, typename AT::value_type{});
  } catch (...) {
    // release resources that were already acquired before rethrowing
    while (it != ptr) AT::destroy(a, --it);
    AT::deallocate(a, ptr, n);
    throw;
  }
  return ptr;
}

template <class Allocator, class Iterator>
auto create_buffer(Allocator& a, std::size_t n, Iterator iter) {
  BOOST_ASSERT(n > 0u);
  using AT = std::allocator_traits<Allocator>;
  auto ptr = AT::allocate(a, n); // may throw
  static_assert(std::is_trivially_copyable<decltype(ptr)>::value,
                "ptr must be trivially copyable");
  auto it = ptr;
  const auto end = ptr + n;
  try {
    // this loop may throw
    while (it != end) AT::construct(a, it++, *iter++);
  } catch (...) {
    // release resources that were already acquired before rethrowing
    while (it != ptr) AT::destroy(a, --it);
    AT::deallocate(a, ptr, n);
    throw;
  }
  return ptr;
}

template <class Allocator>
void destroy_buffer(Allocator& a, typename std::allocator_traits<Allocator>::pointer p,
                    std::size_t n) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(n > 0u);
  using AT = std::allocator_traits<Allocator>;
  auto it = p + n;
  while (it != p) AT::destroy(a, --it);
  AT::deallocate(a, p, n);
}

} // namespace detail

/**
  Memory-efficient storage for integral counters which cannot overflow.

  This storage provides a no-overflow-guarantee if it is filled with integral weights
  only. This storage implementation keeps a contiguous array of elemental counters, one
  for each cell. If an operation is requested, which would overflow a counter, the whole
  array is replaced with another of a wider integral type, then the operation is executed.
  The storage uses integers of 8, 16, 32, 64 bits, and then switches to a multiprecision
  integral type, similar to those in
  [Boost.Multiprecision](https://www.boost.org/doc/libs/develop/libs/multiprecision/doc/html/index.html).

  A scaling operation or adding a floating point number turns the elements into doubles,
  which voids the no-overflow-guarantee.
*/
template <class Allocator>
class unlimited_storage {
  static_assert(
      std::is_same<typename std::allocator_traits<Allocator>::pointer,
                   typename std::allocator_traits<Allocator>::value_type*>::value,
      "unlimited_storage requires allocator with trivial pointer type");

public:
  using allocator_type = Allocator;
  using value_type = double;
  using mp_int = detail::mp_int<
      typename std::allocator_traits<allocator_type>::template rebind_alloc<uint64_t>>;

private:
  using types = mp11::mp_list<uint8_t, uint16_t, uint32_t, uint64_t, mp_int, double>;

  template <class T>
  static constexpr char type_index() noexcept {
    return static_cast<char>(mp11::mp_find<types, T>::value);
  }

  struct buffer_type {
    allocator_type alloc;
    std::size_t size = 0;
    char type = 0;
    void* ptr = nullptr;

    template <class F, class... Ts>
    decltype(auto) apply(F&& f, Ts&&... ts) const {
      // this is intentionally not a switch, the if-chain is faster in benchmarks
      if (type == type_index<uint8_t>())
        return f(static_cast<uint8_t*>(ptr), std::forward<Ts>(ts)...);
      if (type == type_index<uint16_t>())
        return f(static_cast<uint16_t*>(ptr), std::forward<Ts>(ts)...);
      if (type == type_index<uint32_t>())
        return f(static_cast<uint32_t*>(ptr), std::forward<Ts>(ts)...);
      if (type == type_index<uint64_t>())
        return f(static_cast<uint64_t*>(ptr), std::forward<Ts>(ts)...);
      if (type == type_index<mp_int>())
        return f(static_cast<mp_int*>(ptr), std::forward<Ts>(ts)...);
      return f(static_cast<double*>(ptr), std::forward<Ts>(ts)...);
    }

    buffer_type(allocator_type a = {}) : alloc(std::move(a)) {}

    buffer_type(buffer_type&& o) noexcept
        : alloc(std::move(o.alloc)), size(o.size), type(o.type), ptr(o.ptr) {
      o.size = 0;
      o.type = 0;
      o.ptr = nullptr;
    }

    buffer_type& operator=(buffer_type&& o) noexcept {
      if (this != &o) {
        using std::swap;
        swap(alloc, o.alloc);
        swap(size, o.size);
        swap(type, o.type);
        swap(ptr, o.ptr);
      }
      return *this;
    }

    buffer_type(const buffer_type& o) : alloc(o.alloc) {
      o.apply([this, &o](auto* otp) {
        using T = detail::remove_cvref_t<decltype(*otp)>;
        this->template make<T>(o.size, otp);
      });
    }

    buffer_type& operator=(const buffer_type& o) {
      *this = buffer_type(o);
      return *this;
    }

    ~buffer_type() noexcept { destroy(); }

    void destroy() noexcept {
      BOOST_ASSERT((ptr == nullptr) == (size == 0));
      if (ptr == nullptr) return;
      apply([this](auto* tp) {
        using T = detail::remove_cvref_t<decltype(*tp)>;
        using alloc_type =
            typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
        alloc_type a(alloc); // rebind allocator
        detail::destroy_buffer(a, tp, size);
      });
      size = 0;
      type = 0;
      ptr = nullptr;
    }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244) // possible loss of data
#endif

    template <class T>
    void make(std::size_t n) {
      // note: order of commands is to not leave buffer in invalid state upon throw
      destroy();
      if (n > 0) {
        // rebind allocator
        using alloc_type =
            typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
        alloc_type a(alloc);
        ptr = detail::create_buffer(a, n); // may throw
      }
      size = n;
      type = type_index<T>();
    }

    template <class T, class U>
    void make(std::size_t n, U iter) {
      // note: iter may be current ptr, so create new buffer before deleting old buffer
      T* new_ptr = nullptr;
      const auto new_type = type_index<T>();
      if (n > 0) {
        // rebind allocator
        using alloc_type =
            typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
        alloc_type a(alloc);
        new_ptr = detail::create_buffer(a, n, iter); // may throw
      }
      destroy();
      size = n;
      type = new_type;
      ptr = new_ptr;
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  };

  template <class Buffer>
  class reference_t {
  public:
    reference_t(Buffer* b, std::size_t i) : buffer_(b), idx_(i) {}

    reference_t(const reference_t&) = default;
    reference_t& operator=(const reference_t&) = delete; // references do not rebind
    reference_t& operator=(reference_t&&) = delete;      // references do not rebind

    // minimal operators for partial ordering
    bool operator<(reference_t rhs) const { return op<detail::less>(rhs); }
    bool operator>(reference_t rhs) const { return op<detail::greater>(rhs); }
    bool operator==(reference_t rhs) const { return op<detail::equal>(rhs); }

    // adapted copy from boost/operators.hpp for partial ordering
    friend bool operator<=(reference_t x, reference_t y) { return !(y < x); }
    friend bool operator>=(reference_t x, reference_t y) { return !(y > x); }
    friend bool operator!=(reference_t y, reference_t x) { return !(x == y); }

    template <class U>
    bool operator<(const U& rhs) const {
      return op<detail::less>(rhs);
    }

    template <class U>
    bool operator>(const U& rhs) const {
      return op<detail::greater>(rhs);
    }

    template <class U>
    bool operator==(const U& rhs) const {
      return op<detail::equal>(rhs);
    }

    // adapted copy from boost/operators.hpp
    template <class U>
    friend bool operator<=(reference_t x, const U& y) {
      if (detail::is_unsigned_integral<U>::value) return !(x > y);
      return (x < y) || (x == y);
    }
    template <class U>
    friend bool operator>=(reference_t x, const U& y) {
      if (detail::is_unsigned_integral<U>::value) return !(x < y);
      return (x > y) || (x == y);
    }
    template <class U>
    friend bool operator>(const U& x, reference_t y) {
      return y < x;
    }
    template <class U>
    friend bool operator<(const U& x, reference_t y) {
      return y > x;
    }
    template <class U>
    friend bool operator<=(const U& x, reference_t y) {
      if (detail::is_unsigned_integral<U>::value) return !(y < x);
      return (y > x) || (y == x);
    }
    template <class U>
    friend bool operator>=(const U& x, reference_t y) {
      if (detail::is_unsigned_integral<U>::value) return !(y > x);
      return (y < x) || (y == x);
    }
    template <class U>
    friend bool operator==(const U& y, reference_t x) {
      return x == y;
    }
    template <class U>
    friend bool operator!=(const U& y, reference_t x) {
      return !(x == y);
    }
    template <class U>
    friend bool operator!=(reference_t y, const U& x) {
      return !(y == x);
    }

    operator double() const {
      return buffer_->apply(
          [this](const auto* tp) { return static_cast<double>(tp[idx_]); });
    }

  protected:
    template <class Binary, class U>
    bool op(const reference_t<U>& rhs) const {
      const auto i = idx_;
      const auto j = rhs.idx_;
      return buffer_->apply([i, j, &rhs](const auto* ptr) {
        const auto& pi = ptr[i];
        return rhs.buffer_->apply([&pi, j](const auto* q) { return Binary()(pi, q[j]); });
      });
    }

    template <class Binary, class U>
    bool op(const U& rhs) const {
      const auto i = idx_;
      return buffer_->apply([i, &rhs](const auto* tp) { return Binary()(tp[i], rhs); });
    }

    template <class U>
    friend class reference_t;

    Buffer* buffer_;
    std::size_t idx_;
  };

public:
  using const_reference = reference_t<const buffer_type>;

  class reference : public reference_t<buffer_type> {
    using base_type = reference_t<buffer_type>;

  public:
    using base_type::base_type;

    reference operator=(reference t) {
      t.buffer_->apply([this, &t](const auto* otp) { *this = otp[t.idx_]; });
      return *this;
    }

    reference operator=(const_reference t) {
      t.buffer_->apply([this, &t](const auto* otp) { *this = otp[t.idx_]; });
      return *this;
    }

    template <class U>
    reference operator=(const U& t) {
      base_type::buffer_->apply([this, &t](auto* tp) {
        tp[this->idx_] = 0;
        adder()(tp, *(this->buffer_), this->idx_, t);
      });
      return *this;
    }

    template <class U>
    reference operator+=(const U& t) {
      base_type::buffer_->apply(adder(), *base_type::buffer_, base_type::idx_, t);
      return *this;
    }

    template <class U>
    reference operator*=(const U& t) {
      base_type::buffer_->apply(multiplier(), *base_type::buffer_, base_type::idx_, t);
      return *this;
    }

    template <class U>
    reference operator-=(const U& t) {
      return operator+=(-t);
    }

    template <class U>
    reference operator/=(const U& t) {
      return operator*=(1.0 / static_cast<double>(t));
    }

    reference operator++() {
      base_type::buffer_->apply(incrementor(), *base_type::buffer_, base_type::idx_);
      return *this;
    }

    // minimal operators for partial ordering
    bool operator<(reference rhs) const { return base_type::operator<(rhs); }
    bool operator>(reference rhs) const { return base_type::operator>(rhs); }
    bool operator==(reference rhs) const { return base_type::operator==(rhs); }

    // adapted copy from boost/operators.hpp for partial ordering
    friend bool operator<=(reference x, reference y) { return !(y < x); }
    friend bool operator>=(reference x, reference y) { return !(y > x); }
    friend bool operator!=(reference y, reference x) { return !(x == y); }
  };

private:
  template <class Value, class Reference, class Buffer>
  class iterator_t
      : public boost::iterator_adaptor<iterator_t<Value, Reference, Buffer>, std::size_t,
                                       Value, std::random_access_iterator_tag, Reference,
                                       std::ptrdiff_t> {

  public:
    iterator_t() = default;
    template <class V, class R, class B>
    iterator_t(const iterator_t<V, R, B>& it)
        : iterator_t::iterator_adaptor_(it.base()), buffer_(it.buffer_) {}
    iterator_t(Buffer* b, std::size_t i) noexcept
        : iterator_t::iterator_adaptor_(i), buffer_(b) {}

  protected:
    template <class V, class R, class B>
    bool equal(const iterator_t<V, R, B>& rhs) const noexcept {
      return buffer_ == rhs.buffer_ && this->base() == rhs.base();
    }
    Reference dereference() const { return {buffer_, this->base()}; }

    friend class ::boost::iterator_core_access;
    template <class V, class R, class B>
    friend class iterator_t;

  private:
    Buffer* buffer_ = nullptr;
  };

public:
  using const_iterator = iterator_t<const value_type, const_reference, const buffer_type>;
  using iterator = iterator_t<value_type, reference, buffer_type>;

  explicit unlimited_storage(allocator_type a = {}) : buffer(std::move(a)) {}
  unlimited_storage(const unlimited_storage&) = default;
  unlimited_storage& operator=(const unlimited_storage&) = default;
  unlimited_storage(unlimited_storage&&) = default;
  unlimited_storage& operator=(unlimited_storage&&) = default;

  template <class T>
  unlimited_storage(const storage_adaptor<T>& s) {
    using V = detail::remove_cvref_t<decltype(s[0])>;
    constexpr auto ti = type_index<V>();
    detail::static_if_c<(ti < mp11::mp_size<types>::value)>(
        [&](auto) { buffer.template make<V>(s.size(), s.begin()); },
        [&](auto) { buffer.template make<double>(s.size(), s.begin()); }, 0);
  }

  template <class Iterable, class = detail::requires_iterable<Iterable>>
  unlimited_storage& operator=(const Iterable& s) {
    *this = unlimited_storage(s);
    return *this;
  }

  allocator_type get_allocator() const { return buffer.alloc; }

  void reset(std::size_t s) { buffer.template make<uint8_t>(s); }

  std::size_t size() const noexcept { return buffer.size; }

  reference operator[](std::size_t i) noexcept { return {&buffer, i}; }
  const_reference operator[](std::size_t i) const noexcept { return {&buffer, i}; }

  bool operator==(const unlimited_storage& o) const noexcept {
    if (size() != o.size()) return false;
    return buffer.apply([&o](const auto* ptr) {
      return o.buffer.apply([ptr, &o](const auto* optr) {
        return std::equal(ptr, ptr + o.size(), optr, detail::equal{});
      });
    });
  }

  template <class T>
  bool operator==(const T& o) const {
    if (size() != o.size()) return false;
    return buffer.apply([&o](const auto* ptr) {
      return std::equal(ptr, ptr + o.size(), std::begin(o), detail::equal{});
    });
  }

  unlimited_storage& operator*=(const double x) {
    buffer.apply(multiplier(), buffer, x);
    return *this;
  }

  iterator begin() noexcept { return {&buffer, 0}; }
  iterator end() noexcept { return {&buffer, size()}; }
  const_iterator begin() const noexcept { return {&buffer, 0}; }
  const_iterator end() const noexcept { return {&buffer, size()}; }

  /// @private used by unit tests, not part of generic storage interface
  template <class T>
  unlimited_storage(std::size_t s, const T* p, allocator_type a = {})
      : buffer(std::move(a)) {
    buffer.template make<T>(s, p);
  }

  template <class Archive>
  void serialize(Archive&, unsigned);

private:
  struct incrementor {
    template <class T, class Buffer>
    void operator()(T* tp, Buffer& b, std::size_t i) {
      if (!detail::safe_increment(tp[i])) {
        using U = mp11::mp_at_c<types, (type_index<T>() + 1)>;
        b.template make<U>(b.size, tp);
        ++static_cast<U*>(b.ptr)[i];
      }
    }

    template <class Buffer>
    void operator()(mp_int* tp, Buffer&, std::size_t i) {
      ++tp[i];
    }

    template <class Buffer>
    void operator()(double* tp, Buffer&, std::size_t i) {
      ++tp[i];
    }
  };

  struct adder {
    template <class Buffer, class U>
    void operator()(double* tp, Buffer&, std::size_t i, const U& x) {
      tp[i] += static_cast<double>(x);
    }

    template <class T, class Buffer, class U>
    void operator()(T* tp, Buffer& b, std::size_t i, const U& x) {
      U_is_integral(std::is_integral<U>{}, tp, b, i, x);
    }

    template <class T, class Buffer, class U>
    void U_is_integral(std::false_type, T* tp, Buffer& b, std::size_t i, const U& x) {
      b.template make<double>(b.size, tp);
      operator()(static_cast<double*>(b.ptr), b, i, x);
    }

    template <class T, class Buffer, class U>
    void U_is_integral(std::true_type, T* tp, Buffer& b, std::size_t i, const U& x) {
      U_is_unsigned_integral(std::is_unsigned<U>{}, tp, b, i, x);
    }

    template <class T, class Buffer, class U>
    void U_is_unsigned_integral(std::false_type, T* tp, Buffer& b, std::size_t i,
                                const U& x) {
      if (x >= 0)
        U_is_unsigned_integral(std::true_type{}, tp, b, i, detail::make_unsigned(x));
      else
        U_is_integral(std::false_type{}, tp, b, i, static_cast<double>(x));
    }

    template <class Buffer, class U>
    void U_is_unsigned_integral(std::true_type, mp_int* tp, Buffer&, std::size_t i,
                                const U& x) {
      tp[i] += x;
    }

    template <class T, class Buffer, class U>
    void U_is_unsigned_integral(std::true_type, T* tp, Buffer& b, std::size_t i,
                                const U& x) {
      if (detail::safe_radd(tp[i], x)) return;
      using V = mp11::mp_at_c<types, (type_index<T>() + 1)>;
      b.template make<V>(b.size, tp);
      U_is_unsigned_integral(std::true_type{}, static_cast<V*>(b.ptr), b, i, x);
    }
  };

  struct multiplier {
    template <class T, class Buffer>
    void operator()(T* tp, Buffer& b, const double x) {
      // potential lossy conversion that cannot be avoided
      b.template make<double>(b.size, tp);
      operator()(static_cast<double*>(b.ptr), b, x);
    }

    template <class Buffer>
    void operator()(double* tp, Buffer& b, const double x) {
      for (auto end = tp + b.size; tp != end; ++tp) *tp *= x;
    }

    template <class T, class Buffer, class U>
    void operator()(T* tp, Buffer& b, std::size_t i, const U& x) {
      b.template make<double>(b.size, tp);
      operator()(static_cast<double*>(b.ptr), b, i, x);
    }

    template <class Buffer, class U>
    void operator()(double* tp, Buffer&, std::size_t i, const U& x) {
      tp[i] *= static_cast<double>(x);
    }
  };

  buffer_type buffer;
};

} // namespace histogram
} // namespace boost

#endif
