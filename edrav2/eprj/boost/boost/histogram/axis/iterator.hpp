// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_ITERATOR_HPP
#define BOOST_HISTOGRAM_AXIS_ITERATOR_HPP

#include <boost/histogram/axis/interval_view.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/reverse_iterator.hpp>

namespace boost {
namespace histogram {
namespace axis {

template <typename Axis>
class iterator
    : public boost::iterator_adaptor<iterator<Axis>, int,
                                     decltype(std::declval<const Axis&>().bin(0)),
                                     std::random_access_iterator_tag,
                                     decltype(std::declval<const Axis&>().bin(0)), int> {
public:
  explicit iterator(const Axis& axis, int idx)
      : iterator::iterator_adaptor_(idx), axis_(axis) {}

protected:
  bool equal(const iterator& other) const noexcept {
    return &axis_ == &other.axis_ && this->base() == other.base();
  }

  decltype(auto) dereference() const { return axis_.bin(this->base()); }

private:
  const Axis& axis_;
  friend class boost::iterator_core_access;
};

/// Uses CRTP to inject iterator logic into Derived.
template <typename Derived>
class iterator_mixin {
public:
  using const_iterator = iterator<Derived>;
  using const_reverse_iterator = boost::reverse_iterator<const_iterator>;

  /// Bin iterator to beginning of the axis (read-only).
  const_iterator begin() const noexcept {
    return const_iterator(*static_cast<const Derived*>(this), 0);
  }

  /// Bin iterator to the end of the axis (read-only).
  const_iterator end() const noexcept {
    return const_iterator(*static_cast<const Derived*>(this),
                          static_cast<const Derived*>(this)->size());
  }

  /// Reverse bin iterator to the last entry of the axis (read-only).
  const_reverse_iterator rbegin() const noexcept {
    return boost::make_reverse_iterator(end());
  }

  /// Reverse bin iterator to the end (read-only).
  const_reverse_iterator rend() const noexcept {
    return boost::make_reverse_iterator(begin());
  }
};

} // namespace axis
} // namespace histogram
} // namespace boost

#endif
