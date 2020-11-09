// Module:  Log4CPLUS
// File:    stringhelper.cxx
// Created: 4/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2017 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/streams.h>
#include <log4cplus/internal/internal.h>

#include <iterator>
#include <algorithm>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cctype>
#include <cassert>

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#include <catch.hpp>
#include <limits>
#endif


namespace log4cplus
{

namespace internal
{

log4cplus::tstring const empty_str;

} // namespace internal

} // namespace log4cplus


//////////////////////////////////////////////////////////////////////////////
// Global Methods
//////////////////////////////////////////////////////////////////////////////

#if defined (UNICODE) && defined (LOG4CPLUS_ENABLE_GLOBAL_C_STRING_STREAM_INSERTER)

log4cplus::tostream&
operator <<(log4cplus::tostream& stream, const char* str)
{
    return (stream << log4cplus::helpers::towstring(str));
}

#endif


namespace log4cplus
{

namespace helpers
{


void
clear_mbstate (std::mbstate_t & mbs)
{
    // Initialize/clear mbstate_t type.
    // XXX: This is just a hack that works. The shape of mbstate_t varies
    // from single unsigned to char[128]. Without some sort of initialization
    // the codecvt::in/out methods randomly fail because the initial state is
    // random/invalid.
    std::memset (&mbs, 0, sizeof (std::mbstate_t));
}


#if defined (LOG4CPLUS_POOR_MANS_CHCONV)

static
void
tostring_internal (std::string & ret, wchar_t const * src, std::size_t size)
{
    ret.resize(size);
    for (std::size_t i = 0; i < size; ++i)
    {
        std::char_traits<wchar_t>::int_type src_int
            = std::char_traits<wchar_t>::to_int_type (src[i]);
        ret[i] = src_int <= 127
            ? std::char_traits<char>::to_char_type (src_int)
            : '?';
    }
}


std::string
tostring(const std::wstring& src)
{
    std::string ret;
    tostring_internal (ret, src.c_str (), src.size ());
    return ret;
}


std::string
tostring(wchar_t const * src)
{
    assert (src);
    std::string ret;
    tostring_internal (ret, src, std::wcslen (src));
    return ret;
}


static
void
towstring_internal (std::wstring & ret, char const * src, std::size_t size)
{
    ret.resize(size);
    for (std::size_t i = 0; i < size; ++i)
    {
        std::char_traits<char>::int_type src_int
            = std::char_traits<char>::to_int_type (src[i]);
        ret[i] = src_int <= 127
            ? std::char_traits<wchar_t>::to_char_type (src_int)
            : L'?';
    }
}


std::wstring
towstring(const std::string& src)
{
    std::wstring ret;
    towstring_internal (ret, src.c_str (), src.size ());
    return ret;
}


std::wstring
towstring(char const * src)
{
    assert (src);
    std::wstring ret;
    towstring_internal (ret, src, std::strlen (src));
    return ret;
}

#endif // LOG4CPLUS_POOR_MANS_CHCONV


namespace
{


struct toupper_func
{
    tchar
    operator () (tchar ch) const
    {
        return std::char_traits<tchar>::to_char_type (
#ifdef UNICODE
            std::towupper
#else
            std::toupper
#endif
            (std::char_traits<tchar>::to_int_type (ch)));
    }
};


struct tolower_func
{
    tchar
    operator () (tchar ch) const
    {
        return std::char_traits<tchar>::to_char_type (
#ifdef UNICODE
            std::towlower
#else
            std::tolower
#endif
            (std::char_traits<tchar>::to_int_type (ch)));
    }
};


} // namespace


tchar
toUpper (tchar ch)
{
    return toupper_func () (ch);
}


tstring
toUpper(const tstring& s)
{
    tstring ret;
    std::transform(s.begin(), s.end(), std::back_inserter (ret),
        toupper_func ());
    return ret;
}


tchar
toLower (tchar ch)
{
    return tolower_func () (ch);
}


tstring
toLower(const tstring& s)
{
    tstring ret;
    std::transform(s.begin(), s.end(), std::back_inserter (ret),
        tolower_func ());
    return ret;
}


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)

namespace
{

template <typename IntType>
struct test
{
    using limits = std::numeric_limits<IntType>;

    static void run_one (IntType value = 123)
    {
        tostringstream oss;
        oss.imbue (std::locale ("C"));
        oss << +value;

        CATCH_REQUIRE (convertIntegerToString (value) == oss.str ());
    }

    static void run ()
    {
        test<IntType>::run_one ();
        test<IntType>::run_one (limits::min ());
        test<IntType>::run_one (limits::max ());
    }
};

} // namespace


CATCH_TEST_CASE( "Strings helpers", "[strings]" )
{
    CATCH_SECTION ("empty_str is empty")
    {
        CATCH_REQUIRE (internal::empty_str.empty ());
    }

    CATCH_SECTION ("tokenize")
    {
        std::vector<tstring> tokens;

        CATCH_SECTION ("collapse tokens")
        {
            CATCH_SECTION ("1")
            {
                tstring const str = LOG4CPLUS_TEXT ("1");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens));
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,2")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,2");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,2,3")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,2,3");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("3") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens));
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,,2,,,3")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,,2,,,3");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("3") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens));
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,,2,,,3,")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,,2,,,3,");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("3") } };
                tokenize (str, LOG4CPLUS_TEXT (','), std::back_inserter (tokens));
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,,2,,,3,,")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,,2,,,3,,");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("3") } };
                tokenize (str, LOG4CPLUS_TEXT (','), std::back_inserter (tokens));
                CATCH_REQUIRE (tokens == expected_tokens);
            }
        }

        CATCH_SECTION ("do not collapse tokens")
        {
            CATCH_SECTION ("1")
            {
                tstring const str = LOG4CPLUS_TEXT ("1");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens));
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,2")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,2");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,2,3")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,2,3");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("3") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION ("1,,2,,,3")
            {
                tstring const str = LOG4CPLUS_TEXT ("1,,2,,,3");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("3") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }


            CATCH_SECTION (",1,,2,,,3,")
            {
                tstring const str = LOG4CPLUS_TEXT (",1,,2,,,3,");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("3") },
                    { LOG4CPLUS_TEXT ("") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION (",1,,2,,,3,,")
            {
                tstring const str = LOG4CPLUS_TEXT (",1,,2,,,3,,");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("3") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }

            CATCH_SECTION (",,1,,2,,,3,,")
            {
                tstring const str = LOG4CPLUS_TEXT (",,1,,2,,,3,,");
                std::vector<tstring> const expected_tokens = {
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("1") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("2") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("3") },
                    { LOG4CPLUS_TEXT ("") },
                    { LOG4CPLUS_TEXT ("") } };
                tokenize (str, LOG4CPLUS_TEXT (','),
                    std::back_inserter (tokens), false);
                CATCH_REQUIRE (tokens == expected_tokens);
            }
        }
    }

    CATCH_SECTION ("convert integer to string")
    {

#define LOG4CPLUS_GEN_TEST(TYPE)                \
        CATCH_SECTION (#TYPE)                   \
        {                                       \
            test<TYPE>::run ();                 \
        }

        LOG4CPLUS_GEN_TEST (char)
        LOG4CPLUS_GEN_TEST (unsigned char)
        LOG4CPLUS_GEN_TEST (signed char)
        LOG4CPLUS_GEN_TEST (short)
        LOG4CPLUS_GEN_TEST (unsigned short)
        LOG4CPLUS_GEN_TEST (int)
        LOG4CPLUS_GEN_TEST (unsigned int)
        LOG4CPLUS_GEN_TEST (long)
        LOG4CPLUS_GEN_TEST (unsigned long)
        LOG4CPLUS_GEN_TEST (long long)
        LOG4CPLUS_GEN_TEST (unsigned long long)
#undef LOG4CPLUS_GEN_TEST
    }

    CATCH_SECTION ("join strings")
    {
        tstring const items[] = {
            { LOG4CPLUS_TEXT ("1") },
            { LOG4CPLUS_TEXT ("2") },
            { LOG4CPLUS_TEXT ("3") } };
        tstring result;

        CATCH_SECTION ("1")
        {
            helpers::join (result, std::begin (items), items + 1,
                LOG4CPLUS_TEXT (','));
            CATCH_REQUIRE (result == LOG4CPLUS_TEXT ("1"));
        }

        CATCH_SECTION ("1,2")
        {
            helpers::join (result, std::begin (items), items + 2,
                LOG4CPLUS_TEXT (","));
            CATCH_REQUIRE (result == LOG4CPLUS_TEXT ("1,2"));
        }

        CATCH_SECTION ("1,2,3")
        {
            helpers::join (result, std::begin (items), items + 3,
                LOG4CPLUS_TEXT (','));
            CATCH_REQUIRE (result == LOG4CPLUS_TEXT ("1,2,3"));
        }
    }
}
#endif



} // namespace helpers

} // namespace log4cplus
