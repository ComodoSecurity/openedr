// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/core/lightweight_test_trait.hpp>
#include <boost/histogram/axis/category.hpp>
#include <boost/histogram/axis/integer.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/mp11.hpp>
#include "utility_axis.hpp"

using namespace boost::histogram::axis;

int main() {
  {
    auto a = integer<>(1, 3);
    BOOST_TEST_EQ(traits::index(a, 1), 0);
    BOOST_TEST_EQ(traits::value(a, 0), 1);
    BOOST_TEST_EQ(traits::width(a, 0), 0);
    BOOST_TEST_EQ(traits::width(a, 0), 0);

    auto b = integer<double>(1, 3);
    BOOST_TEST_EQ(traits::index(b, 1), 0);
    BOOST_TEST_EQ(traits::value(b, 0), 1);
    BOOST_TEST_EQ(traits::width(b, 0), 1);
    auto& b1 = b;
    BOOST_TEST(traits::static_options<decltype(b1)>::test(option::underflow));
    const auto& b2 = b;
    BOOST_TEST(traits::static_options<decltype(b2)>::test(option::underflow));

    auto c = category<std::string>{"red", "blue"};
    BOOST_TEST_EQ(traits::index(c, "blue"), 1);
    BOOST_TEST_EQ(traits::value(c, 0), std::string("red"));
    BOOST_TEST_EQ(traits::width(c, 0), 0);
  }

  {
    using A = integer<>;
    BOOST_TEST_EQ(traits::static_options<A>::test(option::growth), false);
    BOOST_TEST_EQ(traits::static_options<A&>::test(option::growth), false);
    BOOST_TEST_EQ(traits::static_options<const A&>::test(option::growth), false);
    auto expected = option::underflow | option::overflow;
    auto a = A{};
    BOOST_TEST_EQ(traits::options(a), expected);
    BOOST_TEST_EQ(traits::options(static_cast<A&>(a)), expected);
    BOOST_TEST_EQ(traits::options(static_cast<const A&>(a)), expected);
    BOOST_TEST_EQ(traits::options(std::move(a)), expected);

    using B = integer<int, null_type, option::growth_t>;
    BOOST_TEST_EQ(traits::static_options<B>::test(option::growth), true);
    BOOST_TEST_EQ(traits::static_options<B&>::test(option::growth), true);
    BOOST_TEST_EQ(traits::static_options<const B&>::test(option::growth), true);
    BOOST_TEST_EQ(traits::options(B{}), option::growth);

    struct growing {
      auto update(double) { return std::make_pair(0, 0); }
    };
    using C = growing;
    BOOST_TEST_EQ(traits::static_options<C>::test(option::growth), true);
    BOOST_TEST_EQ(traits::static_options<C&>::test(option::growth), true);
    BOOST_TEST_EQ(traits::static_options<const C&>::test(option::growth), true);
    auto c = C{};
    BOOST_TEST_EQ(traits::options(c), option::growth);
    BOOST_TEST_EQ(traits::options(static_cast<C&>(c)), option::growth);
    BOOST_TEST_EQ(traits::options(static_cast<const C&>(c)), option::growth);
    BOOST_TEST_EQ(traits::options(std::move(c)), option::growth);

    struct notgrowing {
      auto index(double) { return 0; }
    };
    using D = notgrowing;
    BOOST_TEST_EQ(traits::static_options<D>::test(option::growth), false);
    BOOST_TEST_EQ(traits::static_options<D&>::test(option::growth), false);
    BOOST_TEST_EQ(traits::static_options<const D&>::test(option::growth), false);
    auto d = D{};
    BOOST_TEST_EQ(traits::options(d), option::none);
    BOOST_TEST_EQ(traits::options(static_cast<D&>(d)), option::none);
    BOOST_TEST_EQ(traits::options(static_cast<const D&>(d)), option::none);
    BOOST_TEST_EQ(traits::options(std::move(d)), option::none);
  }

  {
    auto a = integer<int, null_type, option::growth_t>();
    BOOST_TEST_EQ(traits::update(a, 0), (std::pair<index_type, index_type>(0, -1)));
    BOOST_TEST_THROWS(traits::update(a, "foo"), std::invalid_argument);
  }

  {
    struct None {};

    struct Const {
      const int& metadata() const { return m; };
      int m = 0;
    };

    struct Both {
      const int& metadata() const { return m; };
      int& metadata() { return m; };
      int m = 0;
    };

    None none;
    BOOST_TEST_TRAIT_SAME(decltype(traits::metadata(none)), null_type&);
    BOOST_TEST_TRAIT_SAME(decltype(traits::metadata(static_cast<None&>(none))),
                          null_type&);
    BOOST_TEST_TRAIT_SAME(decltype(traits::metadata(static_cast<const None&>(none))),
                          const null_type&);

    Const c;
    BOOST_TEST_EQ(traits::metadata(c), 0);
    BOOST_TEST_EQ(traits::metadata(static_cast<Const&>(c)), 0);
    BOOST_TEST_EQ(traits::metadata(static_cast<const Const&>(c)), 0);

    Both b;
    BOOST_TEST_EQ(traits::metadata(b), 0);
    BOOST_TEST_EQ(traits::metadata(static_cast<Both&>(b)), 0);
    BOOST_TEST_EQ(traits::metadata(static_cast<const Both&>(b)), 0);
  }

  return boost::report_errors();
}
