// Copyright 2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt of copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <gtest/gtest.h>
#include "test_uri.hpp"
#include "string_utility.hpp"

TEST(uri_parse_test, test_empty_uri) {
  test::uri uri("");
  EXPECT_FALSE(uri.parse_uri());
  EXPECT_TRUE(uri.parsed_till().empty());
}

TEST(uri_parse_test, test_hierarchical_part_valid_user_info) {
  test::uri uri("http://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
}

TEST(uri_parse_test, test_hierarchical_part_empty_user_info) {
  test::uri uri("http://@www.example.com:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
}

TEST(uri_parse_test, test_hierarchical_part_unset_user_info) {
  test::uri uri("http://www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
}

TEST(uri_parse_test, test_hierarchical_part_unset_user_info_and_host) {
  test::uri uri("http://:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_FALSE(uri.has_host());
}

TEST(uri_parse_test, test_hierarchical_part_valid_user_info_and_host) {
  test::uri uri("http://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
}

TEST(uri_parse_test, test_hierarchical_part_valid_user_info_unset_host) {
  test::uri uri("http://user@:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
  ASSERT_FALSE(uri.has_host());
}

TEST(uri_parse_test, test_hierarchical_part_valid_user_info_host_and_port) {
  test::uri uri("http://user@www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_TRUE(uri.has_port());
  EXPECT_EQ("80", uri.port());
}

TEST(uri_parse_test, test_hierarchical_part_empty_user_info_valid_host_and_port) {
  test::uri uri("http://www.example.com:80/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_TRUE(uri.has_port());
  EXPECT_EQ("80", uri.port());
}

TEST(uri_parse_test, test_hierarchical_part_empty_user_info_valid_host_empty_port) {
  test::uri uri("http://www.example.com/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
}

TEST(uri_parse_test, test_hierarchical_part_valid_user_info_and_host_empty_port) {
  test::uri uri("http://user@www.example.com/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
}

TEST(uri_parse_test, test_hierarchical_part_valid_user_info_empty_host_valid_port) {
  test::uri uri("http://user@:80/path?query#fragment");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_empty_port_empty_path) {
  test::uri uri("http://www.example.com");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  ASSERT_TRUE(uri.path().empty());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_valid_port_empty_path) {
  test::uri uri("http://www.example.com:80");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_TRUE(uri.has_port());
  EXPECT_EQ("80", uri.port());
  ASSERT_TRUE(uri.has_path());
  ASSERT_TRUE(uri.path().empty());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_port_path) {
  test::uri uri("http://www.example.com:80/path");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_TRUE(uri.has_port());
  EXPECT_EQ("80", uri.port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/path", uri.path());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_path) {
  test::uri uri("http://www.example.com/path");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/path", uri.path());
}

TEST(uri_parse_test, test_hierarchical_part_with_opaque_uri) {
  test::uri uri("mailto:user@example.com");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("mailto", uri.scheme());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_FALSE(uri.has_host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("user@example.com", uri.path());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_path_and_query) {
  test::uri uri("http://www.example.com/path?query");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/path", uri.path());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_path_query_and_fragment) {
  test::uri uri("http://www.example.com/path?query#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/path", uri.path());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query", uri.query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment", uri.fragment());
}

TEST(uri_parse_test, test_hierarchical_part_valid_host_path_and_fragment) {
  test::uri uri("http://www.example.com/path#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/path", uri.path());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment", uri.fragment());
}

TEST(uri_parse_test, test_invalid_fragment) {
  test::uri uri("http://www.example.com/path#%fragment");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_valid_fragment_with_pct_encoded_char) {
  test::uri uri("http://www.example.com/path#%ffragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("%ffragment", uri.fragment());
}

TEST(uri_parse_test, test_valid_fragment_with_unreserved_char) {
  test::uri uri("http://www.example.com/path#fragment-");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment-", uri.fragment());
}

TEST(uri_parse_test, test_invalid_fragment_with_gen_delim) {
  test::uri uri("http://www.example.com/path#frag#ment");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_valid_fragment_with_sub_delim) {
  test::uri uri("http://www.example.com/path#frag$ment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("frag$ment", uri.fragment());
}

TEST(uri_parse_test, test_valid_fragment_with_forward_slash_and_question_mark) {
  test::uri uri("http://www.example.com/path#frag/ment?");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("frag/ment?", uri.fragment());
}

TEST(uri_parse_test, test_invalid_query) {
  test::uri uri("http://www.example.com/path?%query");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_valid_query_with_pct_encoded_char) {
  test::uri uri("http://www.example.com/path?%00query");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("%00query", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_valid_query_with_unreserved_char) {
  test::uri uri("http://www.example.com/path?query-");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query-", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_valid_query_with_sub_delim) {
  test::uri uri("http://www.example.com/path?qu$ery");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("qu$ery", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_invalid_port_with_path) {
  test::uri uri("http://123.34.23.56:6662626/");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_invalid_port) {
  test::uri uri("http://123.34.23.56:6662626");
  EXPECT_FALSE(uri.parse_uri());
}

TEST(uri_parse_test, test_empty_port_with_path) {
  test::uri uri("http://123.34.23.56:/");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_port());
  ASSERT_EQ("", uri.port());
}

TEST(uri_parse_test, test_empty_port) {
  test::uri uri("http://123.34.23.56:");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_port());
  ASSERT_EQ("", uri.port());
}

TEST(uri_parse_test, test_ipv6_address) {
  test::uri uri("http://[1080:0:0:0:8:800:200C:417A]");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("[1080:0:0:0:8:800:200C:417A]", uri.host());
}

TEST(uri_parse_test, test_ipv6_address_with_path) {
  test::uri uri("http://[1080:0:0:0:8:800:200C:417A]/");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("[1080:0:0:0:8:800:200C:417A]", uri.host());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/", uri.path());
}

TEST(uri_parse_test, test_invalid_ipv6_address) {
  test::uri uri("http://[1080:0:0:0:8:800:200C:417A");
  EXPECT_FALSE(!uri.parse_uri());
}

TEST(uri_parse_test, test_invalid_ipv6_address_with_path) {
  test::uri uri("http://[1080:0:0:0:8:800:200C:417A/");
  EXPECT_FALSE(!uri.parse_uri());
}

TEST(uri_parse_test, test_opaque_uri_with_one_slash) {
  test::uri uri("scheme:/path/");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("scheme", uri.scheme());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/path/", uri.path());
}

TEST(uri_parse_test, test_query_with_empty_path) {
  test::uri uri("http://www.example.com?query");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_query_with_user_info_and_empty_path) {
  test::uri uri("http://user@www.example.com?query");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_fragment_with_empty_path) {
  test::uri uri("http://www.example.com#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment", uri.fragment());
}

TEST(uri_parse_test, test_fragment_with_user_info_and_empty_path) {
  test::uri uri("http://user@www.example.com#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment", uri.fragment());
}

TEST(uri_parse_test, test_query_with_empty_path_and_ipv6_address) {
  test::uri uri("http://[1080:0:0:0:8:800:200C:417A]?query");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("[1080:0:0:0:8:800:200C:417A]", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_query_with_user_info_empty_path_and_ipv6_address) {
  test::uri uri("http://user@[1080:0:0:0:8:800:200C:417A]?query");
  EXPECT_TRUE(uri.parse_uri());
  EXPECT_EQ("http://user@[1080:0:0:0:8:800:200C:417A]?query", uri.parsed_till());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("[1080:0:0:0:8:800:200C:417A]", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_TRUE(uri.has_query());
  EXPECT_EQ("query", uri.query());
  ASSERT_FALSE(uri.has_fragment());
}

TEST(uri_parse_test, test_fragment_with_empty_path_and_ipv6_address) {
  test::uri uri("http://[1080:0:0:0:8:800:200C:417A]#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_FALSE(uri.has_user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("[1080:0:0:0:8:800:200C:417A]", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment", uri.fragment());
}

TEST(uri_parse_test, test_fragment_with_user_info_empty_path_and_ipv6_address) {
  test::uri uri("http://user@[1080:0:0:0:8:800:200C:417A]#fragment");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("[1080:0:0:0:8:800:200C:417A]", uri.host());
  ASSERT_FALSE(uri.has_port());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("", uri.path());
  ASSERT_FALSE(uri.has_query());
  ASSERT_TRUE(uri.has_fragment());
  EXPECT_EQ("fragment", uri.fragment());
}

TEST(uri_parse_test, test_pct_encoded_user_info) {
  test::uri uri("http://user%3f@www.example.com/");
  EXPECT_TRUE(uri.parse_uri());
  ASSERT_TRUE(uri.has_scheme());
  EXPECT_EQ("http", uri.scheme());
  ASSERT_TRUE(uri.has_user_info());
  EXPECT_EQ("user%3f", uri.user_info());
  ASSERT_TRUE(uri.has_host());
  EXPECT_EQ("www.example.com", uri.host());
  ASSERT_TRUE(uri.has_path());
  EXPECT_EQ("/", uri.path());
}

// http://formvalidation.io/validators/uri/

std::vector<std::string> create_urls(const std::string &filename) {
  std::vector<std::string> urls;
  std::ifstream ifs(filename);
  std::cout << filename << std::endl;
  if (!ifs) {
    std::cout << "Shit." << std::endl;
    throw std::runtime_error("Unable to open file: " + filename);
  }
  for (std::string url; std::getline(ifs, url);) {
    if (url.front() != '#') {
      urls.push_back(url);
    }
  }
  return urls;
}

// All valid URLs in the list should pass
class test_valid_urls : public ::testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_CASE_P(uri_parse_test, test_valid_urls,
                        testing::ValuesIn(create_urls("valid_urls.txt")));

TEST_P(test_valid_urls, urls_are_valid) {
  test::uri uri(GetParam());
  EXPECT_TRUE(uri.parse_uri());
}
