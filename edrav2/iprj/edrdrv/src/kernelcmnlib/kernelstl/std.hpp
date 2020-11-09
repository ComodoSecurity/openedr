//
// edrav2.edrdrv
//
// Author: Yury Podpruzhnikov (15.06.2019)
// Reviewer:
//
///
/// @file Copies from std c++ library
///
/// @addtogroup kernelCmnLib
/// @{
#pragma once

//
// User mode compatible types
//

typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8 int8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

inline constexpr uint64_t c_nMaxUint64 = ULLONG_MAX;
inline constexpr uint32_t c_nMaxUint32 = ULONG_MAX;
inline constexpr size_t c_nMaxSize = SIZE_MAX;


//////////////////////////////////////////////////////////////////////////
//
// FNV hash algo (copied from MSVC STL)
// Need for std::hash
//
//////////////////////////////////////////////////////////////////////////

namespace Fnv32 {

typedef uint32_t HashVal;

inline constexpr HashVal c_nInitValue = 2166136261U;
inline constexpr HashVal c_nPrime = 16777619U;

//
//
//
inline HashVal calcHash(const void* pData, size_t nDataSize, HashVal nInitVal = c_nInitValue)
{
	auto pByteData = (const uint8_t*)pData;

	for (size_t i = 0; i < nDataSize; ++i)
	{
		nInitVal ^= static_cast<size_t>(pByteData[i]);
		nInitVal *= c_nPrime;
	}

	return nInitVal;
}

//
//
//
template<typename T>
inline HashVal calcHash(const T& data)
{
	return calcHash(&data, sizeof(T));
}

} // namespace Fnv32


//////////////////////////////////////////////////////////////////////////
//
// namespace std (adapted from MSVC STL)
//

namespace std {

//////////////////////////////////////////////////////////////////////////
//
// std::remove_reference
//

template<class _Ty>
struct remove_reference
{	// remove reference
using type = _Ty;
};

template<class _Ty>
struct remove_reference<_Ty&>
{	// remove reference
using type = _Ty;
};

template<class _Ty>
struct remove_reference<_Ty&&>
{	// remove rvalue reference
using type = _Ty;
};

template<class _Ty>
using remove_reference_t = typename remove_reference<_Ty>::type;

//////////////////////////////////////////////////////////////////////////
//
// remove_cv
//

// STRUCT TEMPLATE remove_cv
template<class _Ty>
struct remove_cv
{	// remove top level const and volatile qualifiers
	using type = _Ty;
};

template<class _Ty>
struct remove_cv<const _Ty>
{	// remove top level const and volatile qualifiers
	using type = _Ty;
};

template<class _Ty>
struct remove_cv<volatile _Ty>
{	// remove top level const and volatile qualifiers
	using type = _Ty;
};

template<class _Ty>
struct remove_cv<const volatile _Ty>
{	// remove top level const and volatile qualifiers
	using type = _Ty;
};

template<class _Ty>
using remove_cv_t = typename remove_cv<_Ty>::type;


//
// std::add_const
//
template<class _Ty>
struct add_const
{	// add top level const qualifier
	using type = const _Ty;
};

template<class _Ty>
using add_const_t = const _Ty;


//
// std::move
//
template<class _Ty>
inline[[nodiscard]] constexpr remove_reference_t<_Ty>&& move(_Ty&& _Arg) noexcept
{	// forward _Arg as movable
	return (static_cast<remove_reference_t<_Ty>&&>(_Arg));
}

//
// integral_constant
//
template<class _Ty, _Ty _Val>
struct integral_constant
{	// convenient template for integral constant types
	static constexpr _Ty value = _Val;

	using value_type = _Ty;
	using type = integral_constant;

	constexpr operator value_type() const noexcept
	{	// return stored value
		return (value);
	}

	[[nodiscard]] constexpr value_type operator()() const noexcept
	{	// return stored value
		return (value);
	}
};

// ALIAS TEMPLATE bool_constant
template<bool _Val>
using bool_constant = integral_constant<bool, _Val>;

//
// true_type
//
using true_type = bool_constant<true>;

//
// false_type
//
using false_type = bool_constant<false>;

//////////////////////////////////////////////////////////////////////////
//
// enable_if
//

// type is undefined for assumed !_Test
template<bool _Test, class _Ty = void>
struct enable_if
{
};

// type is _Ty for _Test
template<class _Ty>
struct enable_if<true, _Ty>
{
	using type = _Ty;
};

template<bool _Test, class _Ty = void>
using enable_if_t = typename enable_if<_Test, _Ty>::type;

//////////////////////////////////////////////////////////////////////////
//
// conditional
//
// STRUCT TEMPLATE conditional
template<bool _Test, class _Ty1, class _Ty2>
struct conditional
{	// type is _Ty2 for assumed !_Test
	using type = _Ty2;
};

template<class _Ty1, class _Ty2>
struct conditional<true, _Ty1, _Ty2>
{	// type is _Ty1 for _Test
	using type = _Ty1;
};

template<bool _Test, class _Ty1, class _Ty2>
using conditional_t = typename conditional<_Test, _Ty1, _Ty2>::type;


//////////////////////////////////////////////////////////////////////////
//
// is_integral
//

// STRUCT TEMPLATE _Is_integral
template<class _Ty>
struct _Is_integral
	: false_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<bool>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<char>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<unsigned char>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<signed char>
	: true_type
{	// determine whether _Ty is integral
};

#ifdef _NATIVE_WCHAR_T_DEFINED
template<>
struct _Is_integral<wchar_t>
	: true_type
{	// determine whether _Ty is integral
};
#endif /* _NATIVE_WCHAR_T_DEFINED */

template<>
struct _Is_integral<char16_t>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<char32_t>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<unsigned short>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<short>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<unsigned int>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<int>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<unsigned long>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<long>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<unsigned long long>
	: true_type
{	// determine whether _Ty is integral
};

template<>
struct _Is_integral<long long>
	: true_type
{	// determine whether _Ty is integral
};

// STRUCT TEMPLATE is_integral
template<class _Ty>
struct is_integral
	: _Is_integral<remove_cv_t<_Ty>>::type
{	// determine whether _Ty is integral
};

template<class _Ty>
inline constexpr bool is_integral_v = is_integral<_Ty>::value;


//////////////////////////////////////////////////////////////////////////
//
// is_const
//

template<class _Ty>
struct is_const : false_type
{	// determine whether _Ty is const qualified
};

template<class _Ty>
struct is_const<const _Ty> : true_type
{	// determine whether _Ty is const qualified
};

template<class _Ty>
inline constexpr bool is_const_v = is_const<_Ty>::value;


//////////////////////////////////////////////////////////////////////////
//
// hash
//

typedef uint32_t HashVal;

//
//
//
template<class _Kty, bool _Enabled>
struct _Conditionally_enabled_hash
{	// conditionally enabled hash base
	typedef _Kty argument_type;
	typedef HashVal result_type;

	[[nodiscard]] HashVal operator()(const _Kty& _Keyval) const
	{
		return Fnv32::calcHash(_Keyval);
	}
};

template<class _Kty>
struct _Conditionally_enabled_hash<_Kty, false>
{	// conditionally disabled hash base
	_Conditionally_enabled_hash() = delete;
	_Conditionally_enabled_hash(const _Conditionally_enabled_hash&) = delete;
	_Conditionally_enabled_hash(_Conditionally_enabled_hash&&) = delete;
	_Conditionally_enabled_hash& operator=(const _Conditionally_enabled_hash&) = delete;
	_Conditionally_enabled_hash& operator=(_Conditionally_enabled_hash&&) = delete;
};

// STRUCT TEMPLATE hash
template<class _Kty>
struct hash : _Conditionally_enabled_hash<_Kty, is_integral_v<_Kty>>
{	// hash functor primary template (handles enums, integrals, and pointers)
	static size_t _Do_hash(const _Kty& _Keyval) noexcept
	{	// hash _Keyval to size_t value by pseudorandomizing transform
		return (_Hash_representation(_Keyval));
	}
};

} // namespace std 

/// @}
