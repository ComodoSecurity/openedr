//
//  EDRAv2. Unit test
//    MD5
//
#include "pch.h"

using namespace cmd::crypt;

constexpr size_t c_nHashLen = md5::c_nHashLen;

static const uint8_t g_sStr[] = "Hello world!";
static const size_t g_nStrLen = std::size(g_sStr) - 1;
static const uint8_t g_pHash[c_nHashLen] =
	{ 0x86, 0xfb, 0x26, 0x9d, 0x19, 0x0d, 0x2c, 0x85, 0xf6, 0xe0, 0x46, 0x8c, 0xec, 0xa4, 0x2a, 0x20 };
static const uint8_t g_pHashEmpty[c_nHashLen] =
	{ 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e };
static const uint8_t g_pHashBig[c_nHashLen] =
	{ 0xe2, 0xc8, 0x65, 0xdb, 0x41, 0x62, 0xbe, 0xd9, 0x63, 0xbf, 0xaa, 0x9e, 0xf6, 0xac, 0x18, 0xf0 };

//
//
//
TEST_CASE("md5.buffer")
{
	typedef md5::Hasher Hasher;

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, g_nStrLen);
			md5::Hash pHash = ctx.finalize();
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
			md5::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("empty")
	{
		auto fn1 = [&]()
		{
			Hasher ctx;
			md5::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashEmpty, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn1());
	
		auto fn2 = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, 0);
			md5::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashEmpty, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			Hasher ctx;
			ctx.update(nullptr, 1);
			md5::Hash pHash = ctx.finalize();
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
			md5::Hash pHash = ctx.finalize();
			REQUIRE(memcmp(&pHash, g_pHashBig, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

}

TEST_CASE("md5.function")
{
	SECTION("readAll")
	{
		auto fn = [&]()
		{
			md5::Hash pHash = md5::getHash(g_sStr, g_nStrLen);
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("md5.string")
{
	std::string sStr((const char*)g_sStr);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			md5::Hash pHash = md5::getHash(sStr);
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("md5.stream")
{
	auto pStream = cmd::io::createMemoryStream(g_sStr, g_nStrLen);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			md5::Hash pHash = md5::getHash(pStream);
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readSize")
	{
		auto fn = [&]()
		{
			md5::Hash pHash = md5::getHash(pStream, g_nStrLen);
			REQUIRE(memcmp(&pHash, g_pHash, c_nHashLen) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readWrongSize")
	{
		auto fn = [&]()
		{
			md5::Hash pHash = md5::getHash(pStream, g_nStrLen + 1);
		};
		REQUIRE_THROWS_AS(fn(), cmd::error::NoData);
	}
}
