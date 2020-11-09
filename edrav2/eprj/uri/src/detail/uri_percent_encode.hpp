// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_URI_PERCENT_ENCODE_INC
#define NETWORK_DETAIL_URI_PERCENT_ENCODE_INC

#include <string>
#include <vector>
#include <locale>
#include <network/optional.hpp>
#include <network/uri/detail/decode.hpp>

namespace network {
namespace detail {

inline optional<char> percent_encode(std::string::const_iterator it) {
  try {
    char output = '\0';
    detail::decode_char(it, &output);
    return output;
  } catch (percent_decoding_error &) {
    return optional<char>();
  }
}

template <class String>
struct percent_encoded_to_upper {
  percent_encoded_to_upper() : count(0) {}

  void operator()(typename String::value_type &c) {
    if (c == '%') {
      count = 2;
    } else if (count > 0) {
      c = std::toupper(c, std::locale());
      --count;
    }
  }

  unsigned count;
};

template <class Iter>
Iter decode_encoded_unreserved_chars(Iter first, Iter last) {

  // unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"

  const auto is_unreserved = [](char c)
    {
      return std::isalnum(c, std::locale())
      || '-' == c
      || '.' == c
      || '_' == c
      || '~' == c;
    };

  auto it = first, it2 = first;
  while (it != last) {
    if (*it == '%') {
      const auto sfirst = it;
      const auto opt_char = percent_encode(sfirst);
      if (opt_char && is_unreserved(*opt_char)) {
        *it2 = *opt_char;
        ++it; ++it;
      } else {
        *it2 = *it;
      }
    }
    else {
      *it2 = *it;
    }
    ++it; ++it2;
  }
  return it2;
}
}  // namespace detail
}  // namespace network

#endif  // NETWORK_DETAIL_URI_PERCENT_ENCODE_INC
