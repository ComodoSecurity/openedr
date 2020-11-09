// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_NORMALIZE_INC
#define NETWORK_DETAIL_NORMALIZE_INC

#include <network/uri/uri.hpp>
#include <network/string_view.hpp>

namespace network {
namespace detail {
std::string normalize_path_segments(string_view path);

std::string normalize_path(string_view path, uri_comparison_level level);
}  // namespace detail
}  // namespace network

#endif  // NETWORK_DETAIL_NORMALIZE_INC
