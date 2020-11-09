//
//  EDRAv2. Unit test
//
#include "pch.h"

using namespace openEdr::crypt;

static const uint8_t g_sStr[] = "Hello world!";
static const size_t g_nStrLen = std::size(g_sStr) - 1;
static const uint64_t g_nHash = 0x7f173f227ffd7db2;
static const uint64_t g_nHashEmpty = 0xef46db3751d8e999;
static const uint64_t g_nHashBig = 0x1facbe8406cd904b;

//
//
//
TEST_CASE("xxh.buffer")
{
	SECTION("readAll")
	{
		auto fn = [&]()
		{
			xxh::hash_state64_t ctx;
			REQUIRE(ctx.update(g_sStr, g_nStrLen) == xxh::error_code::ok);
			REQUIRE(ctx.digest() == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readTwice")
	{
		auto fn = [&]()
		{
			xxh::hash_state64_t ctx;
			REQUIRE(ctx.update(g_sStr, g_nStrLen / 2) == xxh::error_code::ok);
			REQUIRE(ctx.update(g_sStr + (g_nStrLen / 2), g_nStrLen / 2) == xxh::error_code::ok);
			REQUIRE(ctx.digest() == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("empty")
	{
		auto fn1 = [&]()
		{
			xxh::hash_state64_t ctx;
			REQUIRE(ctx.digest() == g_nHashEmpty);
		};
		REQUIRE_NOTHROW(fn1());
	
		auto fn2 = [&]()
		{
			xxh::hash_state64_t ctx;
			REQUIRE(ctx.update(g_sStr, 0) == xxh::error_code::ok);
			REQUIRE(ctx.digest() == g_nHashEmpty);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			xxh::hash_state64_t ctx;
			REQUIRE(ctx.update(nullptr, g_nStrLen) == xxh::error_code::ok);
			REQUIRE(ctx.digest() == g_nHashEmpty);
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
			xxh::hash_state64_t ctx;
			REQUIRE(ctx.update(pBuffer, sizeof(pBuffer)) == xxh::error_code::ok);
			REQUIRE(ctx.digest() == g_nHashBig);
		};
		REQUIRE_NOTHROW(fn());
	}
}

TEST_CASE("xxh.function")
{
	SECTION("readAll")
	{
		auto fn1 = [&]()
		{
			REQUIRE(xxh::xxhash<64>(g_sStr, g_nStrLen) == g_nHash);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			REQUIRE(xxh::getHash(g_sStr, g_nStrLen) == g_nHash);
		};
		REQUIRE_NOTHROW(fn2());
	}
}

TEST_CASE("xxh.string")
{
	std::string sStr((const char*)g_sStr);

	SECTION("readAll")
	{
		auto fn1 = [&]()
		{
			REQUIRE(xxh::xxhash<64>(sStr) == g_nHash);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			REQUIRE(xxh::getHash(sStr) == g_nHash);
		};
		REQUIRE_NOTHROW(fn2());
	}
}

TEST_CASE("xxh.stream")
{
	auto pStream = openEdr::io::createMemoryStream(g_sStr, g_nStrLen);

	SECTION("readAll")
	{
		auto fn = [&]()
		{
			REQUIRE(xxh::getHash(pStream) == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readSize")
	{
		auto fn = [&]()
		{
			REQUIRE(xxh::getHash(pStream, g_nStrLen) == g_nHash);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("readWrongSize")
	{
		auto fn = [&]()
		{
			xxh::getHash(pStream, g_nStrLen + 1);
		};
		REQUIRE_THROWS_AS(fn(), openEdr::error::NoData);
	}
}
