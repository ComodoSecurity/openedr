//
//  EDRAv2. Unit test
//    SHA1
//
#include "pch.h"

using namespace cmd::crypt;

constexpr size_t c_nHashLen = md5::c_nHashLen;

static const uint8_t g_sStr[] = "Hello world!";
static const size_t g_nStrLen = std::size(g_sStr) - 1;
static const uint8_t g_pHash[sha1::c_nHashLen] =
	{0xd3, 0x48, 0x6a, 0xe9, 0x13, 0x6e, 0x78, 0x56, 0xbc, 0x42, 0x21, 0x23, 0x85, 0xea, 0x79, 0x70, 
	0x94, 0x47, 0x58, 0x02 };
static const uint8_t g_pHashEmpty[sha1::c_nHashLen] =
	{ 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90,
	0xaf, 0xd8, 0x07, 0x09 };
static const uint8_t g_pHashBig[sha1::c_nHashLen] =
	{ 0x49, 0x16, 0xd6, 0xbd, 0xb7, 0xf7, 0x8e, 0x68, 0x03, 0x69, 0x8c, 0xab, 0x32, 0xd1, 0x58, 0x6e,
	0xa4, 0x57, 0xdf, 0xc8 };

//
//
//
TEST_CASE("sha1.buffer")
{
	typedef sha1::Hasher Hasher;

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, g_nStrLen);
			sha1::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readTwice")
	{
		auto fn = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, g_nStrLen / 2);
			ctx.update(g_sStr + (g_nStrLen / 2), g_nStrLen / 2);
			sha1::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("empty")
	{
		auto fn1 = [&]()
		{
			Hasher ctx;
			sha1::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashEmpty, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, 0);
			sha1::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashEmpty, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			Hasher ctx;
			ctx.update(nullptr, 1);
			sha1::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashEmpty, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn3());
	}

	SECTION("readBig")
	{
		uint8_t pBuffer[0x100];
		for (size_t i = 0; i < sizeof(pBuffer); i++)
			pBuffer[i] = uint8_t(i);

		auto fn = [&]()
		{
			Hasher ctx;
			ctx.update(pBuffer, sizeof(pBuffer));
			sha1::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashBig, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

}

TEST_CASE("sha1.function")
{
	SECTION("readAll")
	{
		auto fn = [&]()
		{
			sha1::Hash pHash = sha1::getHash(g_sStr, g_nStrLen);
			REQUIRE(memcmp(&pHash, g_pHash, sha1::c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("sha1.string")
{
	std::string sStr((const char*)g_sStr);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			sha1::Hash pHash = sha1::getHash(sStr);
			REQUIRE(memcmp(&pHash, g_pHash, sha1::c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("sha1.stream")
{
	auto pStream = cmd::io::createMemoryStream(g_sStr, g_nStrLen);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			sha1::Hash pHash = sha1::getHash(pStream);
			REQUIRE(memcmp(&pHash, g_pHash, sha1::c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readSize")
	{
		auto fn = [&]()
		{
			sha1::Hash pHash = sha1::getHash(pStream, g_nStrLen);
			REQUIRE(memcmp(&pHash, g_pHash, sha1::c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readWrongSize")
	{
		auto fn = [&]()
		{
			sha1::Hash pHash = sha1::getHash(pStream, g_nStrLen + 1);
		};
		REQUIRE_THROWS_AS(fn(), cmd::error::NoData);
	}
}
