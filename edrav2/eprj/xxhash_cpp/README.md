# xxhash_cpp
Port of the xxHash library to C++17.

[![Build Status](https://travis-ci.org/RedSpah/xxhash_cpp.svg?branch=master)](https://travis-ci.org/RedSpah/xxhash_cpp)

Compatibility
----
| Compiler             | Min. Version        | 
|----------------------|:-------------------:|
| MSVC (Visual Studio) | 19.1 (VS 2017.3 P2) | 
| clang                | 3.9                 | 
| gcc                  | 7                   |
| EDG eccp             | 4.14                |

Example Usage
----

```cpp
// standalone hash
std::array<int, 4> input {322, 2137, 42069, 65536};
xxh::hash_t<32> hash = xxh::xxhash<32>(input); 

// hash streaming
std::array<unsigned char, 512> buffer;
xxh::hash_state_t<64> hash_stream; 
while (fill_buffer(buffer))
{
  hash_stream.update(buffer);
}
xxh::hash_t<64> final_hash = hash_stream.digest();
```
The template argument specifies whether the algorithm will use the 32 or 64 bit version. Other values are not allowed. Typedefs `hash32_t`, `hash64_t`, `hash_state32_t` and `hash_state64_t` are provided.

`xxh::xxhash` and `xxh::hash_state_t::update` provide several convenient overloads, all accepting optional `seed` and `endianness` arguments:
* C-style `const void*` + `size_t` pair
* `const T*` + `size_t` pair, for passing C arrays
* `const std::vector<T>&`
* `const std::basic_string<T>&`
* A pair of templated iterators
* `const std::array<T, N>&`
* `const std::initializer_list<T>&`

Build Instructions
----

The library is provided as a single standalone header, for static linking only. No build instructions are nessessary.


xxHash - Extremely fast hash algorithm
======================================

xxHash is an Extremely fast Hash algorithm, running at RAM speed limits.
It successfully completes the [SMHasher](http://code.google.com/p/smhasher/wiki/SMHasher) test suite
which evaluates collision, dispersion and randomness qualities of hash functions.
Code is highly portable, and hashes are identical on all platforms (little / big endian).


Benchmarks
-------------------------

The benchmark uses SMHasher speed test, compiled with Visual 2010 on a Windows Seven 32-bits box.
The reference system uses a Core 2 Duo @3GHz


| Name          |   Speed  | Quality | Author           |
|---------------|----------|:-------:|------------------|
| [xxHash]      | 5.4 GB/s |   10    | Y.C.             |
| MurmurHash 3a | 2.7 GB/s |   10    | Austin Appleby   |
| SBox          | 1.4 GB/s |    9    | Bret Mulvey      |
| Lookup3       | 1.2 GB/s |    9    | Bob Jenkins      |
| CityHash64    | 1.05 GB/s|   10    | Pike & Alakuijala|
| FNV           | 0.55 GB/s|    5    | Fowler, Noll, Vo |
| CRC32         | 0.43 GB/s|    9    |                  |
| MD5-32        | 0.33 GB/s|   10    | Ronald L.Rivest  |
| SHA1-32       | 0.28 GB/s|   10    |                  |

[xxHash]: http://www.xxhash.com

Q.Score is a measure of quality of the hash function.
It depends on successfully passing SMHasher test set.
10 is a perfect score.
Algorithms with a score < 5 are not listed on this table.

A more recent version, XXH64, has been created thanks to [Mathias Westerdahl](https://github.com/JCash),
which offers superior speed and dispersion for 64-bits systems.
Note however that 32-bits applications will still run faster using the 32-bits version.

SMHasher speed test, compiled using GCC 4.8.2, on Linux Mint 64-bits.
The reference system uses a Core i5-3340M @2.7GHz

| Version    | Speed on 64-bits | Speed on 32-bits |
|------------|------------------|------------------|
| XXH64      | 13.8 GB/s        |  1.9 GB/s        |
| XXH32      |  6.8 GB/s        |  6.0 GB/s        |

### License

The library file `xxhash.hpp` is BSD licensed.


### Build modifiers

The following macros influence xxhash behavior. They are all disabled by default.

- `XXH_FORCE_NATIVE_FORMAT` : on big-endian systems : use native number representation,
                              resulting in system-specific results.
                              Breaks consistency with little-endian results.

- `XXH_CPU_LITTLE_ENDIAN` : if defined to 0, sets the native endianness to big endian,
                            if defined to 1, sets the native endianness to little endian,
                            if left undefined, the endianness is resolved at runtime, 
                            before `main` is called, at the cost of endianness not being `constexpr`.

- `XXH_FORCE_MEMORY_ACCESS` : if defined to 2, enables unaligned reads as an optimization, this is not standard compliant,
                              if defined to 1, enables the use of `packed` attribute for optimization, only defined for gcc and icc
                              otherwise, uses the default fallback method (`memcpy`) 

### Other languages

Beyond the C reference version,
xxHash is also available on many programming languages,
thanks to great contributors.
They are [listed here](http://www.xxhash.com/#other-languages).
