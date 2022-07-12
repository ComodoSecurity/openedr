//
// edrav2.libcore project
// 
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
///
/// @file String processing functions
///
/// @addtogroup commonFunctions Common functions
/// @{

#pragma once

///
/// String formating macros.
///
/// Creates a new std::string based on stream output (std::ostringstream)
///
/// Usage examples: 
/// * FMT(variable)
/// * FMT("data: " << variable)
/// * FMT("data: " << std::hex << variable)
///
#define FMT(data) ((std::ostringstream() << data).str())
#define WFMT(data) ((std::wostringstream() << data).str())

namespace cmd {
namespace detail {

//
// cmd::hex helper
//
template<typename T>
struct HexOutputter
{
	using UnsignedInteger = std::make_unsigned_t<T>;
	UnsignedInteger m_nValue;
	HexOutputter(T value) : m_nValue(static_cast<UnsignedInteger>(value)) {}
};


//
// cmd::hex helper
//
template<typename T>
inline std::ostream& operator<<(std::ostream& os, const HexOutputter<T>& value)
{
	auto previosFlag = os.flags();
	auto previosFiller = os.fill('0');
	os << std::hex << std::noshowbase << std::uppercase << "0x" << std::setw(sizeof(T) * 2) << value.m_nValue;
	os.fill(previosFiller);
	os.flags(previosFlag);
	return os;
}

//
// cmd::hex helper
//
template<typename T>
inline std::wostream& operator<<(std::wostream& os, const HexOutputter<T>& value)
{
	auto previosFlag = os.flags();
	auto previosFiller = os.fill(L'0');
	os << std::hex << std::noshowbase << std::uppercase << L"0x" << std::setw(sizeof(T) * 2) << value.m_nValue;
	os.fill(previosFiller);
	os.flags(previosFlag);
	return os;
}

} // namespace detail 

///
/// Out to stream as hex
/// @param value Integer or enum value
///
/// Usage:
/// @code,cpp
/// int value = 100;
/// std::cout << hex(value);
/// @endcode
///
template<typename T>
inline auto hex(T value)
{
	return detail::HexOutputter<T>(value);
}


	
namespace string {

//////////////////////////////////////////////////////////////////////////
//
// String convertion wchar <-> utf8
//

///
/// Converts a wchar-string to utf8.
///
std::string convertWCharToUtf8(std::wstring_view sSrc);

///
/// Converts a utf8-string to wchar.
///
std::wstring convertUtf8ToWChar(std::string_view sSrc);

///
/// Converts a string to UTF-8 using the system locale.
///
/// @param sSrc a source string
/// @return Returns a converted UTF-8 string.
///
std::string convertToUtf8(const std::string& sSrc);

///
/// Converts a specified integer-value into a hex-string.
///
template<typename T>
inline std::string convertToHex(T nValue)
{
	return FMT(hex(nValue));
}

///
/// Converts a specified integer-value into a hex-string.
///
template<typename T>
inline std::wstring convertToHexW(T nValue)
{
	return WFMT(hex(nValue));
}

///
/// Converts an iterable object into a hex-string.
///
template<typename Iter>
std::string convertToHex(Iter pFirst, Iter pLast, bool fUppercase = false, bool fSpaces = false)
{
	std::ostringstream ss;
	ss << std::hex << std::setfill('0');
	if (fUppercase)
		ss << std::uppercase;
	while (pFirst != pLast)
	{
		// FIXME: You use cast to int and write int to stream! What is an idea?
		// Looks like you want to write chars?
		ss << std::setw(2) << static_cast<int>(*pFirst++);
		if (fSpaces && pFirst != pLast)
			ss << " ";
	}
	return ss.str();
}

const std::locale g_locale = std::locale("");

///
/// Transforms a string to lower case.
///
inline std::wstring convertToLow(const std::wstring& s)
{
	std::wstring tmp(s);
	std::transform(tmp.begin(), tmp.end(), tmp.begin(),
		[](wchar_t ch)  -> wchar_t 
		{
			if (ch > 127) return std::tolower(ch, g_locale);
			else if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
			else return ch;
		});
	return tmp;
}

///
/// Transforms string to upper case.
///
inline std::wstring convertToUpper(const std::wstring& s)
{
	std::wstring tmp(s);
	std::transform(tmp.begin(), tmp.end(), tmp.begin(),
		[](wchar_t ch) { return std::toupper(ch, g_locale); });
	return tmp;
}

///
/// Converts a string to lower case.
///
inline std::string convertToLower(std::string_view s)
{
	std::string sResult(s);
	std::transform(sResult.begin(), sResult.end(), sResult.begin(),
		[](unsigned char ch) -> unsigned char
		{
			if (ch > 127) return std::tolower(ch, g_locale);
			else if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
			else return ch;
		});
	return sResult;
}

///
/// Converts a string to lower case.
///
inline std::wstring convertToLower(std::wstring_view s)
{
	std::wstring sResult(s);
	std::transform(sResult.begin(), sResult.end(), sResult.begin(),
		[](wchar_t ch) -> wchar_t
	{
		if (ch > 127) return std::tolower(ch, g_locale);
		else if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
		else return ch;
	});
	return sResult;
}

//
// Converts a string to lower case.
//
//template<typename CharT>
//inline std::basic_string<CharT> convertToLower(std::basic_string_view<CharT> s)
//{
//	std::basic_string<CharT> sResult(s);
//	std::transform(sResult.begin(), sResult.end(), sResult.begin(),
//		[](std::make_unsigned_t<CharT> ch) { return std::tolower(ch, g_locale); });
//	return sResult;
//}

//
// TODO: Locale usage commented to speedup comparison. Uncomment if needed.
//
template<typename T>
inline bool _startsWith(std::basic_string_view<T> sStr, std::basic_string_view<T> sSubstr, bool fCI = false)
{
	if (sStr.empty() || sSubstr.empty() || sSubstr.size() > sStr.size())
		return false;
	if (fCI)
		return std::equal(sSubstr.begin(), sSubstr.end(), sStr.begin(),
			[](const T& c1, const T& c2) {
		return (c1 == c2 || std::toupper(c1/*, g_locale*/) == std::toupper(c2/*, g_locale*/));
	});
	else
		return std::equal(sSubstr.begin(), sSubstr.end(), sStr.begin());
}

///
/// Checks if string starts from given substring.
///
inline bool startsWith(std::string_view sStr, std::string_view sSubstr, bool fCI = false)
{
	return _startsWith(sStr, sSubstr, fCI);
}

///
/// Checks if string starts from given substring.
///
inline bool startsWith(std::wstring_view sStr, std::wstring_view sSubstr, bool fCI = false)
{
	return _startsWith(sStr, sSubstr, fCI);
}

//
// TODO: Locale usage commented to speedup comparison. Uncomment if needed.
//
template<typename T>
inline bool _endsWith(std::basic_string_view<T> sStr, std::basic_string_view<T> sSubstr, bool fCI = false)
{
	auto nStrSize = sStr.size();
	auto nSubstrSize = sSubstr.size();
	if (sStr.empty() || sSubstr.empty() || nSubstrSize > nStrSize)
		return false;

	auto nDelta = nStrSize - nSubstrSize;
	if (fCI)
		return std::equal(sSubstr.begin(), sSubstr.end(), sStr.begin() + nDelta,
			[](const T& c1, const T& c2) {
		return (c1 == c2 || std::toupper(c1/*, g_locale*/) == std::toupper(c2/*, g_locale*/));
	});
	else
		return std::equal(sSubstr.begin(), sSubstr.end(), sStr.begin() + nDelta);
}

///
/// Checks if string ends with given substring.
///
inline bool endsWith(std::string_view sStr, std::string_view sSubstr, bool fCI = false)
{
	return _endsWith(sStr, sSubstr, fCI);
}

///
/// Check if string ends with given substring
///
inline bool endsWith(std::wstring_view sStr, std::wstring_view sSubstr, bool fCI = false)
{
	return _endsWith(sStr, sSubstr, fCI);
}

///
/// Converts a wildcard-based expression to regex format.
///
/// @param sWildcard a string expression in wildcard format.
/// @return Returns a string with result regex-expression.
///
extern std::string convertWildcardToRegex(std::string_view sWildcard);
extern std::wstring convertWildcardToRegex(std::wstring_view sWildcard);

} // namespace string
} // namespace cmd 
/// @}
