//
//  EDRAv2. Unit test
//  io::StringReadableStream
//
#include "pch.h"
#include "test_stream.h"

using namespace openEdr;

TEST_CASE("StringReadableStream.finalConstruct")
{
	SECTION("simple")
	{
		std::string sData = "1234567890";

		auto pObject = createObject(CLSID_StringReadableStream, Dictionary({ {"data", sData} }));
		auto pStream = queryInterface<io::IReadableStream>(pObject);
		REQUIRE(pStream->getPosition() == 0);
		REQUIRE(sData.size() == pStream->getSize());

		for (Size i = 0; i < sData.size(); ++i)
		{
			CAPTURE(i);
			char ch;
			REQUIRE(pStream->read(&ch, sizeof(ch)) == sizeof(ch));
			REQUIRE(ch == sData[i]);
		}
	}

	SECTION("no_parameters")
	{
		REQUIRE_THROWS([]() {
			(void)createObject(CLSID_StringReadableStream);
		}());
	}
}

TEST_CASE("StringReadableStream.queryInterface")
{
	auto pObj = createObject(CLSID_StringReadableStream, Dictionary({ {"data", "12345"} }));

	SECTION("IRawReadableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pRawReadableStream = queryInterface<io::IRawReadableStream>(pObj);
		}());
	}

	SECTION("ISeekableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pSeekableStream = queryInterface<io::ISeekableStream>(pObj);
		}());
	}

	SECTION("IReadableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pReadableStream = queryInterface<io::IReadableStream>(pObj);
		}());
	}
}

TEST_CASE("StringReadableStream.read")
{
	auto pObject = createObject(CLSID_StringReadableStream, Dictionary({ {"data", std::string(c_nStreamSize, 'a')} }));
	auto pStream = queryInterface<io::IReadableStream>(pObject);
	readStream(pStream);
}

TEST_CASE("StringReadableStream.position")
{
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createMemoryStream(c_nStreamSize);
	}());

	streamPosition(pStream);
}

