// Copyright (c) Glyn Matthews 2012-2016.
// Copyright 2012 Dean Michael Berris <dberris@google.com>
// Copyright 2012 Google, Inc.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file
 * \brief Contains the definition of the uri_builder.
 */

#ifndef NETWORK_URI_BUILDER_INC
#define NETWORK_URI_BUILDER_INC

#include <cstdint>
#include <utility>
#include <type_traits>
#include <network/uri/uri.hpp>

#ifdef NETWORK_URI_MSVC
#pragma warning(push)
#pragma warning(disable : 4251 4231 4660)
#endif

namespace network {
#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
namespace detail {

template <class T>
struct host_converter {
  uri::string_type operator()(const T &host) const {
    return detail::translate(host);
  }
};

template <class T, class Enable = void>
struct port_converter {
  uri::string_type operator()(const T &port) const {
    return detail::translate(port);
  }
};

template <class T>
struct port_converter<T, typename std::enable_if<std::is_integral<
                             typename std::decay<T>::type>::value>::type> {
  uri::string_type operator()(std::uint16_t port) const {
    return std::to_string(port);
  }
};

template <class T>
struct path_converter {
  uri::string_type operator()(const T &path) const {
    return detail::translate(path);
  }
};
}  // namespace detail
#endif  // !defined(DOXYGEN_SHOULD_SKIP_THIS)

/**
 * \ingroup uri
 * \class uri_builder network/uri/uri_builder.hpp network/uri.hpp
 * \brief A class that allows complex uri objects to be constructed.
 * \sa uri
 */
class uri_builder {
#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
  friend class uri;
#endif  // !defined(DOXYGEN_SHOULD_SKIP_THIS)

  uri_builder(const uri_builder &) = delete;
  uri_builder &operator=(const uri_builder &) = delete;

 public:
  /**
   * \brief The uri_builder string_type.
   */
  using string_type = network::uri::string_type;

  /**
   * \brief Constructor.
   */
  uri_builder() = default;

  /**
   * \brief Constructor.
   * \param base A URI that is the base on which a new URI is built.
   */
  explicit uri_builder(const uri &base);

  /**
   * \brief Destructor.
   */
  ~uri_builder() noexcept;

  /**
   * \brief Adds a new scheme to the uri_builder.
   * \param scheme The scheme.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &scheme(const Source &scheme) {
    set_scheme(detail::translate(scheme));
    return *this;
  }

  /**
   * \brief Adds a new user info to the uri_builder.
   * \param user_info The user info.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &user_info(const Source &user_info) {
    set_user_info(detail::translate(user_info));
    return *this;
  }

  /**
   * \brief Clears the URI user_info part.
   * \returns \c *this
   */
  uri_builder &clear_user_info();

  /**
   * \brief Adds a new host to the uri_builder.
   * \param host The host.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &host(const Source &host) {
    detail::host_converter<Source> convert;
    set_host(convert(host));
    return *this;
  }

  /**
   * \brief Adds a new port to the uri_builder.
   * \param port The port.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &port(const Source &port) {
    detail::port_converter<Source> convert;
    set_port(convert(port));
    return *this;
  }

  /**
   * \brief Clears the URI port part.
   * \returns \c *this
   */
  uri_builder &clear_port();

  /**
   * \brief Adds a new authority to the uri_builder.
   * \param authority The authority.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &authority(const Source &authority) {
    set_authority(detail::translate(authority));
    return *this;
  }

  /**
   * \brief Adds a new path to the uri_builder.
   * \param path The path.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &path(const Source &path) {
    detail::path_converter<Source> convert;
    set_path(convert(path));
    return *this;
  }

  /**
   * \brief Clears the URI path part.
   * \returns \c *this
   */
  uri_builder &clear_path();

  /**
   * \brief Adds a new query to the uri_builder.
   * \param query The query.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &append_query(const Source &query) {
    append_query(detail::translate(query));
    return *this;
  }

  /**
   * \brief Clears the URI query part.
   * \returns \c *this
   */
  uri_builder &clear_query();

  /**
   * \brief Adds a new query to the uri_builder.
   * \param key The query key.
   * \param value The query value.
   * \returns \c *this
   */
  template <typename Key, typename Value>
    uri_builder &append_query_key_value_pair(const Key &key, const Value &value) {
    if (!query_) {
      query_ = string_type();
    }
    else {
      query_->append("&");
    }
    string_type query_pair = detail::translate(key) + "=" + detail::translate(value);
    network::uri::encode_query(std::begin(query_pair), std::end(query_pair),
                               std::back_inserter(*query_));
    return *this;
  }

  /**
   * \brief Adds a new fragment to the uri_builder.
   * \param fragment The fragment.
   * \returns \c *this
   */
  template <typename Source>
  uri_builder &fragment(const Source &fragment) {
    set_fragment(detail::translate(fragment));
    return *this;
  }

  /**
   * \brief Clears the URI fragment part.
   * \returns \c *this
   */
  uri_builder &clear_fragment();

  /**
   * \brief Builds a new uri object.
   * \returns A valid uri object.
   * \throws uri_builder_error if the uri_builder is unable to build
   *         a valid URI.
   * \throws std::bad_alloc If the underlying string cannot be
   *         allocated.
   */
  network::uri uri() const;

 private:
  void set_scheme(string_type scheme);
  void set_user_info(string_type user_info);
  void set_host(string_type host);
  void set_port(string_type port);
  void set_authority(string_type authority);
  void set_path(string_type path);
  void append_query(string_type query);
  void set_fragment(string_type fragment);

  optional<string_type> scheme_, user_info_, host_, port_, path_, query_,
      fragment_;
};
}  // namespace network

#ifdef NETWORK_URI_MSVC
#pragma warning(pop)
#endif

#endif  // NETWORK_URI_BUILDER_INC
