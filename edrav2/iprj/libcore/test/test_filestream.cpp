//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"
#include "test_stream.h"

using namespace cmd;

TEST_CASE("FileStream.createObject", "[cmd::io]")
{
	SECTION("Create object")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_File, Dictionary({ 
				{"path", io::getTempFileName().c_str()}, 
				{"mode", io::FileMode::Read | io::FileMode::Write | io::FileMode::Temp} }));
			pObj.reset();
		}());
	}
}

TEST_CASE("FileStream.queryInterface", "[cmd::io]")
{
	auto pObj = io::createTempStream();

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

	SECTION("IFile")
	{
		REQUIRE_NOTHROW([&]() {
			auto pWritableStream = queryInterface<io::IFile>(pObj);
		}());
	}

}

TEST_CASE("FileStream.read", "[cmd::io]")
{
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createTempStream();

		Byte pBuffer[c_nStreamSize];
		auto pObj = queryInterface<io::IWritableStream>(pStream);
		pObj->write(pBuffer, c_nStreamSize);
		pObj->setPosition(0);
	}());

	readStream(pStream);
}

TEST_CASE("FileStream.write", "[cmd::io]")
{
	ObjPtr<io::IWritableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = queryInterface<io::IWritableStream>(io::createTempStream());
	}());

	writeStream(pStream);
	testRawWritableStream(pStream);
}

TEST_CASE("FileStream.position", "[cmd::io]")
{
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createTempStream();

		Byte pBuffer[c_nStreamSize];
		auto pObj = queryInterface<io::IWritableStream>(pStream);
		pObj->write(pBuffer, c_nStreamSize);
		pObj->setPosition(0);
	}());

	streamPosition(pStream);
}

TEST_CASE("FileStream.size", "[cmd::io]")
{
	ObjPtr<io::IWritableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = queryInterface<io::IWritableStream>(io::createTempStream());
	}());

	streamSize(pStream);
}
