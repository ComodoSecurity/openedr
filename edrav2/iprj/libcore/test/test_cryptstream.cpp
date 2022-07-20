//
//  EDRAv2. Unit test
//    Stream serializer/deserializer.
//
#include "pch.h"
#include "test_stream.h"

using namespace cmd;

TEST_CASE("StreamSerializer.createObject")
{
	auto pStream = queryInterface<io::IWritableStream>(io::createMemoryStream());

	SECTION("default")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_StreamSerializer, Dictionary({
					{"stream", pStream}
				}));
			pObj.reset();
		}());
	}

	SECTION("full")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_StreamSerializer, Dictionary({
					{"stream", pStream},
					{"format", "json"},
					{"encryption", "aes"},
					{"compression", ""}
				}));
			pObj.reset();
		}());
	}

	SECTION("wrong")
	{
		REQUIRE_THROWS_AS([&]() {
			auto pObj = createObject(CLSID_StreamSerializer, Dictionary({
					{"stream", pStream},
					{"encryption", "rc4"}
				}));
			pObj.reset();
		}(), error::InvalidArgument);
	}

}

TEST_CASE("StreamSerializer.queryInterface")
{
	auto pStream = queryInterface<io::IWritableStream>(io::createMemoryStream());

	SECTION("IRawWritableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<io::IRawWritableStream>(createObject(CLSID_StreamSerializer, Dictionary({
					{"stream", pStream}
				})));
			pObj.reset();
		}());
	}

	SECTION("IDataReceiver")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_StreamSerializer, Dictionary({
					{"stream", pStream}
				})));
			pObj.reset();
		}());
	}
}

auto vData = Dictionary({
	{"int", 123},
	{"str", "Hello world!"},
	{"dict", Dictionary({ {"a", 1}, {"b", 2}, {"c", 3} })}
});

TEST_CASE("StreamSerializer.put")
{
	auto pStream = queryInterface<io::IWritableStream>(io::createMemoryStream());

	REQUIRE_NOTHROW([&]() {
		auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_StreamSerializer, Dictionary({
				{"stream", pStream}
			})));
		pObj->put(vData);
		pObj.reset();

		auto pReadStream = queryInterface<io::IReadableStream>(pStream);
		uint32_t nMagic = 0;
		pReadStream->setPosition(0);
		REQUIRE(pReadStream->read(&nMagic, sizeof(nMagic)) == sizeof(nMagic));
		REQUIRE(nMagic == '!RDE');
	}());
}


TEST_CASE("StreamDeserializer.createObject")
{
	auto pStream = queryInterface<io::IWritableStream>(io::createMemoryStream());
	variant::serialize(pStream, vData, Dictionary({ {"encryption", "aes"} }));
	pStream->setPosition(0);

	SECTION("default")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pStream}
				}));
			pObj.reset();
		}());
	}
}

TEST_CASE("StreamDeserializer.queryInterface")
{
	auto pStream = queryInterface<io::IWritableStream>(io::createMemoryStream());
	variant::serialize(pStream, vData, Dictionary({ {"encryption", "aes"} }));
	pStream->setPosition(0);

	SECTION("IRawReadableStream")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<io::IRawReadableStream>(createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pStream}
				})));
			pObj.reset();
		}());
	}

	SECTION("IDataProvider")
	{
		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<IDataProvider>(createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pStream}
				})));
			pObj.reset();
		}());
	}
}


enum Codec : uint8_t
{
	None = 0,
	Json = 1,
	Aes = 2,
	JsonPretty = 3,
};

static const uint8_t c_nHdrVersion = 2;

#pragma pack(push, 1)
struct StreamHeader
{
	uint32_t nMagic = '!RDE';
	uint8_t nVersion = c_nHdrVersion;
	// Version 1 fields
	Codec eFormat = Codec::None;
	Codec eCompession = Codec::None;
	Codec eEncryption = Codec::None;
	io::IoSize nSize = 0;
	// Version 2 fields
	crypt::crc32::Hash nChecksum = 0;
};
#pragma pack(pop)

void readHeader(ObjPtr<IObject> pStream, StreamHeader* pHdr)
{
	if (pHdr == nullptr)
		error::InvalidArgument().throwException();
	auto pReadStream = queryInterface<io::IReadableStream>(pStream);
	pReadStream->setPosition(0);
	if (pReadStream->read(pHdr, sizeof(StreamHeader)) != sizeof(StreamHeader))
		error::NoData().throwException();
}

void writeHeader(ObjPtr<IObject> pStream, StreamHeader* pHdr)
{
	if (pHdr == nullptr)
		error::InvalidArgument().throwException();
	auto pWriteStream = queryInterface<io::IWritableStream>(pStream);
	pWriteStream->setPosition(0);
	pWriteStream->write(pHdr, sizeof(StreamHeader));
}

TEST_CASE("StreamDeserializer.get")
{
	auto pStream = queryInterface<io::IWritableStream>(io::createMemoryStream());
	variant::serialize(pStream, vData, Dictionary({ {"encryption", "aes"} }));

	StreamHeader hdr;
	REQUIRE_NOTHROW(readHeader(pStream, &hdr));

	SECTION("valid")
	{
		auto fn = [&]()
		{
			pStream->setPosition(0);
			auto pObj = queryInterface<IDataProvider>(createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pStream}
				})));
			auto data = pObj->get();
			REQUIRE(data.has_value());
			REQUIRE(data.value() == vData);
			pObj.reset();
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("invalidVersion")
	{
		auto fn = [&]()
		{
			hdr.nVersion = hdr.nVersion + 1;
			writeHeader(pStream, &hdr);

			pStream->setPosition(0);
			auto pObj = queryInterface<IDataProvider>(createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pStream}
				})));
			pObj.reset();
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidFormat);
	}

	SECTION("invalidEncryption")
	{
		auto fn = [&]()
		{
			hdr.eEncryption = Codec::Json;
			writeHeader(pStream, &hdr);

			pStream->setPosition(0);
			auto pObj = queryInterface<IDataProvider>(createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pStream}
				})));
			pObj.reset();
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidFormat);
	}

	SECTION("invalidStreamSize1")
	{
		auto fn = [&]()
		{
			hdr.nSize -= 0x10;
			writeHeader(pStream, &hdr);

			auto pReadStream = queryInterface<io::IReadableStream>(pStream);
			pReadStream->setPosition(0);
			auto data = variant::deserialize(pReadStream);
			REQUIRE(data == vData);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidFormat);
	}

	SECTION("invalidStreamSize2")
	{
		auto fn = [&]()
		{
			hdr.nSize += 0x10;
			writeHeader(pStream, &hdr);

			auto pReadStream = queryInterface<io::IReadableStream>(pStream);
			pReadStream->setPosition(0);
			auto data = variant::deserialize(pReadStream);
			REQUIRE(data == vData);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("invalidStreamSize3")
	{
		auto fn = [&]()
		{
			// Concatenate some garbage
			uint32_t addData = 0x12345678;
			pStream->setPosition(pStream->getSize());
			pStream->write(&addData, sizeof(addData));

			auto pReadStream = queryInterface<io::IReadableStream>(pStream);
			pReadStream->setPosition(0);
			auto data = variant::deserialize(pReadStream);
			REQUIRE(data == vData);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("invalidStreamSize4")
	{
		auto fn = [&]()
		{
			pStream->setSize(pStream->getSize() - 0x10);

			auto pReadStream = queryInterface<io::IReadableStream>(pStream);
			pReadStream->setPosition(0);
			auto data = variant::deserialize(pReadStream);
			REQUIRE(data == vData);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidFormat);
	}

	SECTION("invalidChecksum1")
	{
		auto fn = [&]()
		{
			hdr.nChecksum = 0x12345678;
			writeHeader(pStream, &hdr);

			auto pReadStream = queryInterface<io::IReadableStream>(pStream);
			pReadStream->setPosition(0);
			auto data = variant::deserialize(pReadStream);
			REQUIRE(data == vData);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidFormat);
	}

	SECTION("invalidChecksum2")
	{
		auto fn = [&]()
		{
			// Overwrite some data with garbage
			uint32_t addData = 0x12345678;
			pStream->setPosition(sizeof(StreamHeader));
			pStream->write(&addData, sizeof(addData));

			auto pReadStream = queryInterface<io::IReadableStream>(pStream);
			pReadStream->setPosition(0);
			auto data = variant::deserialize(pReadStream);
			REQUIRE(data == vData);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidFormat);
	}
}

/***
TEST_CASE("CacheStream.readRawReadableStreamCache", "[cmd::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IRawReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createStreamCache(queryInterface<io::IRawReadableStream>(io::createMemoryStream(sizeof(pBuffer))));
	}());

	readRawStream(pStream);
}

TEST_CASE("CacheStream.readReadableStreamCache", "[cmd::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createStreamCache(io::createMemoryStream(sizeof(pBuffer)));
	}());

	readStream(pStream);
}

TEST_CASE("CacheStream.readRawReadableStreamConverter", "[cmd::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::convertStream(io::createMemoryStream(sizeof(pBuffer)));
	}());

	readStream(pStream);
}


TEST_CASE("CacheStream.positionReadableStreamCache", "[cmd::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::createStreamCache(io::createMemoryStream(sizeof(pBuffer)));
	}());

	streamPosition(pStream);
}

TEST_CASE("CacheStream.positionRawReadableStreamConverter", "[cmd::io]")
{
	Byte pBuffer[0x10];
	ObjPtr<io::IReadableStream> pStream;
	REQUIRE_NOTHROW([&]() {
		pStream = io::convertStream(io::createMemoryStream(sizeof(pBuffer)));
	}());

	streamPosition(pStream);
}
/***/
