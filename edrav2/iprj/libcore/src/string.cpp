//
// edrav2.libcore project
//
// Author: Yury Ermakov (06.03.2019)
// Reviewer: Denis Bogdanov (30.04.2019)
//
///
/// @file String processing functions
///
/// @addtogroup commonFunctions Common functions
/// @{

#include "pch.h"

namespace cmd {
namespace string {

//
//
//
std::string convertWildcardToRegex(std::string_view sWildcard)
{
	if (sWildcard.empty())
		return "";

	std::string sRegex = sWildcard.data();

	// Unescape regex special symbols
	boost::replace_all(sRegex, "\\", "\\\\");
	boost::replace_all(sRegex, "^", "\\^");
	boost::replace_all(sRegex, ".", "\\.");
	boost::replace_all(sRegex, "$", "\\$");
	boost::replace_all(sRegex, "|", "\\|");
	boost::replace_all(sRegex, "(", "\\(");
	boost::replace_all(sRegex, ")", "\\)");
	boost::replace_all(sRegex, "[", "\\[");
	boost::replace_all(sRegex, "]", "\\]");
	boost::replace_all(sRegex, "{", "\\{");
	boost::replace_all(sRegex, "}", "\\}");
	boost::replace_all(sRegex, "*", "\\*");
	boost::replace_all(sRegex, "+", "\\+");
	boost::replace_all(sRegex, "?", "\\?");
	boost::replace_all(sRegex, "/", "\\/");

	// Replace wildcard symbols with regex-relative
	boost::replace_all(sRegex, "\\*", ".*");
	boost::replace_all(sRegex, "\\?", ".");

	return sRegex;
}

//
//
//
std::wstring convertWildcardToRegex(std::wstring_view sWildcard)
{
	if (sWildcard.empty())
		return L"";

	std::wstring sRegex = sWildcard.data();

	// Unescape regex special symbols
	boost::replace_all(sRegex, L"\\", L"\\\\");
	boost::replace_all(sRegex, L"^", L"\\^");
	boost::replace_all(sRegex, L".", L"\\.");
	boost::replace_all(sRegex, L"$", L"\\$");
	boost::replace_all(sRegex, L"|", L"\\|");
	boost::replace_all(sRegex, L"(", L"\\(");
	boost::replace_all(sRegex, L")", L"\\)");
	boost::replace_all(sRegex, L"[", L"\\[");
	boost::replace_all(sRegex, L"]", L"\\]");
	boost::replace_all(sRegex, L"{", L"\\{");
	boost::replace_all(sRegex, L"}", L"\\}");
	boost::replace_all(sRegex, L"*", L"\\*");
	boost::replace_all(sRegex, L"+", L"\\+");
	boost::replace_all(sRegex, L"?", L"\\?");
	boost::replace_all(sRegex, L"/", L"\\/");

	// Replace wildcard symbols with regex-relative
	boost::replace_all(sRegex, L"\\*", L".*");
	boost::replace_all(sRegex, L"\\?", L".");

	return sRegex;
}

} // namespace string
} // namespace cmd

// @}
