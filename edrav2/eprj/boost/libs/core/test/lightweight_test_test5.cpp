//
// Negative test for BOOST_TEST_TRAIT_SAME
//
// Copyright 2014, 2019 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/core/lightweight_test_trait.hpp>

struct X
{
    typedef int type;
};

template<class T1, class T2> struct Y
{
    typedef T1 type;
};

int main()
{
    BOOST_TEST_TRAIT_SAME(char[1], char[2]);
    BOOST_TEST_TRAIT_SAME(char[1], char[]);
    BOOST_TEST_TRAIT_SAME(char[1], char*);
    BOOST_TEST_TRAIT_SAME(void(), void(int));
    BOOST_TEST_TRAIT_SAME(void(), void(*)());
    BOOST_TEST_TRAIT_SAME(X, void);
    BOOST_TEST_TRAIT_SAME(X::type, void);
    BOOST_TEST_TRAIT_SAME(X, Y<void, void>);
    BOOST_TEST_TRAIT_SAME(X::type, Y<float, int>::type);
    BOOST_TEST_TRAIT_SAME(Y<int, float>, Y<int, double>);

    return boost::report_errors() == 10;
}
