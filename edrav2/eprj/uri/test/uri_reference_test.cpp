// Copyright (c) Glyn Matthews 2012-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <network/uri.hpp>


TEST(uri_make_relative_test, opaque_uri) {
  network::uri uri_1("mailto:glynos@example.com");
  network::uri uri_2("mailto:john.doe@example.com");
  ASSERT_EQ(uri_2, uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, simple_test) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/path/");
  ASSERT_EQ("/path/", uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, simple_test_with_different_authority) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.org/path/");
  ASSERT_EQ("http://www.example.org/path/", uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, simple_test_is_relative) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/path/");
  ASSERT_FALSE(uri_1.make_relative(uri_2).is_absolute());
}

TEST(uri_make_relative_test, simple_test_is_hierarchical) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/path/");
  ASSERT_FALSE(uri_1.make_relative(uri_2).is_opaque());
}

TEST(uri_make_relative_test, simple_test_with_query) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/path/?key=value");
  ASSERT_EQ("/path/?key=value", uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, simple_test_with_fragment) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/path/#fragment");
  ASSERT_EQ("/path/#fragment", uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, make_relative_with_percent_encoding_normalization) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/%7E%66%6F%6F%62%61%72%5F%36%39/");
  ASSERT_EQ("/~foobar_69/", uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, make_relative_with_percent_encoding_normalization_with_query) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/%7E%66%6F%6F%62%61%72%5F%36%39/?key=value");
  ASSERT_EQ("/~foobar_69/?key=value", uri_1.make_relative(uri_2));
}

TEST(uri_make_relative_test, make_relative_with_percent_encoding_normalization_with_fragment) {
  network::uri uri_1("http://www.example.com/");
  network::uri uri_2("http://www.example.com/%7E%66%6F%6F%62%61%72%5F%36%39/#fragment");
  ASSERT_EQ("/~foobar_69/#fragment", uri_1.make_relative(uri_2));
}
