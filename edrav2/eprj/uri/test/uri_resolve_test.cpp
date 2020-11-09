// Copyright 2012-2016 Glyn Matthews.
// Copyright 2013 Hannes Kamecke.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <network/uri.hpp>
#include "string_utility.hpp"

using namespace network;

struct uri_resolve_test : public ::testing::Test {
  uri_resolve_test() : base_uri("http://a/b/c/d;p?q")
  {
  }

  uri::string_type resolved(const uri& base, const uri& reference) {
    return reference.resolve(base).string();
  }

  uri::string_type resolved(const uri& reference)
  {
    return resolved(base_uri, reference);
  }

  network::uri base_uri;
};


TEST_F(uri_resolve_test, is_absolute_uri__returns_other)
{
  ASSERT_EQ("https://www.example.com/", resolved(uri("https://www.example.com/")));
}

TEST_F(uri_resolve_test, base_has_empty_path__path_is_ref_path_1)
{
  uri reference = uri_builder().path("g").uri();
  ASSERT_EQ("http://a/g", resolved(uri("http://a/"), reference));
}

TEST_F(uri_resolve_test, base_has_empty_path__path_is_ref_path_2)
{
  uri reference = uri_builder().path("g/x/y").append_query("q").fragment("s").uri();
  ASSERT_EQ("http://a/g/x/y?q#s", resolved(uri("http://a/"), reference));
}

// normal examples
// http://tools.ietf.org/html/rfc3986#section-5.4.1

TEST_F(uri_resolve_test, remove_dot_segments1) {
  uri reference = uri_builder().path("./g").uri();
  ASSERT_EQ("http://a/b/c/g", resolved(reference));
}

TEST_F(uri_resolve_test, base_has_path__path_is_merged_1) {
  uri reference = uri_builder().path("g/").uri();
  ASSERT_EQ("http://a/b/c/g/", resolved(reference));
}

TEST_F(uri_resolve_test, base_has_path__path_is_merged_2) {
  uri reference = uri_builder().path("g").uri();
  ASSERT_EQ("http://a/b/c/g", resolved(reference));
}

TEST_F(uri_resolve_test, path_starts_with_slash__path_is_ref_path) {
  uri reference = uri_builder().path("/g").uri();
  ASSERT_EQ("http://a/g", resolved(reference));
}

TEST_F(uri_resolve_test, path_starts_with_slash_with_query_fragment__path_is_ref_path) {
  uri reference = uri_builder().path("/g/x").append_query("y").fragment("s").uri();
  ASSERT_EQ("http://a/g/x?y#s", resolved(reference));
}

TEST_F(uri_resolve_test, DISABLED_has_authority__base_scheme_with_ref_authority) {
  // ASSERT_EQ("http://g", resolved("//g"));
  // uri reference = uri_builder().host("g").path("").uri();
  uri reference = uri_builder().path("//g").uri();
  ASSERT_EQ("http://g", resolved(reference));
}

TEST_F(uri_resolve_test, path_is_empty_but_has_query__returns_base_with_ref_query) {
  uri reference = uri_builder().append_query("y").uri();
  ASSERT_EQ("http://a/b/c/d;p?y", resolved(reference));
}

TEST_F(uri_resolve_test, path_is_empty_but_has_query_base_no_query__returns_base_with_ref_query) {
  uri reference = uri_builder().append_query("y").uri();
  ASSERT_EQ("http://a/b/c/d?y", resolved(uri("http://a/b/c/d"), reference));
}

TEST_F(uri_resolve_test, merge_path_with_query) {
  uri reference = uri_builder().path("g").append_query("y").uri();
  ASSERT_EQ("http://a/b/c/g?y", resolved(reference));
}

TEST_F(uri_resolve_test, append_fragment) {
  uri reference = uri_builder().fragment("s").uri();
  ASSERT_EQ("http://a/b/c/d;p?q#s", resolved(reference));
}

TEST_F(uri_resolve_test, merge_paths_with_fragment) {
  uri reference = uri_builder().path("g").fragment("s").uri();
  ASSERT_EQ("http://a/b/c/g#s", resolved(reference));
}

TEST_F(uri_resolve_test, merge_paths_with_query_and_fragment) {
  uri reference = uri_builder().path("g").append_query("y").fragment("s").uri();
  ASSERT_EQ("http://a/b/c/g?y#s", resolved(reference));
}

TEST_F(uri_resolve_test, merge_paths_with_semicolon_1) {
  uri reference = uri_builder().path(";x").uri();
  ASSERT_EQ("http://a/b/c/;x", resolved(reference));
}

TEST_F(uri_resolve_test, merge_paths_with_semicolon_2) {
  uri reference = uri_builder().path("g;x").uri();
  ASSERT_EQ("http://a/b/c/g;x", resolved(reference));
}

TEST_F(uri_resolve_test, merge_paths_with_semicolon_3) {
  uri reference = uri_builder().path("g;x").append_query("y").fragment("s").uri();
  ASSERT_EQ("http://a/b/c/g;x?y#s", resolved(reference));
}

TEST_F(uri_resolve_test, path_is_empty__returns_base) {
  uri reference = uri_builder().uri();
  ASSERT_EQ("http://a/b/c/d;p?q", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments2) {
  uri reference = uri_builder().path(".").uri();
  ASSERT_EQ("http://a/b/c/", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments3) {
  uri reference = uri_builder().path("./").uri();
  ASSERT_EQ("http://a/b/c/", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments4) {
  uri reference = uri_builder().path("..").uri();
  ASSERT_EQ("http://a/b/", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments5) {
  uri reference = uri_builder().path("../").uri();
  ASSERT_EQ("http://a/b/", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments6) {
  uri reference = uri_builder().path("../g").uri();
  ASSERT_EQ("http://a/b/g", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments7) {
  uri reference = uri_builder().path("../..").uri();
  ASSERT_EQ("http://a/", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments8) {
  uri reference = uri_builder().path("../../").uri();
  ASSERT_EQ("http://a/", resolved(reference));
}

TEST_F(uri_resolve_test, remove_dot_segments9) {
  uri reference = uri_builder().path("../../g").uri();
  ASSERT_EQ("http://a/g", resolved(reference));
}


// abnormal examples
// http://tools.ietf.org/html/rfc3986#section-5.4.2

TEST_F(uri_resolve_test, abnormal_example_1) {
  uri reference = uri_builder().path("../../../g").uri();
  ASSERT_EQ("http://a/g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_2) {
  uri reference = uri_builder().path("../../../../g").uri();
  ASSERT_EQ("http://a/g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_3) {
  uri reference = uri_builder().path("/./g").uri();
  ASSERT_EQ("http://a/g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_4) {
  uri reference = uri_builder().path("/../g").uri();
  ASSERT_EQ("http://a/g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_5) {
  uri reference = uri_builder().path("g.").uri();
  ASSERT_EQ("http://a/b/c/g.", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_6) {
  uri reference = uri_builder().path(".g").uri();
  ASSERT_EQ("http://a/b/c/.g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_7) {
  uri reference = uri_builder().path("g..").uri();
  ASSERT_EQ("http://a/b/c/g..", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_8) {
  uri reference = uri_builder().path("..g").uri();
  ASSERT_EQ("http://a/b/g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_9) {
  uri reference = uri_builder().path("./../g").uri();
  ASSERT_EQ("http://a/b/g", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_10) {
  uri reference = uri_builder().path("./g/.").uri();
  ASSERT_EQ("http://a/b/c/g/", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_11) {
  uri reference = uri_builder().path("g/./h").uri();
  ASSERT_EQ("http://a/b/c/g/h", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_12) {
  uri reference = uri_builder().path("g/../h").uri();
  ASSERT_EQ("http://a/b/c/h", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_13) {
  uri reference = uri_builder().path("g;x=1/./y").uri();
  ASSERT_EQ("http://a/b/c/g;x=1/y", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_14) {
  uri reference = uri_builder().path("g;x=1/../y").uri();
  ASSERT_EQ("http://a/b/c/y", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_15) {
  uri reference = uri_builder().path("g").append_query("y/./x").uri();
  ASSERT_EQ("http://a/b/c/g?y/./x", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_16) {
  uri reference = uri_builder().path("g").append_query("y/../x").uri();
  ASSERT_EQ("http://a/b/c/g?y/../x", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_17) {
  uri reference = uri_builder().path("g").fragment("s/./x").uri();
  ASSERT_EQ("http://a/b/c/g#s/./x", resolved(reference));
}

TEST_F(uri_resolve_test, abnormal_example_18) {
  uri reference = uri_builder().path("g").fragment("s/../x").uri();
  ASSERT_EQ("http://a/b/c/g#s/../x", resolved(reference));
}

TEST_F(uri_resolve_test, issue_resolve_from_copy) {
  // https://github.com/cpp-netlib/uri/issues/15
  network::uri base("http://a.com/");
  network::uri uri("http:/example.com/path/");
  network::uri copy = uri;
  ASSERT_TRUE(copy.is_opaque());
  auto result = copy.resolve(base);
  ASSERT_EQ("http:/example.com/path/", result);
}

TEST_F(uri_resolve_test, issue_resolve_from_move) {
  // https://github.com/cpp-netlib/uri/issues/15
  network::uri base("http://a.com/");
  network::uri uri("http:/example.com/path/");
  network::uri copy = std::move(uri);
  ASSERT_TRUE(copy.is_opaque());
  auto result = copy.resolve(base);
  ASSERT_EQ("http:/example.com/path/", result);
}

TEST_F(uri_resolve_test, issue_15_resolve_from_copy_with_query) {
  // https://github.com/cpp-netlib/uri/issues/15
  network::uri base("http://a.com/");
  network::uri uri("http:/example.com/path/?query#fragment");
  network::uri copy = uri;
  ASSERT_TRUE(copy.is_opaque());
  auto result = copy.resolve(base);
  ASSERT_EQ("query", uri.query());
  ASSERT_EQ("query", copy.query());
  ASSERT_EQ("query", result.query());
}

TEST_F(uri_resolve_test, issue_15_resolve_from_copy_with_fragment) {
  // https://github.com/cpp-netlib/uri/issues/15
  network::uri base("http://a.com/");
  network::uri uri("http:/example.com/path/?query#fragment");
  network::uri copy = uri;
  ASSERT_TRUE(copy.is_opaque());
  auto result = copy.resolve(base);
  ASSERT_EQ("fragment", result.fragment());
}
