// Copyright 2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_RANGE_INC
#define NETWORK_DETAIL_RANGE_INC

#include <algorithm>
#include <iterator>
#include <utility>
#include <string>
#include <cctype>
#include <locale>

namespace network {
namespace detail {
template <class Rng, class Pred>
inline void for_each(Rng& rng, Pred&& pred) {
  std::for_each(std::begin(rng), std::end(rng), pred);
}

template <class Rng, class Iter, class Pred>
inline void transform(Rng& rng, Iter out, Pred&& pred) {
  std::transform(std::begin(rng), std::end(rng), out, pred);
}

template <class Rng>
inline typename Rng::difference_type distance(Rng& rng) {
  return std::distance(std::begin(rng), std::end(rng));
}

template <class Rng1, class Rng2>
inline bool equal(const Rng1& rng1, const Rng2& rng2) {
  return std::equal(std::begin(rng1), std::end(rng1), std::begin(rng2));
}

template <class Rng, class Pred>
inline void remove_erase_if(Rng& rng, Pred&& pred) {
  auto first = std::begin(rng), last = std::end(rng);
  auto it = std::remove_if(first, last, pred);
  rng.erase(it, last);
}

inline std::string trim_front(const std::string& str) {
  auto first = std::begin(str), last = std::end(str);
  auto it = std::find_if(first, last, [](char ch) {
    return !std::isspace(ch, std::locale());
  });
  return std::string(it, last);
}

inline std::string trim_back(const std::string& str) {
  using reverse_iterator = std::reverse_iterator<std::string::const_iterator>;

  auto first = reverse_iterator(std::end(str)),
       last = reverse_iterator(std::begin(str));
  auto it = std::find_if(first, last, [](char ch) {
    return !std::isspace(ch, std::locale());
  });
  std::string result(it, last);
  std::reverse(std::begin(result), std::end(result));
  return result;
}

inline
std::string trim_copy(const std::string &str) {
  return trim_back(trim_front(str));
}
}  // namespace detail
}  // namespace network

#endif  // NETWORK_DETAIL_RANGE_INC
