// Copyright 2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "uri_parse_authority.hpp"
#include <cstdlib>
#include <iterator>
#include <limits>
#include "grammar.hpp"

namespace network {
namespace detail {
namespace {
enum class authority_state {
  user_info,
  host,
  host_ipv6,
  port
};
}  // namespace

bool parse_authority(string_view::const_iterator &it,
                     string_view::const_iterator last,
                     optional<uri_part> &user_info,
                     optional<uri_part> &host,
                     optional<uri_part> &port) {
  auto first = it;

  auto state = authority_state::user_info;
  while (it != last) {
    if (state == authority_state::user_info) {
      if (is_in(first, last, "@:")) {
        return false;
      }

      if (*it == '@') {
        user_info = uri_part(first, it);
        state = authority_state::host;
        ++it;
        first = it;
        continue;
      }
      else if (*it == '[') {
        // this is an IPv6 address
        state = authority_state::host_ipv6;
        first = it;
        continue;
      }
      else if (*it == ':') {
        // this is actually the host, and the next part is expected to be the port
        host = uri_part(first, it);
        state = authority_state::port;
        ++it;
        first = it;
        continue;
      }
    }
    else if (state == authority_state::host) {
      if (*first == ':') {
        return false;
      }

      if (*it == ':') {
        host = uri_part(first, it);
        state = authority_state::port;
        ++it;
        first = it;
        continue;
      }
    }
    else if (state == authority_state::host_ipv6) {
      if (*first != '[') {
        return false;
      }

      if (*it == ']') {
        host = uri_part(first, it);
        ++it;
        // Then test if the next part is a host, part, or the end of the file
        if (it == last) {
          break;
        }
        else if (*it == ':') {
          host = uri_part(first, it);
          state = authority_state::port;
          ++it;
          first = it;
        }
      }
    }
    else if (state == authority_state::port) {
      if (*first == '/') {
        // the port is empty, but valid
        port = uri_part(first, it);
        if (!is_valid_port(std::begin(*port))) {
          return false;
        }

        continue;
      }

      if (!isdigit(it, last)) {
        return false;
      }
    }

    ++it;
  }

  if (state == authority_state::user_info) {
    host = uri_part(first, last);
  }
  else if (state == authority_state::host) {
    host = uri_part(first, last);
  }
  else if (state == authority_state::host_ipv6) {
    host = uri_part(first, last);
  }
  else if (state == authority_state::port) {
    port = uri_part(first, last);
    if (!is_valid_port(std::begin(*port))) {
      return false;
    }
  }

  return true;
}
}  // namespace detail
}  // namespace network
