//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"
#include "test_stream.h"

using namespace openEdr;

// FIXME: Do we need test strings with "="-end?

static const Byte c_sB64[] = "KiBIZWxsbyB3b3JsZCEgKg==";
static const Byte c_sOrig[] = "* Hello world! *";
static const Size c_nOrigLen = sizeof(c_sOrig) - 1;
static const Size c_nB64Len = sizeof(c_sB64) - 1;

////////////////////////////////////////////////////////////////////////////////
//
// Base64 Encoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
TEST_CASE("Base64Encoder.createObject")
{
	SECTION("normal")
	{
		auto fn = [&]()
		{
			auto pMemStream = io::createMemoryStream(sizeof(c_sB64) - 1);
			auto pObj = createObject(CLSID_Base64Decoder, Dictionary({ {"stream", pMemStream} }));
			ObjPtr<io::IRawReadableStream> pStream = queryInterface<io::IRawReadableStream>(pObj);
			pObj.reset();
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("null_stream_paremeter")
	{
		auto fn = [&]()
		{
			auto pMemStream = nullptr;
			auto pObj = createObject(CLSID_Base64Decoder, Dictionary({ {"stream", pMemStream} }));
			ObjPtr<io::IRawReadableStream> pStream = queryInterface<io::IRawReadableStream>(pObj);
			pObj.reset();
		};
		REQUIRE_THROWS(fn());
	}
}

//
//
//
TEST_CASE("Base64Encoder.write")
{
	SECTION("fixme")
	{
		// FIXME: Refactor this code, may we don't need to call common code for decryptors
		// FIXME: Check invalid parameters and critical situations
		auto fn = [&]()
		{
			auto pMemStream = createObject(CLSID_MemoryStream);
			ObjPtr<io::IRawWritableStream> pStream =
				queryInterface<io::IRawWritableStream>(createObject(CLSID_Base64Encoder,
					Dictionary({ {"stream", pMemStream} })));

			testRawWritableStream(pStream);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("full")
	{
		auto fn = [&]()
		{
			auto pMemStream = createObject(CLSID_MemoryStream);
			ObjPtr<io::IRawWritableStream> pStream =
				queryInterface<io::IRawWritableStream>(createObject(CLSID_Base64Encoder,
					Dictionary({ {"stream", pMemStream} })));

			pStream->write(c_sOrig, c_nOrigLen);
			pStream->flush();
			std::pair<void *, Size> oMemInfo = queryInterface<io::IMemoryBuffer>(pMemStream)->getData();
			REQUIRE(oMemInfo.second == c_nB64Len);
			REQUIRE(memcmp(c_sB64, oMemInfo.first, oMemInfo.second) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("partial_1")
	{
		auto fn = [&]()
		{
			auto pMemStream = createObject(CLSID_MemoryStream);
			ObjPtr<io::IRawWritableStream> pStream =
				queryInterface<io::IRawWritableStream>(createObject(CLSID_Base64Encoder,
					Dictionary({ {"stream", pMemStream} })));

			pStream->write(c_sOrig, c_nOrigLen - 1);
			pStream->flush();
			std::pair<void *, Size> oMemInfo = queryInterface<io::IMemoryBuffer>(pMemStream)->getData();
			REQUIRE(oMemInfo.second == c_nB64Len - 4);
			REQUIRE(memcmp(c_sB64, oMemInfo.first, oMemInfo.second) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	// FIXME: Same test as above but different parameters
	// FIXME: May it is better to compare strings but not only lengths for all variants (0-4)
	SECTION("partial_4")
	{
		auto fn = [&]()
		{
			auto pMemStream = createObject(CLSID_MemoryStream);
			ObjPtr<io::IRawWritableStream> pStream =
				queryInterface<io::IRawWritableStream>(createObject(CLSID_Base64Encoder,
					Dictionary({ {"stream", pMemStream} })));

			pStream->write(c_sOrig, c_nOrigLen - 4);
			pStream->flush();
			std::pair<void *, Size> oMemInfo = queryInterface<io::IMemoryBuffer>(pMemStream)->getData();
			REQUIRE(oMemInfo.second == c_nB64Len - 8);
			REQUIRE(memcmp(c_sB64, oMemInfo.first, oMemInfo.second) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Base64 Decoder
//
////////////////////////////////////////////////////////////////////////////////

//
// FIXME: Refactor
//
TEST_CASE("Base64.decoder", "[openEdr::io]")
{
	static const Byte sOrig[] = "* Hello world! *";
	static const Byte sB64[] = "KiBIZWxsbyB3b3JsZCEgKg==";

	SECTION("CreateObject")
	{
		auto fn = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, sizeof(sB64));
			auto pObj = createObject(CLSID_Base64Decoder, Dictionary({ {"stream", pMemStream} }));
			ObjPtr<io::IRawReadableStream> pStream = queryInterface<io::IRawReadableStream>(pObj);
			pObj.reset();
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("Read")
	{
		static const Size nBufferSize = sizeof(sOrig) - 1;
		static const Size nSrcStrSize = sizeof(sB64) - 1;

		auto fn = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			readRawStream(pStream);
		};
		REQUIRE_NOTHROW(fn());

		auto fn1 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			REQUIRE(pStream->read(pBuffer, nBufferSize) == nBufferSize);
			REQUIRE(!memcmp(sOrig, pBuffer, nBufferSize));
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			REQUIRE(pStream->read(pBuffer, nBufferSize / 2) == nBufferSize / 2);
			REQUIRE(memcmp(sOrig, pBuffer, nBufferSize / 2) == 0);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			Size nDelta = 1;
			REQUIRE(nDelta <= nBufferSize);
			REQUIRE(pStream->read(pBuffer, nBufferSize - nDelta) == nBufferSize - nDelta);
			REQUIRE(memcmp(sOrig, pBuffer, nBufferSize - nDelta) == 0);
			REQUIRE(pStream->read(pBuffer, nDelta) == nDelta);
			REQUIRE(memcmp(sOrig + nBufferSize - nDelta, pBuffer, nDelta) == 0);
		};
		REQUIRE_NOTHROW(fn3());

		auto fn4 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			Size nDelta = nBufferSize - 1;
			REQUIRE(nDelta <= nBufferSize);
			REQUIRE(pStream->read(pBuffer, nBufferSize - nDelta) == nBufferSize - nDelta);
			REQUIRE(memcmp(sOrig, pBuffer, nBufferSize - nDelta) == 0);
			REQUIRE(pStream->read(pBuffer, nDelta) == nDelta);
			REQUIRE(memcmp(sOrig + nBufferSize - nDelta, pBuffer, nDelta) == 0);
		};
		REQUIRE_NOTHROW(fn4());

	}

	SECTION("ReadTrim")
	{
		static const Size nBufferSize = sizeof(sOrig) - 1;
		static const Size nSrcStrSize = sizeof(sB64) - 1;

		auto fn1 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize - 2);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			REQUIRE(pStream->read(pBuffer, nBufferSize) == nBufferSize);
			REQUIRE(memcmp(sOrig, pBuffer, nBufferSize) == 0);
		};
		REQUIRE_NOTHROW(fn1());
	
		auto fn2 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize - 3);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			REQUIRE(pStream->read(pBuffer, nBufferSize) == nBufferSize - 1);
			REQUIRE(memcmp(sOrig, pBuffer, nBufferSize - 1) == 0);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			auto pMemStream = io::createMemoryStream((Byte*)sB64, nSrcStrSize - 10);
			ObjPtr<io::IRawReadableStream> pStream =
				queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
					Dictionary({ {"stream", pMemStream} })));

			Byte pBuffer[nBufferSize];
			REQUIRE(pStream->read(pBuffer, nBufferSize) == nBufferSize - 6);
			REQUIRE(memcmp(sOrig, pBuffer, nBufferSize - 6) == 0);
		};
		REQUIRE_NOTHROW(fn3());
	}
}