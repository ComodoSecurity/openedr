// Copyright (c) Glyn Matthews 2012-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <locale>
#include "network/uri/uri_builder.hpp"
#include "detail/uri_normalize.hpp"
#include "detail/uri_parse_authority.hpp"
#include "detail/algorithm.hpp"

namespace network {
uri_builder::uri_builder(const network::uri &base_uri) {
  if (base_uri.has_scheme()) {
    set_scheme(base_uri.scheme().to_string());
  }

  if (base_uri.has_user_info()) {
    set_user_info(base_uri.user_info().to_string());
  }

  if (base_uri.has_host()) {
    set_host(base_uri.host().to_string());
  }

  if (base_uri.has_port()) {
    set_port(base_uri.port().to_string());
  }

  if (base_uri.has_path()) {
    set_path(base_uri.path().to_string());
  }

  if (base_uri.has_query()) {
    append_query(base_uri.query().to_string());
  }

  if (base_uri.has_fragment()) {
    set_fragment(base_uri.fragment().to_string());
  }
}

uri_builder::~uri_builder() noexcept {}

network::uri uri_builder::uri() const { return network::uri(*this); }

void uri_builder::set_scheme(string_type scheme) {
  // validate scheme is valid and normalize
  scheme_ = scheme;
  detail::transform(*scheme_, std::begin(*scheme_),
                    [] (char ch) { return std::tolower(ch, std::locale()); });
}

void uri_builder::set_user_info(string_type user_info) {
  user_info_ = string_type();
  network::uri::encode_user_info(std::begin(user_info), std::end(user_info),
                                 std::back_inserter(*user_info_));
}

uri_builder &uri_builder::clear_user_info() {
  user_info_ = network::nullopt;
  return *this;
}

void uri_builder::set_host(string_type host) {
  host_ = string_type();
  network::uri::encode_host(std::begin(host), std::end(host),
                            std::back_inserter(*host_));
  detail::transform(*host_, std::begin(*host_),
                    [](char ch) { return std::tolower(ch, std::locale()); });
}

void uri_builder::set_port(string_type port) {
  port_ = string_type();
  network::uri::encode_port(std::begin(port), std::end(port),
                            std::back_inserter(*port_));
}

uri_builder &uri_builder::clear_port() {
  port_ = network::nullopt;
  return *this;
}

void uri_builder::set_authority(string_type authority) {
  optional<detail::uri_part> user_info, host, port;
  uri::string_view view(authority);
  uri::const_iterator it = std::begin(view), last = std::end(view);
  detail::parse_authority(it, last, user_info, host, port);

  if (user_info) {
    set_user_info(user_info->to_string());
  }

  if (host) {
    set_host(host->to_string());
  }

  if (port) {
    set_port(port->to_string());
  }
}

void uri_builder::set_path(string_type path) {
  path_ = string_type();
  network::uri::encode_path(std::begin(path), std::end(path),
                            std::back_inserter(*path_));
}

uri_builder &uri_builder::clear_path() {
  path_ = network::nullopt;
  return *this;
}

void uri_builder::append_query(string_type query) {
  if (!query_) {
    query_ = string_type();
  }
  else {
    query_->append("&");
  }
  network::uri::encode_query(std::begin(query), std::end(query),
                             std::back_inserter(*query_));
}

uri_builder &uri_builder::clear_query() {
  query_ = network::nullopt;
  return *this;
}

void uri_builder::set_fragment(string_type fragment) {
  fragment_ = string_type();
  network::uri::encode_fragment(std::begin(fragment), std::end(fragment),
                                std::back_inserter(*fragment_));
}

uri_builder &uri_builder::clear_fragment() {
  fragment_ = network::nullopt;
  return *this;
}
}  // namespace network
