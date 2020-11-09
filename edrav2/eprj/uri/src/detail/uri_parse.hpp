// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_URI_PARSE_INC
#define NETWORK_DETAIL_URI_PARSE_INC

#include <network/string_view.hpp>

namespace network {
namespace detail {
struct uri_parts;

bool parse(string_view::const_iterator &first, string_view::const_iterator last, uri_parts &parts);
}  // namespace detail
}  // namespace network

#endif  // NETWORK_DETAIL_URI_PARSE_INC
