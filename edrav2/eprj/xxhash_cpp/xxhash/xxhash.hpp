#pragma once
#ifdef XXH_USE_STDLIB
#include <cstdint>
#include <cstring>
#include <array>
#include <type_traits>
#include <vector>
#include <string>
#else
#include <stdlib.h>
#include <memory.h>
#endif

/*
xxHash - Extremely Fast Hash algorithm
Header File
Copyright (C) 2012-2018, Yann Collet.
Copyright (C) 2017-2018, Piotr Pliszka.
All rights reserved.

BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
You can contact the author at :
- xxHash source repository : https://github.com/Cyan4973/xxHash
- xxHash C++ port repository : https://github.com/RedSpah/xxhash_cpp
*/

/* *************************************
*  Tuning parameters
***************************************/
/*!XXH_FORCE_MEMORY_ACCESS :
* By default, access to unaligned memory is controlled by `memcpy()`, which is safe and portable.
* Unfortunately, on some target/compiler combinations, the generated assembly is sub-optimal.
* The below switch allow to select different access method for improved performance.
* Method 0 (default) : use `memcpy()`. Safe and portable.
* Method 1 : `__packed` statement. It depends on compiler extension (ie, not portable).
*            This method is safe if your compiler supports it, and *generally* as fast or faster than `memcpy`.
* Method 2 : direct access. This method doesn't depend on compiler but violate C standard.
*            It can generate buggy code on targets which do not support unaligned memory accesses.
*            But in some circumstances, it's the only known way to get the most performance (ie GCC + ARMv6)
* See http://stackoverflow.com/a/32095106/646947 for details.
* Prefer these methods in priority order (0 > 1 > 2)
*/
#ifndef XXH_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
#  if defined(__GNUC__) && ( defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) )
#    define XXH_FORCE_MEMORY_ACCESS 2
#  elif defined(__INTEL_COMPILER) || (defined(__GNUC__) && ( defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) ))
#    define XXH_FORCE_MEMORY_ACCESS 1
#  endif
#endif


/*!XXH_ACCEPT_NULL_INPUT_POINTER :
 * If input pointer is NULL, xxHash default behavior is to dereference it, triggering a segfault.
 * When this macro is enabled, xxHash actively checks input for null pointer.
 * It it is, result for null input pointers is the same as a null-length input.
 */
#ifndef XXH_ACCEPT_NULL_INPUT_POINTER   /* can be defined externally */
#  define XXH_ACCEPT_NULL_INPUT_POINTER 0
#endif
 
 
 /*!XXH_FORCE_NATIVE_FORMAT :
* By default, xxHash library provides endian-independent Hash values, based on little-endian convention.
* Results are therefore identical for little-endian and big-endian CPU.
* This comes at a performance cost for big-endian CPU, since some swapping is required to emulate little-endian format.
* Should endian-independence be of no importance for your application, you may set the #define below to 1,
* to improve speed for Big-endian CPU.
* This option has no impact on Little_Endian CPU.
*/
#if !defined(XXH_FORCE_NATIVE_FORMAT) || (XXH_FORCE_NATIVE_FORMAT == 0)  /* can be defined externally */
#	define XXH_FORCE_NATIVE_FORMAT 0
#	define XXH_CPU_LITTLE_ENDIAN 1
#endif


/*!XXH_FORCE_ALIGN_CHECK :
* This is a minor performance trick, only useful with lots of very small keys.
* It means : check for aligned/unaligned input.
* The check costs one initial branch per hash;
* set it to 0 when the input is guaranteed to be aligned,
* or when alignment doesn't matter for performance.
*/
#ifndef XXH_FORCE_ALIGN_CHECK /* can be defined externally */
#	if defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#		define XXH_FORCE_ALIGN_CHECK 0
#	else
#		define XXH_FORCE_ALIGN_CHECK 1
#	endif
#endif

/*!XXH_CPU_LITTLE_ENDIAN :
* This is a CPU endian detection macro, will be
* automatically set to 1 (little endian) if XXH_FORCE_NATIVE_FORMAT
* is left undefined, XXH_FORCE_NATIVE_FORMAT is defined to 0, or if an x86/x86_64 compiler macro is defined.
* If left undefined, endianness will be determined at runtime, at the cost of a slight one-time overhead
* and a larger overhead due to get_endian() not being constexpr.
*/
#ifndef XXH_CPU_LITTLE_ENDIAN
#	if defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#		define XXH_CPU_LITTLE_ENDIAN 1
#	endif
#endif

/* *************************************
*  Compiler Specific Options
***************************************/
#define XXH_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

namespace xxh
{
	/* *************************************
	*  Version
	***************************************/
	constexpr int cpp_version_major = 0;
	constexpr int cpp_version_minor = 6;
	constexpr int cpp_version_release = 5;
	constexpr uint32_t version_number() { return cpp_version_major * 10000 + cpp_version_minor * 100 + cpp_version_release; }

	namespace hash_t_impl
	{
		/* *************************************
		*  Basic Types - Detail
		***************************************/

		using _hash32_underlying = uint32_t;
		using _hash64_underlying = uint64_t;

		template <size_t N>
		struct hash_type { using type = void; };
		template <>
		struct hash_type<32> { using type = _hash32_underlying; };
		template <>
		struct hash_type<64> { using type = _hash64_underlying; };
	}

	/* *************************************
	*  Basic Types - Public
	***************************************/

	template <size_t N>
	using hash_t = typename hash_t_impl::hash_type<N>::type;
	using hash32_t = hash_t<32>;
	using hash64_t = hash_t<64>;

	/* *************************************
	*  Bit Functions - Public
	***************************************/

	namespace bit_ops
	{
		/* ****************************************
		*  Intrinsics and Bit Operations
		******************************************/

#if defined(_MSC_VER)
		static inline uint32_t rotl32(uint32_t x, int32_t r) { return _rotl(x, r); }
		static inline uint64_t rotl64(uint64_t x, int32_t r) { return _rotl64(x, r); }
#else
		static inline uint32_t rotl32(uint32_t x, int32_t r) { return ((x << r) | (x >> (32 - r))); }
		static inline uint64_t rotl64(uint64_t x, int32_t r) { return ((x << r) | (x >> (64 - r))); }
#endif

#if defined(_MSC_VER)     /* Visual Studio */
		static inline uint32_t swap32(uint32_t x) { return _byteswap_ulong(x); }
		static inline uint64_t swap64(uint64_t x) { return _byteswap_uint64(x); }
#elif XXH_GCC_VERSION >= 403
		static inline uint32_t swap32(uint32_t x) { return __builtin_bswap32(x); }
		static inline uint64_t swap64(uint64_t x) { return __builtin_bswap64(x); }
#else
		static inline uint32_t swap32(uint32_t x) { return ((x << 24) & 0xff000000) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff); }
		static inline uint64_t swap64(uint64_t x) { return ((x << 56) & 0xff00000000000000ULL) | ((x << 40) & 0x00ff000000000000ULL) | ((x << 24) & 0x0000ff0000000000ULL) | ((x << 8) & 0x000000ff00000000ULL) | ((x >> 8) & 0x00000000ff000000ULL) | ((x >> 24) & 0x0000000000ff0000ULL) | ((x >> 40) & 0x000000000000ff00ULL) | ((x >> 56) & 0x00000000000000ffULL); }
#endif

		template <size_t N>
		static inline hash_t<N> rotl(hash_t<N> n, int32_t r)
		{
			if constexpr (N == 32) { return rotl32(n, r); }
			else { return rotl64(n, r); }
		}

		template <size_t N>
		static inline hash_t<N> swap(hash_t<N> x)
		{
			if constexpr (N == 32) { return swap32(x); }
			else { return swap64(x); }
		}
	}

	/* *************************************
	*  Memory Functions - Public
	***************************************/

	enum class alignment : uint8_t { aligned, unaligned };
	enum class endianness : uint8_t { big_endian = 0, little_endian = 1, unspecified = 2 };

	namespace mem_ops
	{
		/* *************************************
		*  Memory Access
		***************************************/
#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))

		/* Force direct memory access. Only works on CPU which support unaligned memory access in hardware */
		template <size_t N>
		static inline hash_t<N> read_unaligned(const void* memPtr) { return *(const hash_t<N>*)memPtr; }

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))

		/* __pack instructions are safer, but compiler specific, hence potentially problematic for some compilers */
		/* currently only defined for gcc and icc */
		template <size_t N>
		using unalign = union { hash_t<N> uval; } __attribute((packed));

		template <size_t N>
		static inline hash_t<N> read_unaligned(const void* memPtr) { return ((const unalign*)memPtr)->uval; }
#else

		/* portable and safe solution. Generally efficient.
		* see : http://stackoverflow.com/a/32095106/646947
		*/
		template <size_t N>
		static inline hash_t<N> read_unaligned(const void* memPtr)
		{
			hash_t<N> val;
			memcpy(&val, memPtr, sizeof(val));
			return val;
		}

#endif   /* XXH_FORCE_DIRECT_MEMORY_ACCESS */

		static inline hash_t<32> read32(const void* memPtr) { return read_unaligned<32>(memPtr); }
		static inline hash_t<64> read64(const void* memPtr) { return read_unaligned<64>(memPtr); }

		/* *************************************
		*  Architecture Macros
		***************************************/

		/* XXH_CPU_LITTLE_ENDIAN can be defined externally, for example on the compiler command line */

#ifndef XXH_CPU_LITTLE_ENDIAN

		static inline endianness get_endian(endianness endian)
		{
			static struct _dummy_t
			{
				//std::array<endianness, 3> endian_lookup = { endianness::big_endian, endianness::little_endian, endianness::unspecified };
				endianness endian_lookup[3] = { endianness::big_endian, endianness::little_endian, endianness::unspecified };
				const int g_one = 1;
				_dummy_t()
				{
					endian_lookup[2] = static_cast<endianness>(*(const char*)(&g_one));
				}
			} _dummy;

			return _dummy.endian_lookup[(uint8_t)endian];
		}

		static inline bool is_little_endian()
		{
			return get_endian(endianness::unspecified) == endianness::little_endian;
		}

#else
		constexpr endianness get_endian(endianness endian)
		{
			//constexpr std::array<endianness, 3> endian_lookup = { endianness::big_endian, endianness::little_endian, (XXH_CPU_LITTLE_ENDIAN) ? endianness::little_endian : endianness::big_endian };
			constexpr endianness endian_lookup[3] = { endianness::big_endian, endianness::little_endian, (XXH_CPU_LITTLE_ENDIAN) ? endianness::little_endian : endianness::big_endian };
			return endian_lookup[static_cast<uint8_t>(endian)];
		}

		constexpr bool is_little_endian()
		{
			return get_endian(endianness::unspecified) == endianness::little_endian;
		}

#endif



		/* ***************************
		*  Memory reads
		*****************************/


		template <size_t N>
		static inline hash_t<N> readLE_align(const void* ptr, endianness endian, alignment align)
		{
			if (align == alignment::unaligned)
			{
				return endian == endianness::little_endian ? read_unaligned<N>(ptr) : bit_ops::swap<N>(read_unaligned<N>(ptr));
			}
			else
			{
				return endian == endianness::little_endian ? *reinterpret_cast<const hash_t<N>*>(ptr) : bit_ops::swap<N>(*reinterpret_cast<const hash_t<N>*>(ptr));
			}
		}

		template <size_t N>
		static inline hash_t<N> readLE(const void* ptr, endianness endian)
		{
			return readLE_align<N>(ptr, endian, alignment::unaligned);
		}

		template <size_t N>
		static inline hash_t<N> readBE(const void* ptr)
		{
			return is_little_endian() ? bit_ops::swap<N>(read_unaligned<N>(ptr)) : read_unaligned<N>(ptr);
		}

		template <size_t N>
		static inline alignment get_alignment(const void* input)
		{
			return ((XXH_FORCE_ALIGN_CHECK) && ((reinterpret_cast<uintptr_t>(input) & ((N / 8) - 1)) == 0)) ? xxh::alignment::aligned : xxh::alignment::unaligned;
		}
	}

	/* *******************************************************************
	*  Hash functions
	*********************************************************************/

	namespace detail
	{
		/* *******************************************************************
		*  Hash functions - Implementation
		*********************************************************************/

		//constexpr static std::array<hash32_t, 5> primes32 = { 2654435761U, 2246822519U, 3266489917U, 668265263U, 374761393U };
		//constexpr static std::array<hash64_t, 5> primes64 = { 11400714785074694791ULL, 14029467366897019727ULL, 1609587929392839161ULL, 9650029242287828579ULL, 2870177450012600261ULL };
		constexpr static hash32_t primes32[5] = { 2654435761U, 2246822519U, 3266489917U, 668265263U, 374761393U };
		constexpr static hash64_t primes64[5] = { 11400714785074694791ULL, 14029467366897019727ULL, 1609587929392839161ULL, 9650029242287828579ULL, 2870177450012600261ULL };

		template <size_t N>
		constexpr hash_t<N> PRIME(int32_t n)
		{
			if constexpr (N == 32) return primes32[n - 1];
			else if constexpr (N == 64) return primes64[n - 1];
#pragma warning(suppress: 4702)
			//return 0;
		}

		template <size_t N>
		static inline hash_t<N> round(hash_t<N> seed, hash_t<N> input)
		{
			seed += input * PRIME<N>(2);
			seed = bit_ops::rotl<N>(seed, ((N == 32) ? 13 : 31));
			seed *= PRIME<N>(1);
			return seed;
		}

		static inline hash64_t mergeRound64(hash64_t acc, hash64_t val)
		{
			val = round<64>(0, val);
			acc ^= val;
			acc = acc * PRIME<64>(1) + PRIME<64>(4);
			return acc;
		}

		template <size_t N>
		static inline hash_t<N> endian_align(const void* input, size_t len, hash_t<N> seed, xxh::endianness endian, xxh::alignment align)
		{
			static_assert(!(N != 32 && N != 64), "You can only call endian_align in 32 or 64 bit mode.");

			const uint8_t* p = static_cast<const uint8_t*>(input);
			const uint8_t* bEnd = p + len;
			hash_t<N> hash_ret;

#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
			if (p == nullptr) {
				len = 0;
				bEnd = p = (const uint8_t*)(size_t)16;
			}
#endif 
			if (len >= (N / 2))
			{
				const uint8_t* const limit = bEnd - (N / 2);
				hash_t<N> v1 = seed + PRIME<N>(1) + PRIME<N>(2);
				hash_t<N> v2 = seed + PRIME<N>(2);
				hash_t<N> v3 = seed + 0;
				hash_t<N> v4 = seed - PRIME<N>(1);

				do
				{
					v1 = round<N>(v1, mem_ops::readLE_align<N>(p, endian, align)); p += (N / 8);
					v2 = round<N>(v2, mem_ops::readLE_align<N>(p, endian, align)); p += (N / 8);
					v3 = round<N>(v3, mem_ops::readLE_align<N>(p, endian, align)); p += (N / 8);
					v4 = round<N>(v4, mem_ops::readLE_align<N>(p, endian, align)); p += (N / 8);
				} while (p <= limit);

				hash_ret = bit_ops::rotl<N>(v1, 1) + bit_ops::rotl<N>(v2, 7) + bit_ops::rotl<N>(v3, 12) + bit_ops::rotl<N>(v4, 18);

				if constexpr (N == 64)
				{
					hash_ret = mergeRound64(hash_ret, v1);
					hash_ret = mergeRound64(hash_ret, v2);
					hash_ret = mergeRound64(hash_ret, v3);
					hash_ret = mergeRound64(hash_ret, v4);
				}
			}
			else { hash_ret = seed + PRIME<N>(5); }

			hash_ret += static_cast<hash_t<N>>(len);

			if constexpr (N == 64)
			{
				while (p + 8 <= bEnd)
				{
					const hash64_t k1 = round<64>(0, mem_ops::readLE_align<64>(p, endian, align));
					hash_ret ^= k1;
					hash_ret = bit_ops::rotl<64>(hash_ret, 27) * PRIME<64>(1) + PRIME<64>(4);
					p += 8;
				}

				if (p + 4 <= bEnd)
				{
					hash_ret ^= static_cast<hash64_t>(mem_ops::readLE_align<32>(p, endian, align)) * PRIME<64>(1);
					hash_ret = bit_ops::rotl<64>(hash_ret, 23) * PRIME<64>(2) + PRIME<64>(3);
					p += 4;
				}

				while (p < bEnd)
				{
					hash_ret ^= (*p) * PRIME<64>(5);
					hash_ret = bit_ops::rotl<64>(hash_ret, 11) * PRIME<64>(1);
					p++;
				}

				hash_ret ^= hash_ret >> 33;
				hash_ret *= PRIME<64>(2);
				hash_ret ^= hash_ret >> 29;
				hash_ret *= PRIME<64>(3);
				hash_ret ^= hash_ret >> 32;
			}
			else
			{
				while ((p + 4) <= bEnd)
				{
					hash_ret += mem_ops::readLE_align<32>(p, endian, align) * PRIME<32>(3);
					hash_ret = bit_ops::rotl<32>(hash_ret, 17) * PRIME<32>(4);
					p += 4;
				}

				while (p < bEnd)
				{
					hash_ret += (*p) * PRIME<32>(5);
					hash_ret = bit_ops::rotl<32>(hash_ret, 11) * PRIME<32>(1);
					p++;
				}

				hash_ret ^= hash_ret >> 15;
				hash_ret *= PRIME<32>(2);
				hash_ret ^= hash_ret >> 13;
				hash_ret *= PRIME<32>(3);
				hash_ret ^= hash_ret >> 16;
			}

			return hash_ret;
		}

	}

	template <size_t N>
	hash_t<N> xxhash(const void* input, size_t len, hash_t<N> seed = 0, endianness endian = endianness::unspecified)
	{
		static_assert(!(N != 32 && N != 64), "You can only call xxhash in 32 or 64 bit mode.");
		return detail::endian_align<N>(input, len, seed, mem_ops::get_endian(endian), mem_ops::get_alignment<N>(input));
	}

#ifdef XXH_USE_STDLIB
	template <size_t N, typename T>
	hash_t<N> xxhash(const std::basic_string<T>& input, hash_t<N> seed = 0, endianness endian = endianness::unspecified)
	{
		static_assert(!(N != 32 && N != 64), "You can only call xxhash in 32 or 64 bit mode.");
		return detail::endian_align<N>(static_cast<const void*>(input.data()), input.length() * sizeof(T), seed, mem_ops::get_endian(endian), mem_ops::get_alignment<N>(static_cast<const void*>(input.data())));
	}

	template <size_t N, typename ContiguousIterator>
	hash_t<N> xxhash(ContiguousIterator begin, ContiguousIterator end, hash_t<N> seed = 0, endianness endian = endianness::unspecified)
	{
		static_assert(!(N != 32 && N != 64), "You can only call xxhash in 32 or 64 bit mode.");
		using T = typename std::decay_t<decltype(*end)>;
		return detail::endian_align<N>(static_cast<const void*>(&*begin), (end - begin) * sizeof(T), seed, mem_ops::get_endian(endian), mem_ops::get_alignment<N>(static_cast<const void*>(&*begin)));
	}

	template <size_t N, typename T>
	hash_t<N> xxhash(const std::vector<T>& input, hash_t<N> seed = 0, endianness endian = endianness::unspecified)
	{
		static_assert(!(N != 32 && N != 64), "You can only call xxhash in 32 or 64 bit mode.");
		return detail::endian_align<N>(static_cast<const void*>(input.data()), input.size() * sizeof(T), seed, mem_ops::get_endian(endian), mem_ops::get_alignment<N>(static_cast<const void*>(input.data())));
	}

	template <size_t N, typename T, size_t AN>
	hash_t<N> xxhash(const std::array<T, AN>& input, hash_t<N> seed = 0, endianness endian = endianness::unspecified)
	{
		static_assert(!(N != 32 && N != 64), "You can only call xxhash in 32 or 64 bit mode.");
		return detail::endian_align<N>(static_cast<const void*>(input.data()), AN * sizeof(T), seed, mem_ops::get_endian(endian), mem_ops::get_alignment<N>(static_cast<const void*>(input.data())));
	}

	template <size_t N, typename T>
	hash_t<N> xxhash(const std::initializer_list<T>& input, hash_t<N> seed = 0, endianness endian = endianness::unspecified)
	{
		static_assert(!(N != 32 && N != 64), "You can only call xxhash in 32 or 64 bit mode.");
		return detail::endian_align<N>(static_cast<const void*>(input.begin()), input.size() * sizeof(T), seed, mem_ops::get_endian(endian), mem_ops::get_alignment<N>(static_cast<const void*>(input.begin())));
	}
#endif


	/* *******************************************************************
	*  Hash streaming
	*********************************************************************/
	enum class error_code : uint8_t { ok = 0, error };

	template <size_t N>
	class hash_state_t {

		uint64_t total_len = 0;
		hash_t<N> v1 = 0, v2 = 0, v3 = 0, v4 = 0;
		//std::array<hash_t<N>, 4> mem = { 0,0,0,0 };
		hash_t<N> mem[4] = { 0,0,0,0 };
		uint32_t memsize = 0;

		inline error_code _update_impl(const void* input, size_t length, endianness endian)
		{
			const uint8_t* p = reinterpret_cast<const uint8_t*>(input);
			const uint8_t* const bEnd = p + length;

			if (!input)
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
				return xxh::error_code::ok;
#else
				return xxh::error_code::error;
#endif 

			total_len += length;

			if (memsize + length < (N / 2))
			{   /* fill in tmp buffer */
				memcpy(reinterpret_cast<uint8_t*>(&mem[0]) + memsize, input, length);
				memsize += static_cast<uint32_t>(length);
				return error_code::ok;
			}

			if (memsize)
			{   /* some data left from previous update */
				memcpy(reinterpret_cast<uint8_t*>(&mem[0]) + memsize, input, (N / 2) - memsize);

				const hash_t<N>* ptr = &mem[0];
				v1 = detail::round<N>(v1, mem_ops::readLE<N>(ptr, endian)); ptr++;
				v2 = detail::round<N>(v2, mem_ops::readLE<N>(ptr, endian)); ptr++;
				v3 = detail::round<N>(v3, mem_ops::readLE<N>(ptr, endian)); ptr++;
				v4 = detail::round<N>(v4, mem_ops::readLE<N>(ptr, endian));

				p += (N / 2) - memsize;
				memsize = 0;
			}

			if (p <= bEnd - (N / 2))
			{
				const uint8_t* const limit = bEnd - (N / 2);

				do
				{
					v1 = detail::round<N>(v1, mem_ops::readLE<N>(p, endian)); p += (N / 8);
					v2 = detail::round<N>(v2, mem_ops::readLE<N>(p, endian)); p += (N / 8);
					v3 = detail::round<N>(v3, mem_ops::readLE<N>(p, endian)); p += (N / 8);
					v4 = detail::round<N>(v4, mem_ops::readLE<N>(p, endian)); p += (N / 8);
				} while (p <= limit);
			}

			if (p < bEnd)
			{
				memcpy(&mem[0], p, static_cast<size_t>(bEnd - p));
				memsize = static_cast<uint32_t>(bEnd - p);
			}

			return error_code::ok;
		}

		inline hash_t<N> _digest_impl(endianness endian) const
		{
			const uint8_t* p = reinterpret_cast<const uint8_t*>(&mem[0]);
			const uint8_t* const bEnd = reinterpret_cast<const uint8_t*>(&mem[0]) + memsize;
			hash_t<N> hash_ret;

			if (total_len > (N / 2))
			{
				hash_ret = bit_ops::rotl<N>(v1, 1) + bit_ops::rotl<N>(v2, 7) + bit_ops::rotl<N>(v3, 12) + bit_ops::rotl<N>(v4, 18);

				if constexpr (N == 64)
				{
					hash_ret = detail::mergeRound64(hash_ret, v1);
					hash_ret = detail::mergeRound64(hash_ret, v2);
					hash_ret = detail::mergeRound64(hash_ret, v3);
					hash_ret = detail::mergeRound64(hash_ret, v4);
				}
			}
			else { hash_ret = v3 + detail::PRIME<N>(5); }

			hash_ret += static_cast<hash_t<N>>(total_len);

			if constexpr (N == 64)
			{

				while (p + 8 <= bEnd) {
					const hash64_t k1 = detail::round<64>(0, mem_ops::readLE<64>(p, endian));
					hash_ret ^= k1;
					hash_ret = bit_ops::rotl<64>(hash_ret, 27) * detail::PRIME<64>(1) + detail::PRIME<64>(4);
					p += 8;
				}

				if (p + 4 <= bEnd) {
					hash_ret ^= static_cast<hash64_t>(mem_ops::readLE<32>(p, endian)) * detail::PRIME<64>(1);
					hash_ret = bit_ops::rotl<64>(hash_ret, 23) * detail::PRIME<64>(2) + detail::PRIME<64>(3);
					p += 4;
				}

				while (p < bEnd) {
					hash_ret ^= (*p) * detail::PRIME<64>(5);
					hash_ret = bit_ops::rotl<64>(hash_ret, 11) * detail::PRIME<64>(1);
					p++;
				}

				hash_ret ^= hash_ret >> 33;
				hash_ret *= detail::PRIME<64>(2);
				hash_ret ^= hash_ret >> 29;
				hash_ret *= detail::PRIME<64>(3);
				hash_ret ^= hash_ret >> 32;
			}
			else
			{
				while (p + 4 <= bEnd)
				{
					hash_ret += mem_ops::readLE<32>(p, endian) * detail::PRIME<32>(3);
					hash_ret = bit_ops::rotl<32>(hash_ret, 17) * detail::PRIME<32>(4);
					p += 4;
				}

				while (p < bEnd)
				{
					hash_ret += (*p) * detail::PRIME<32>(5);
					hash_ret = bit_ops::rotl<32>(hash_ret, 11) * detail::PRIME<32>(1);
					p++;
				}

				hash_ret ^= hash_ret >> 15;
				hash_ret *= detail::PRIME<32>(2);
				hash_ret ^= hash_ret >> 13;
				hash_ret *= detail::PRIME<32>(3);
				hash_ret ^= hash_ret >> 16;
			}

			return hash_ret;
		}

	public:
		hash_state_t(hash_t<N> seed = 0)
		{
			static_assert(!(N != 32 && N != 64), "You can only stream hashing in 32 or 64 bit mode.");
			v1 = seed + detail::PRIME<N>(1) + detail::PRIME<N>(2);
			v2 = seed + detail::PRIME<N>(2);
			v3 = seed + 0;
			v4 = seed - detail::PRIME<N>(1);
		};

		hash_state_t operator=(hash_state_t<N>& other)
		{
			memcpy(this, other, sizeof(hash_state_t<N>));
		}

		error_code reset(hash_t<N> seed = 0)
		{
			memset(this, 0, sizeof(hash_state_t<N>));
			v1 = seed + detail::PRIME<N>(1) + detail::PRIME<N>(2);
			v2 = seed + detail::PRIME<N>(2);
			v3 = seed + 0;
			v4 = seed - detail::PRIME<N>(1);
			return error_code::ok;
		}

		error_code update(const void* input, size_t length, endianness endian = endianness::unspecified)
		{
			return _update_impl(input, length, mem_ops::get_endian(endian));
		}

#ifdef XXH_USE_STDLIB
		template <typename T>
		error_code update(const std::basic_string<T>& input, endianness endian = endianness::unspecified)
		{
			return _update_impl(static_cast<const void*>(input.data()), input.length() * sizeof(T), mem_ops::get_endian(endian));
		}

		template <typename ContiguousIterator>
		error_code update(ContiguousIterator begin, ContiguousIterator end, endianness endian = endianness::unspecified)
		{
			using T = typename std::decay_t<decltype(*end)>;
			return _update_impl(static_cast<const void*>(&*begin), (end - begin) * sizeof(T), mem_ops::get_endian(endian));
		}

		template <typename T>
		error_code update(const std::vector<T>& input, endianness endian = endianness::unspecified)
		{
			return _update_impl(static_cast<const void*>(input.data()), input.size() * sizeof(T), mem_ops::get_endian(endian));
		}

		template <typename T, size_t AN>
		error_code update(const std::array<T, AN>& input, endianness endian = endianness::unspecified)
		{
			return _update_impl(static_cast<const void*>(input.data()), AN * sizeof(T), mem_ops::get_endian(endian));
		}

		template <typename T>
		error_code update(const std::initializer_list<T>& input, endianness endian = endianness::unspecified)
		{
			return _update_impl(static_cast<const void*>(input.begin()), input.size() * sizeof(T), mem_ops::get_endian(endian));
		}
#endif

		hash_t<N> digest(endianness endian = endianness::unspecified)
		{
			return _digest_impl(mem_ops::get_endian(endian));
		}
	};

	using hash_state32_t = hash_state_t<32>;
	using hash_state64_t = hash_state_t<64>;


	/* *******************************************************************
	*  Canonical
	*********************************************************************/

#ifdef XXH_USE_STDLIB
	template <size_t N>
	struct canonical_t
	{
		std::array<uint8_t, N / 8> digest;

		canonical_t(hash_t<N> hash)
		{
			if (mem_ops::is_little_endian()) { hash = bit_ops::swap<N>(hash); }
			memcpy(digest.data(), &hash, sizeof(canonical_t<N>));
		}

		hash_t<N> get_hash() const
		{
			return mem_ops::readBE<64>(&digest);
		}
	};

	using canonical32_t = canonical_t<32>;
	using canonical64_t = canonical_t<64>;
#endif
}
