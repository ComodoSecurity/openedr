// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt of copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <network/uri.hpp>
#include <sstream>

TEST(uri_stream_test, ostream) {
  std::ostringstream oss;
  oss << network::uri("http://www.example.com/");
  ASSERT_EQ("http://www.example.com/", oss.str());
}

TEST(uri_stream_test, wostream) {
  std::wostringstream oss;
  oss << network::uri("http://www.example.com/");
  ASSERT_EQ(L"http://www.example.com/", oss.str());
}

TEST(uri_stream_test, istream) {
  std::istringstream iss("http://www.example.com/");
  network::uri instance;
  iss >> instance;
  ASSERT_EQ("http://www.example.com/", instance);
}

TEST(uri_stream_test, wistream) {
  std::wistringstream iss(L"http://www.example.com/");
  network::uri instance;
  iss >> instance;
  ASSERT_EQ("http://www.example.com/", instance);
}

TEST(uri_stream_test, DISABLED_istream_invalid_uri) {
  std::istringstream iss("I am not a valid URI.");
  network::uri instance;
  ASSERT_THROW((iss >> instance), network::uri_syntax_error);
}

TEST(uri_stream_test, DISABLED_wistream_invalid_uri) {
  std::wistringstream iss(L"I am not a valid URI.");
  network::uri instance;
  ASSERT_THROW((iss >> instance), network::uri_syntax_error);
}

// This is not the full story with istream and exceptions...
