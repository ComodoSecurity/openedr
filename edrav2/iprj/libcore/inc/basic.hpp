//
// edrav2.libcore project
//
// Autor: Denis Bogdanov (10.12.2018)
// Reviewer: Denis Bogdanov (18.12.2018)
//
///
/// @file Definition of basic types, functions and enums
///
/// @addtogroup commonFunctions Common functions
/// Common functions and objects are intended to implement the frequently used
/// generic actions and operations. 
/// @{
#pragma once

/// 
/// This is the main namespace for COMODO projects.
///
namespace cmd {

// Detect current platform
#ifdef _WIN32
#define PLATFORM_WIN
#elif __linux__
#define PLATFORM_LNX
// Uncomment below for appropriate OS support
//#elif __unix__
//#define PLATFORM_NIX
//#elif __APPLE__
//#define PLATFORM_MAC
#else
#error Unsupported platform type
#endif

// CPU type strings
inline constexpr char c_sAmd64[] = "amd64";
inline constexpr char c_sX86[] = "x86";
inline constexpr char c_sArm64[] = "arm64";
inline constexpr char c_sIa64[] = "ia64";
inline constexpr char c_sArm[] = "arm";


// Detect CPU type
#ifdef PLATFORM_WIN
#ifdef _WIN64
#define CPUTYPE_AMD64
#define CPUTYPE_STRING c_sAmd64
#else
#define CPUTYPE_X86
#define CPUTYPE_STRING c_sX86
#endif
#else
#error Undefined bittness for current platform
#endif

#ifdef _DEBUG
#define ENABLE_TIMINGS
#endif

typedef std::size_t Size;
typedef uint8_t Byte;
typedef uint64_t Enum;
typedef uint64_t Time;

static constexpr Size c_nMaxSize = SIZE_MAX;
static constexpr Time c_nMaxTime = UINT64_MAX;
static constexpr Size c_nMaxStackBufferSize = 0x4000;

//////////////////////////////////////////////////////////////////////////
//
// SFINAE type checker
//

//
// like as std::is_same_v + remove_cv_t
//
template<typename T1, typename T2, typename Result = void>
using IsSameType = std::enable_if_t <
	std::is_same_v<
	typename std::remove_cv_t<T1>,
	typename std::remove_cv_t<T2>
	>, Result
>;

//
// Check is bool
//
template<typename T, typename Result = void>
using IsBoolType = IsSameType<T, bool, Result>;


namespace detail {

template<
	typename Type,
	typename TypeWithoutRefCV = typename std::remove_cv_t<typename std::remove_reference_t<Type>> >
	struct IsIntOrEnumTypeHelper
{
	static constexpr bool value = !std::is_same_v<TypeWithoutRefCV, bool> && // not bool
		(std::is_integral_v<TypeWithoutRefCV> || std::is_enum_v<TypeWithoutRefCV>); // integer or enum
};

} // namespace detail

//
// Check is integer or enum but not bool
//
template<typename T, typename Result = void>
using IsIntOrEnumType = std::enable_if_t<detail::IsIntOrEnumTypeHelper<T>::value, Result>;




//
// Identificator concantination at preprocessor stage
//
#ifndef CONCAT_ID
#define CONCAT_ID_IMPL(a,b) a##b
#define CONCAT_ID(a,b) CONCAT_ID_IMPL(a,b)
#endif // CONCAT_ID


#ifndef CMD_COMPONENT
#define CMD_COMPONENT ""
#endif // CMD_COMPONENT

//
// file + line + component
//
struct SourceLocationTag {};
struct SourceLocation
{
	const char* sFileName;
	int nLine;
	const char* sComponent;

	SourceLocation(SourceLocationTag = {}, const char* _sFileName = "", int _nLine = 0,
			const char* _sComponent = "") :
		sFileName(_sFileName),
		nLine(_nLine),
		sComponent(_sComponent)
	{}

	SourceLocation(const SourceLocation& sl) :
		sFileName(sl.sFileName),
		nLine(sl.nLine),
		sComponent(sl.sComponent)
	{}
};

//
// Short form macro for current source location
//
#define SL cmd::SourceLocation{cmd::SourceLocationTag(), __FILE__, __LINE__, CMD_COMPONENT }

///
/// Conversion enum to underlying type
///
template <typename E>
constexpr typename std::underlying_type<E>::type castToUnderlying(E e) noexcept 
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename E>
constexpr typename std::underlying_type<E>::type* castToUnderlyingPtr(E* e) noexcept 
{
	return reinterpret_cast<typename std::underlying_type<E>::type*>(e);
}


///
/// Definition of bitwise operators for enum which contains flags.
///
#define CMD_DEFINE_ENUM_FLAG_OPERATORS(EnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) noexcept { return static_cast<EnumType>(cmd::castToUnderlying(a) | cmd::castToUnderlying(b)); } \
inline constexpr EnumType operator & (EnumType a, EnumType b) noexcept { return static_cast<EnumType>(cmd::castToUnderlying(a) & cmd::castToUnderlying(b)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) noexcept { return static_cast<EnumType>(cmd::castToUnderlying(a) ^ cmd::castToUnderlying(b)); } \
inline EnumType& operator |= (EnumType &a, EnumType b) noexcept { (*cmd::castToUnderlyingPtr(&a) |= cmd::castToUnderlying(b)); return a; } \
inline EnumType& operator &= (EnumType &a, EnumType b) noexcept { (*cmd::castToUnderlyingPtr(&a) &= cmd::castToUnderlying(b)); return a; } \
inline EnumType& operator ^= (EnumType &a, EnumType b) noexcept { (*cmd::castToUnderlyingPtr(&a) ^= cmd::castToUnderlying(b)); return a; } \
inline constexpr EnumType operator ~ (EnumType a) noexcept { return static_cast<EnumType>(~cmd::castToUnderlying(a)); } \

///
/// Definition of enum which contains flags.
///
/// @param EnumType - a name of new enum.
///
/// @code{.cpp}
/// CMD_DEFINE_ENUM_FLAG(EnumExample)
/// {
///    ExampleElement1 = 1 << 0,
///    ExampleElement2 = 1 << 1,
/// }
/// @endcode
///
#define CMD_DEFINE_ENUM_FLAG(EnumType) \
enum class EnumType : std::uint32_t; \
CMD_DEFINE_ENUM_FLAG_OPERATORS(EnumType) \
enum class EnumType : std::uint32_t


///
/// Bit operations.
///
template <typename T>
inline bool testFlag(T V, T F)
{
	return (V & F) != T(0);
}

template <typename T>
inline T resetFlag(T& V, T F)
{
	V &= (~(F));
	return V;
}

template <typename T>
inline T resetFlag(const T& V, T F)
{
	return V & (~(F));
}

template <typename T>
inline T setFlag(T& V, T F)
{
	V |= F;
	return V;
}

template <typename T>
inline bool testMask(T V, T M)
{
	return (V & M) == M;
}

template <typename T>
inline bool testMaskAny(T V, T M)
{
	return (V & M) != T(0);
}


///
/// Truncate and grow operations.
///

template<typename T>
inline T growToLog2(T Val, T Size)
{
	return (Val & (Size - 1)) ? (Val & ~(Size - 1)) + Size : Val;
}

template<typename T>
inline T trimToLog2(T Val, T Size)
{
	return Val & ~(Size - 1);
}


///
/// BigEndian<->LowEndian conversion.
///
template<typename T>
inline static void swapBytes(T* val)
{
	for (int i = 0; i < sizeof(T) / 2; ++i)
		std::swap(((char*)val)[i], ((char*)val)[sizeof(T) - i - 1]);
}

template<typename T>
inline static T swapBytes(const T& val)
{
	T swapped = (T)0;
	for (int i = 0; i < sizeof(T); ++i)
		swapped |= (val >> (8 * (sizeof(T) - i - 1)) & 0xFF) << (8 * i);
	return swapped;
}

///
/// Memory information descriptor.
///
struct MemInfo
{
	Size nCurrSize;			///< Current allocated memory sise
	Size nTotalSize;		///< Total allocated memory size
	Size nCurrCount;		///< Current allocated blocks count
	Size nTotalCount;		///< Total allocated blocks count
};

//
// Forward definitions for core functions.
// TODO: Move it to separate core_fwd.hpp
//
class ThreadPool;
extern ThreadPool& getCoreThreadPool();
extern ThreadPool& getTimerThreadPool();

} // namespace cmd
/// @}
