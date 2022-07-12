//
// EDRAv2::libcore
// UTF8 strings processing functions
// Wrapper of the utfcpp library (https://github.com/nemtrif/utfcpp)
// 
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#include "pch.h"

namespace cmd {
namespace string {

//
//
//
std::string convertWCharToUtf8(std::wstring_view sSrc)
{
	static_assert(
		sizeof(std::wstring_view::value_type) == sizeof(char16_t) ||
		sizeof(std::wstring_view::value_type) == sizeof(char32_t), 
		"Unicode encoding supports char16_t or char32_t only");
	
	try
	{
		std::vector<std::string::value_type> ResultArray;

		if constexpr (sizeof(std::wstring_view::value_type) == sizeof(char16_t))
			utf8::utf16to8(sSrc.begin(), sSrc.end(), std::back_inserter(ResultArray));
		else
			utf8::utf32to8(sSrc.begin(), sSrc.end(), std::back_inserter(ResultArray));
		
		std::string sResult(ResultArray.begin(), ResultArray.end());
		return sResult;
	}
	catch (utf8::exception& e)
	{
		error::StringEncodingError(SL, FMT("Can't convert wchar to utf8: " << e.what())).throwException();
	}
}

//
//
//
std::wstring convertUtf8ToWChar(std::string_view sSrc)
{
	static_assert(
		sizeof(std::wstring_view::value_type) == sizeof(char16_t) ||
		sizeof(std::wstring_view::value_type) == sizeof(char32_t), 
		"Unicode encoding supports char16_t or char32_t only");

	try
	{
		std::vector<std::wstring::value_type> ResultArray;

		if constexpr (sizeof(std::wstring::value_type) == sizeof(char16_t))
			utf8::utf8to16(sSrc.begin(), sSrc.end(), std::back_inserter(ResultArray));
		else
			utf8::utf8to32(sSrc.begin(), sSrc.end(), std::back_inserter(ResultArray));
		
		std::wstring sResult(ResultArray.begin(), ResultArray.end());
		return sResult;
	}
	catch (utf8::exception& e)
	{
		error::StringEncodingError(SL, FMT("Can't convert utf8 to wchar: " << e.what())).throwException();
	}

}

//
//
//
std::string convertToUtf8(const std::string& sSrc)
{
	boost::locale::generator gen;
	gen.locale_cache_enabled(true);
	std::locale loc = gen(boost::locale::util::get_system_locale());
	return boost::locale::conv::to_utf<char>(sSrc, loc);
}

} // namespace string
} // namespace cmd 
