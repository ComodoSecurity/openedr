// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt of copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef URI_TEST_STRING_UTILITY_INC
#define URI_TEST_STRING_UTILITY_INC

#include <network/string_view.hpp>

namespace network {
inline bool operator==(const char *lhs, string_view rhs) {
  return string_view(lhs) == rhs;
}
}  // namespace network

#endif  // URI_TEST_STRING_UTILITY_INC
