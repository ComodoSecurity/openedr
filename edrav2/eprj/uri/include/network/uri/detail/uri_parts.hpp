// Copyright 2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_URI_PARTS_INC
#define NETWORK_DETAIL_URI_PARTS_INC

#include <string>
#include <utility>
#include <iterator>
#include <network/optional.hpp>
#include <network/string_view.hpp>

namespace network {
namespace detail {
class uri_part {
 public:
  typedef string_view::value_type value_type;
  typedef string_view::iterator iterator;
  typedef string_view::const_iterator const_iterator;

  uri_part() {}

  uri_part(const_iterator first, const_iterator last)
      : first(first), last(last) {}

  const_iterator begin() const { return first; }

  const_iterator end() const { return last; }

  bool empty() const { return first == last; }

  std::string to_string() const { return std::string(first, last); }

 private:
  const_iterator first, last;
};

struct hierarchical_part {
  optional<uri_part> user_info;
  optional<uri_part> host;
  optional<uri_part> port;
  optional<uri_part> path;
};

struct uri_parts {
  optional<uri_part> scheme;
  hierarchical_part hier_part;
  optional<uri_part> query;
  optional<uri_part> fragment;
};
}  // namespace detail
}  // namespace network

#endif  // NETWORK_DETAIL_URI_PARTS_INC
