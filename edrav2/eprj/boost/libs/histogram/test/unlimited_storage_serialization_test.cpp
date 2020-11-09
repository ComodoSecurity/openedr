// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/histogram/serialization.hpp>
#include <boost/histogram/unlimited_storage.hpp>
#include <memory>
#include <sstream>

using unlimited_storage_type = boost::histogram::unlimited_storage<>;

using namespace boost::histogram;

template <typename T>
unlimited_storage_type prepare(std::size_t n, const T x) {
  std::unique_ptr<T[]> v(new T[n]);
  std::fill(v.get(), v.get() + n, static_cast<T>(0));
  v.get()[0] = x;
  return unlimited_storage_type(n, v.get());
}

template <typename T>
unlimited_storage_type prepare(std::size_t n) {
  return unlimited_storage_type(n, static_cast<T*>(nullptr));
}

template <typename T>
void serialization_impl() {
  const auto a = prepare(1, T(1));
  std::string buf;
  {
    std::ostringstream os;
    boost::archive::text_oarchive oa(os);
    oa << a;
    buf = os.str();
  }
  unlimited_storage_type b;
  BOOST_TEST(!(a == b));
  {
    std::istringstream is(buf);
    boost::archive::text_iarchive ia(is);
    ia >> b;
  }
  BOOST_TEST(a == b);
}

int main() {
  // serialization_test
  {
    serialization_impl<uint8_t>();
    serialization_impl<uint16_t>();
    serialization_impl<uint32_t>();
    serialization_impl<uint64_t>();
    serialization_impl<unlimited_storage_type::mp_int>();
    serialization_impl<double>();
  }

  return boost::report_errors();
}
