//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"
#include "test_stream.h"

using namespace openEdr;

TEST_CASE("CacheStream.createObject", "[openEdr::io]")
{
	SECTION("RawReadableStreamCache.")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_MemoryStream);
			auto pCache = io::createStreamCache(queryInterface<io::IRawReadableStream>(pObj));
			pObj.reset();
			pCache.reset();
		}());
	}

	SECTION("ReadableStreamCache.")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_MemoryStream);
			auto pCache = io::createStreamCache(queryInterface<io::IReadableStream>(pObj));
			pObj.reset();
			pCache.reset();
		}());
	}

	SECTION("RawReadableStreamConverter.")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_MemoryStream);
			auto pCache = io::convertStream(queryInterface<io::IReadableStream>(pObj));
			pObj.reset();
			pCache.reset();
		}());
	}
}

TEST_CASE("CacheStream.queryInterface", "[openEdr::io]")
{
	auto pObj = createObject(CLSID_MemoryStream);

	SECTION("RawReadableStreamCache.")
	{
		REQUIRE_NOTHROW([&]() {
			auto pCache = queryInterface<io::IRawReadableStream>(io::createStreamCache(queryInterface<io::IRawReadableStream>(pObj)));
			pCache.reset();
		}());
		REQUIRE_THROWS([&]() {
			auto pCache = queryInterface<io::IReadableStream>(io::createStreamCache(queryInterface<io::IRawReadableStream>(pObj)));
			pCache.reset();
		}());
	}

	SECTION("ReadableStreamCache.")
	{
		REQUIRE_NOTHROW([&]() {
			auto pCacheRR = queryInterface<io::IRawReadableStream>(io::createStreamCache(queryInterface<io::IReadableStream>(pObj)));
			pCacheRR.reset();
			auto pCacheR = queryInterface<io::IReadableStream>(io::createStreamCache(queryInterface<io::IReadableStream>(pObj)));
			pCacheR.reset();
		}());
	}

	SECTION("RawReadableStreamConverter.")
	{
		REQUIRE_NOTHROW([&]() {
			auto pCacheRR = queryInterface<io::IRawReadableStream>(io::convertStream(queryInterface<io::IReadableStream>(pObj)));
			pCacheRR.reset();
			auto pCacheR = queryInterface<io::IReadableStream>(io::convertStream(queryInterface<io::IReadableStream>(pObj)));
			pCacheR.reset();
		}());
	}
}


TEST_CASE("CacheStream.readRawReadableStreamCache", "[openEdr::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IRawReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createStreamCache(queryInterface<io::IRawReadableStream>(io::createMemoryStream(sizeof(pBuffer))));
	}());

	readRawStream(pStream);
}

TEST_CASE("CacheStream.readReadableStreamCache", "[openEdr::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createStreamCache(io::createMemoryStream(sizeof(pBuffer)));
	}());

	readStream(pStream);
}

TEST_CASE("CacheStream.readRawReadableStreamConverter", "[openEdr::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::convertStream(io::createMemoryStream(sizeof(pBuffer)));
	}());

	readStream(pStream);
}


TEST_CASE("CacheStream.positionReadableStreamCache", "[openEdr::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createStreamCache(io::createMemoryStream(sizeof(pBuffer)));
	}());

	streamPosition(pStream);
}

TEST_CASE("CacheStream.positionRawReadableStreamConverter", "[openEdr::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::convertStream(io::createMemoryStream(sizeof(pBuffer)));
	}());

	streamPosition(pStream);
}

