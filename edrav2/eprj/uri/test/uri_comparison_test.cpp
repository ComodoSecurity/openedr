// Copyright (c) Glyn Matthews 2012-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <network/uri.hpp>


TEST(uri_comparison_test, equality_test) {
  network::uri lhs("http://www.example.com/");
  network::uri rhs("http://www.example.com/");
  ASSERT_EQ(lhs, rhs);
}

TEST(uri_comparison_test, equality_test_capitalized_scheme) {
  network::uri lhs("http://www.example.com/");
  network::uri rhs("HTTP://www.example.com/");
  ASSERT_NE(lhs.compare(rhs, network::uri_comparison_level::string_comparison), 0);
}

TEST(uri_comparison_test, equality_test_capitalized_scheme_with_case_normalization) {
  network::uri lhs("http://www.example.com/");
  network::uri rhs("HTTP://www.example.com/");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, DISABLED_equality_test_capitalized_host) {
  network::uri lhs("http://www.example.com/");
  network::uri rhs("http://WWW.EXAMPLE.COM/");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, equality_test_with_single_dot_segment) {
  network::uri lhs("http://www.example.com/./path");
  network::uri rhs("http://www.example.com/path");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, equality_test_with_double_dot_segment) {
  network::uri lhs("http://www.example.com/1/../2/");
  network::uri rhs("http://www.example.com/2/");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, DISABLED_given_example_test) {
  network::uri lhs("example://a/b/c/%7Bfoo%7D");
  network::uri rhs("eXAMPLE://a/./b/../b/%63/%7bfoo%7d");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, equality_empty_lhs) {
  network::uri lhs;
  network::uri rhs("http://www.example.com/");
  ASSERT_NE(lhs, rhs);
}

TEST(uri_comparison_test, equality_empty_rhs) {
  network::uri lhs("http://www.example.com/");
  network::uri rhs;
  ASSERT_NE(lhs, rhs);
}

TEST(uri_comparison_test, inequality_test) {
  network::uri lhs("http://www.example.com/");
  network::uri rhs("http://www.example.com/");
  ASSERT_FALSE(lhs != rhs);
}

TEST(uri_comparison_test, less_than_test) {
  // lhs is lexicographically less than rhs
  network::uri lhs("http://www.example.com/");
  network::uri rhs("http://www.example.org/");
  ASSERT_LT(lhs, rhs);
}

TEST(uri_comparison_test, percent_encoded_query_reserved_chars) {
  network::uri lhs("http://www.example.com?foo=%5cbar");
  network::uri rhs("http://www.example.com?foo=%5Cbar");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}


TEST(uri_comparison_test, percent_encoded_query_unreserved_chars) {
  network::uri lhs("http://www.example.com?foo=%61%6c%70%68%61%31%32%33%2d%2e%5f%7e");
  network::uri rhs("http://www.example.com?foo=alpha123-._~");
  ASSERT_EQ(lhs.compare(rhs, network::uri_comparison_level::syntax_based), 0);
}
