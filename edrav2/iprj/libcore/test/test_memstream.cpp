//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"
#include "test_stream.h"

using namespace cmd;

TEST_CASE("MemoryStream.createObject", "[cmd::io]")
{
	SECTION("CreateObject")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_MemoryStream);
			pObj.reset();
		}());
	}
}

TEST_CASE("MemoryStream.queryInterface", "[cmd::io]")
{
	auto pObj = createObject(CLSID_MemoryStream);

	SECTION("IRawReadableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pRawReadableStream = queryInterface<io::IRawReadableStream>(pObj);
		}());
	}

	SECTION("IRawWritableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pRawWritableStream = queryInterface<io::IRawWritableStream>(pObj);
		}());
	}

	SECTION("ISeekableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pSeekableStream = queryInterface<io::ISeekableStream>(pObj);
		}());
	}

	SECTION("IRawReadableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pReadableStream = queryInterface<io::IReadableStream>(pObj);
		}());
	}
	
	SECTION("IRawReadableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pWritableStream = queryInterface<io::IWritableStream>(pObj);
		}());
	}
}

TEST_CASE("MemoryStream.read", "[cmd::io]")
{
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createMemoryStream(c_nStreamSize);
	}());

	readStream(pStream);
}

TEST_CASE("MemoryStream.write", "[cmd::io]")
{
	ObjPtr<io::IWritableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = queryInterface<io::IWritableStream>(createObject(CLSID_MemoryStream));
	}());

	writeStream(pStream);
	testRawWritableStream(pStream);
}

TEST_CASE("MemoryStream.position", "[cmd::io]")
{
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createMemoryStream(c_nStreamSize);
	}());

	streamPosition(pStream);
}

TEST_CASE("MemoryStream.extDataRead", "[cmd::io]")
{
	Byte pBuffer[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createMemoryStream(pBuffer, sizeof(pBuffer));
	}());

	SECTION("Read")
	{
		auto fn = [&]()
		{
			pStream->setPosition(0);
			Size nReadSize = 0;
			pStream->read(pBuffer, sizeof(pBuffer), &nReadSize);
			REQUIRE(nReadSize == sizeof(pBuffer));
		};
		REQUIRE_NOTHROW(fn());
		
	}

	SECTION("ReadStreamPart")
	{
		auto fn = [&]()
		{
			pStream->setPosition(sizeof(pBuffer) / 2);
			Size nReadSize = 0;
			pStream->read(pBuffer, sizeof(pBuffer), &nReadSize);
			REQUIRE(nReadSize == sizeof(pBuffer) / 2);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamTwise")
	{
		auto fn = [&]()
		{
			Size nReadSize = 0;
			pStream->setPosition(0);
			pStream->read(pBuffer, sizeof(pBuffer) / 2, &nReadSize);
			REQUIRE(nReadSize == sizeof(pBuffer) / 2);
			pStream->read(pBuffer, sizeof(pBuffer) / 2, &nReadSize);
			REQUIRE(nReadSize == sizeof(pBuffer) / 2);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadEndOfStream")
	{
		auto fn = [&]()
		{
			pStream->setPosition(sizeof(pBuffer));
			Size nReadSize = 0;
			pStream->read(pBuffer, sizeof(pBuffer), &nReadSize);
			REQUIRE(nReadSize == 0);
		};
		REQUIRE_NOTHROW(fn());
		
	}

	SECTION("ReadStreamOversize")
	{
		auto fn = [&]()
		{
			pStream->setPosition(0);
			Size nReadSize = 0;
			pStream->read(pBuffer, sizeof(pBuffer) * 2, &nReadSize);
			REQUIRE(nReadSize == sizeof(pBuffer));
		};
		REQUIRE_NOTHROW(fn());
	}
}


TEST_CASE("MemoryStream.extDataWrite", "[cmd::io]")
{
	Byte pBuffer[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };
	Byte pMemBuffer[sizeof(pBuffer)];
	ObjPtr<io::IWritableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = queryInterface<io::IWritableStream>(io::createMemoryStream(pMemBuffer, sizeof(pMemBuffer)));
	}());

	SECTION("Write")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, sizeof(pBuffer));
			REQUIRE(pStream->getSize() == sizeof(pMemBuffer));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("WriteStreamTwise")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, sizeof(pBuffer) / 2);
			REQUIRE(pStream->getPosition() == sizeof(pBuffer) / 2);
			REQUIRE(pStream->getSize() == sizeof(pMemBuffer));
			pStream->write(pBuffer, sizeof(pBuffer));
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}
}

TEST_CASE("MemoryStream.size", "[cmd::io]")
{
	ObjPtr<io::IWritableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = queryInterface<io::IWritableStream>(createObject(CLSID_MemoryStream));
	}());

	streamSize(pStream);
}
