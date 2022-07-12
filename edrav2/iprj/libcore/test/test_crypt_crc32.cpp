//
//  EDRAv2. Unit test
//    CRC32
//
#include "pch.h"

using namespace cmd::crypt;

static const uint8_t g_sStr[] = "Hello world!";
static const size_t g_nStrLen = std::size(g_sStr) - 1;
static const uint32_t g_nHash = 0x1B851995;
static const uint32_t g_nHashEmpty = 0x00000000;
static const uint32_t g_nHashBig = 0x29058C73;

//
//
//
TEST_CASE("crc32.buffer")
{
	typedef crc32::Hasher Hasher;

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, g_nStrLen);
			REQUIRE(ctx.finalize() == g_nHash);
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
			REQUIRE(ctx.finalize() == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("empty")
	{
		auto fn1 = [&]()
		{
			Hasher ctx;
			REQUIRE(ctx.finalize() == g_nHashEmpty);
		};
		REQUIRE_NOTHROW(fn1());
	
		auto fn2 = [&]()
		{
			Hasher ctx;
			ctx.update(g_sStr, 0);
			REQUIRE(ctx.finalize() == g_nHashEmpty);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			Hasher ctx;
			ctx.update(nullptr, 1);
			REQUIRE(ctx.finalize() == g_nHashEmpty);
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
			ctx.update( pBuffer, sizeof(pBuffer));
			REQUIRE(ctx.finalize() == g_nHashBig);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("crc32.function")
{
	SECTION("readAll")
	{
		auto fn = [&]()
		{
			REQUIRE(crc32::getHash(g_sStr, g_nStrLen) == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("crc32.string")
{
	std::string sStr((const char*)g_sStr);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			REQUIRE(crc32::getHash(sStr) == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("crc32.stream")
{
	auto pStream = cmd::io::createMemoryStream(g_sStr, g_nStrLen);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			REQUIRE(crc32::getHash(pStream) == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readSize")
	{
		auto fn = [&]()
		{
			REQUIRE(crc32::getHash(pStream, g_nStrLen) == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readWrongSize")
	{
		auto fn = [&]()
		{
			crc32::getHash(pStream, g_nStrLen + 1);
		};
		REQUIRE_THROWS_AS(fn(), cmd::error::NoData);
	}
}
