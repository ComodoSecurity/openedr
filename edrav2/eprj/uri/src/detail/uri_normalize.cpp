// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iterator>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include "uri_normalize.hpp"
#include "uri_percent_encode.hpp"
#include "algorithm.hpp"

namespace network {
namespace detail {
std::string normalize_path_segments(string_view path) {
  std::string result;

  if (!path.empty()) {
    std::vector<std::string> path_segments;
    boost::split(path_segments, path, [](char ch) {
      return ch == '/';
    });

    // remove single dot segments
    detail::remove_erase_if(
        path_segments, [](const std::string& s) { return (s == "."); });

    // remove double dot segments
    std::vector<std::string> normalized_segments;
    for (auto& segment : path_segments) {
      if (segment == "..") {
        if (normalized_segments.size() <= 1) {
          throw uri_builder_error();
        }
        normalized_segments.pop_back();
      }
      else {
        normalized_segments.push_back(segment);
      }
    }

    // remove adjacent slashes
    optional<std::string> prev_segment;
    detail::remove_erase_if(
        normalized_segments, [&prev_segment](const std::string& segment) {
          bool has_adjacent_slash =
              ((prev_segment && prev_segment->empty()) && segment.empty());
          if (!has_adjacent_slash) {
            prev_segment = segment;
          }
          return has_adjacent_slash;
        });

    result = boost::join(normalized_segments, "/");
  }

  if (result.empty()) {
    result = "/";
  }

  return result;
}

std::string normalize_path(string_view path, uri_comparison_level level) {
  auto result = path.to_string();

  if (uri_comparison_level::syntax_based == level) {
    // case normalization
    detail::for_each(result, percent_encoded_to_upper<std::string>());

    // % encoding normalization
    result.erase(
        detail::decode_encoded_unreserved_chars(std::begin(result),
                                                std::end(result)),
        std::end(result));

    // % path segment normalization
    result = normalize_path_segments(result);
  }

  return result;
}
}  // namespace detail
}  // namespace network
