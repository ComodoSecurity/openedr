// Copyright 2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt of copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iterator>
#include <gtest/gtest.h>
#include "test_uri.hpp"
#include "string_utility.hpp"

// scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

TEST(uri_parse_test, test_valid_scheme) {
  test::uri uri("http://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
}

TEST(uri_parse_test, test_scheme_beginning_with_a_colon) {
  test::uri uri(":http://user@www.example.com:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
  ASSERT_FALSE(uri.has_scheme());
}

TEST(uri_parse_test, test_scheme_beginning_with_a_number) {
  test::uri uri("8http://user@www.example.com:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_scheme_with_a_minus) {
  test::uri uri("ht-tp://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("ht-tp", uri.scheme());
}

TEST(uri_parse_test, test_scheme_with_a_plus) {
  test::uri uri("ht+tp://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("ht+tp", uri.scheme());
}

TEST(uri_parse_test, test_scheme_with_a_dot) {
  test::uri uri("ht.tp://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("ht.tp", uri.scheme());
}

TEST(uri_parse_test, test_scheme_with_a_number) {
  test::uri uri("http1://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http1", uri.scheme());
}

TEST(uri_parse_test, test_scheme_with_an_invalid_character) {
  test::uri uri("http$://user@www.example.com:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_scheme_with_capital_letters) {
  test::uri uri("HTTP://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("HTTP", uri.scheme());
}

TEST(uri_parse_test, test_scheme_with_a_percent) {
  test::uri uri("ht%tp://user@www.example.com:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_scheme_with_a_valid_percent_encoded_character) {
  test::uri uri("ht%00tp://user@www.example.com:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
}
