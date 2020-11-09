//
// edrav2.ut_libcore project
//
// Author:
// Reviewer:
//
#include "pch.h"

using namespace openEdr;

//
//
//
TEST_CASE("string.utf8_encoding_valid")
{
#pragma warning( push ) 
#pragma warning( disable : 4566 )

#define UTF8_TEST_STRING u8"ABCァD漢字фыва"
#define WCHAR_TEST_STRING L"ABCァD漢字фыва"

	std::string sUTF8 = UTF8_TEST_STRING;
	REQUIRE(string::convertUtf8ToWChar(sUTF8) == WCHAR_TEST_STRING);
	REQUIRE(string::convertUtf8ToWChar(UTF8_TEST_STRING) == WCHAR_TEST_STRING);

	std::wstring sWide = WCHAR_TEST_STRING;
	REQUIRE(string::convertWCharToUtf8(sWide) == UTF8_TEST_STRING);
	REQUIRE(string::convertWCharToUtf8(WCHAR_TEST_STRING) == UTF8_TEST_STRING);

#undef UTF8_TEST_STRING 
#undef WCHAR_TEST_STRING

#pragma warning( pop ) 
}

//
//
//
TEST_CASE("string.utf8_encoding_invalid")
{
	REQUIRE_THROWS_AS([&]() {
		(void)string::convertUtf8ToWChar("\xFF\x00");
	}(), error::StringEncodingError);

	REQUIRE_THROWS_AS([&]() {
		(void)string::convertWCharToUtf8(L"\xDC00\xDC00\xDC00\xDC00");
	}(), error::StringEncodingError);
}

//
//
//
TEST_CASE("string.convertWildcardToRegex_string")
{
	auto fnScenario = [&](const std::string_view sWildcard, const std::string_view sRegex)
	{
		std::string sResult = string::convertWildcardToRegex(sWildcard);
		REQUIRE(sResult == sRegex);
	};

	const std::map<const std::string, std::tuple<const std::string, const std::string>> mapData{
		// sName: sWildcard, sRegex
		{ "blank", { "", "" } },
		{ "asterisk_1", { "*", ".*" } },
		{ "asterisk_2", { "*xyz*", ".*xyz.*" } },
		{ "question_1", { "x?yz", "x.yz" } },
		{ "question_2", { "xyz??", "xyz.." } },
		{ "complex", { "*xy??z", ".*xy..z" } },
		{ "unescape", { "\\^.$|()[]{}*+?/", "\\\\\\^\\.\\$\\|\\(\\)\\[\\]\\{\\}.*\\+.\\/" } },
	};

	for (const auto&[sName, params] : mapData)
	{
		const auto&[sWildcard, sRegex] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sWildcard, sRegex));
		}
	}
}

//
//
//
TEST_CASE("string.convertWildcardToRegex_wstring")
{
	auto fnScenario = [&](const std::wstring_view sWildcard, const std::wstring_view sRegex)
	{
		std::wstring sResult = string::convertWildcardToRegex(sWildcard);
		REQUIRE(sResult == sRegex);
	};

	const std::map<const std::string, std::tuple<const std::wstring, const std::wstring>> mapData{
		// sName: sWildcard, sRegex
		{ "blank", { L"", L"" } },
		{ "asterisk_1", { L"*", L".*" } },
		{ "asterisk_2", { L"*xyz*", L".*xyz.*" } },
		{ "question_1", { L"x?yz", L"x.yz" } },
		{ "question_2", { L"xyz??", L"xyz.." } },
		{ "complex", { L"*xy??z", L".*xy..z" } },
		{ "unescape", { L"\\^.$|()[]{}*+?/", L"\\\\\\^\\.\\$\\|\\(\\)\\[\\]\\{\\}.*\\+.\\/" } },
	};

	for (const auto&[sName, params] : mapData)
	{
		const auto&[sWildcard, sRegex] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sWildcard, sRegex));
		}
	}
}
