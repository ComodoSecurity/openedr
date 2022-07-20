//
//  EDRAv2. Unit test
//    Streams.
//
#include "pch.h"
#include "test_stream.h"

using namespace cmd;

//
//
//
void readRawStream(ObjPtr<io::IRawReadableStream> pStream)
{
	SECTION("Read")
	{
		auto fn = [&]()
		{
			Byte pBuffer[c_nStreamSize];
			REQUIRE(pStream->read(pBuffer, c_nStreamSize) == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamPart")
	{
		auto fn = [&]()
		{
			Byte pBuffer[c_nStreamSize];
			REQUIRE(pStream->read(pBuffer, c_nStreamSize / 2) == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamTwise")
	{
		auto fn = [&]()
		{
			Byte pBuffer[c_nStreamSize];
			REQUIRE(pStream->read(pBuffer, c_nStreamSize / 2) == c_nStreamSize / 2);
			REQUIRE(pStream->read(pBuffer, c_nStreamSize / 2) == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamOversize")
	{
		auto fn = [&]()
		{
			Byte pBuffer[c_nStreamSize * 2];
			REQUIRE(pStream->read(pBuffer, c_nStreamSize * 2) == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
void testRawWritableStream(ObjPtr<io::IRawWritableStream> pStream)
{
	SECTION("Write")
	{
		auto fn = [&]()
		{
			Byte pBuffer[c_nStreamSize];
			pStream->write(pBuffer, c_nStreamSize);
			pStream->flush();
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("WriteStreamTwise")
	{
		auto fn = [&]()
		{
			Byte pBuffer[c_nStreamSize];
			pStream->write(pBuffer, c_nStreamSize);
			pStream->write(pBuffer, c_nStreamSize);
			pStream->flush();
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
void readStream(ObjPtr<io::IReadableStream> pStream)
{
	Byte pBuffer[c_nStreamSize];
	REQUIRE(pStream->getPosition() == 0);

	SECTION("Read")
	{
		auto fn = [&]()
		{
			REQUIRE(pStream->read(pBuffer, c_nStreamSize) == c_nStreamSize);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamPart")
	{
		auto fn = [&]()
		{
			REQUIRE(pStream->read(pBuffer, c_nStreamSize / 2) == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamTwise")
	{
		auto fn = [&]()
		{
			REQUIRE(pStream->read(pBuffer, c_nStreamSize / 2) == c_nStreamSize / 2);
			REQUIRE(pStream->read(pBuffer, c_nStreamSize / 2) == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadEndOfStream")
	{
		auto fn = [&]()
		{
			pStream->setPosition(c_nStreamSize);
			REQUIRE(pStream->read(pBuffer, c_nStreamSize) == 0);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("ReadStreamOversize")
	{
		auto fn1 = [&]()
		{
			pStream->setPosition(0);
			REQUIRE(pStream->read(pBuffer, sizeof(pBuffer) * 2) == sizeof(pBuffer));
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			pStream->setPosition(c_nStreamSize / 2);
			REQUIRE(pStream->read(pBuffer, c_nStreamSize) == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn2());
	}
}

//
//
//
void writeStream(ObjPtr<io::IWritableStream> pStream)
{
	Byte pBuffer[c_nStreamSize];
	REQUIRE(pStream->getPosition() == 0);

	SECTION("Write")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, c_nStreamSize);
			pStream->flush();
			REQUIRE(pStream->getSize() == c_nStreamSize);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("WriteStreamTwise")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, c_nStreamSize);
			pStream->write(pBuffer, c_nStreamSize);
			pStream->flush();
			REQUIRE(pStream->getSize() == c_nStreamSize * 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize * 2);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("WriteStreamTwiseOverwrite")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, c_nStreamSize);
			pStream->setPosition(c_nStreamSize / 2);
			pStream->write(pBuffer, c_nStreamSize);
			pStream->flush();
			REQUIRE(pStream->getSize() == c_nStreamSize * 1.5);
			REQUIRE(pStream->getPosition() == c_nStreamSize * 1.5);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("writeToTheMidleOfStream")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, sizeof(pBuffer));
			pStream->setPosition(0);
			pStream->write("@", 1);
			pStream->flush();
			REQUIRE(pStream->getSize() == sizeof(pBuffer));
			REQUIRE(pStream->getPosition() == 1);
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
void streamPosition(ObjPtr<io::IReadableStream> pStream)
{
	static const io::IoOffset nOffset = static_cast<io::IoOffset>(c_nStreamSize);

	// Test size for IReadableStream indirectly
	REQUIRE(pStream->getSize() == c_nStreamSize);

	SECTION("getPosition")
	{
		auto fn = [&]()
		{
			REQUIRE(pStream->getPosition() == 0);
		};
		REQUIRE_NOTHROW(fn());
	}


	SECTION("setPositionDefault")
	{
		auto fn1 = [&]()
		{
			REQUIRE(pStream->setPosition(nOffset / 2) == 0);
			REQUIRE(pStream->getPosition() == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			REQUIRE(pStream->setPosition(nOffset) == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			REQUIRE(pStream->setPosition(0) == c_nStreamSize);
			REQUIRE(pStream->getPosition() == 0);
		};
		REQUIRE_NOTHROW(fn3());
	}

	SECTION("setPositionDefaultFail")
	{
		auto fn = [&]()
		{
			pStream->setPosition(nOffset + 1);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidArgument);
	}


	SECTION("setPositionBegin")
	{
		auto fn1 = [&]()
		{
			REQUIRE(pStream->setPosition(nOffset / 2, io::StreamPos::Begin) == 0);
			REQUIRE(pStream->getPosition() == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			REQUIRE(pStream->setPosition(nOffset, io::StreamPos::Begin) == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			REQUIRE(pStream->setPosition(0, io::StreamPos::Begin) == c_nStreamSize);
			REQUIRE(pStream->getPosition() == 0);
		};
		REQUIRE_NOTHROW(fn3());
	}

	SECTION("setPositionBeginFail")
	{
		auto fn1 = [&]()
		{
			pStream->setPosition(nOffset + 1, io::StreamPos::Begin);
		};
		REQUIRE_THROWS_AS(fn1(), error::InvalidArgument);

		auto fn2 = [&]()
		{
			pStream->setPosition(-1, io::StreamPos::Begin);
		};
		REQUIRE_THROWS_AS(fn2(), error::InvalidArgument);
	}


	SECTION("setPositionEnd")
	{
		auto fn1 = [&]()
		{
			REQUIRE(pStream->setPosition(-nOffset / 2, io::StreamPos::End) == 0);
			REQUIRE(pStream->getPosition() == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			REQUIRE(pStream->setPosition(0, io::StreamPos::End) == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			REQUIRE(pStream->setPosition(-nOffset, io::StreamPos::End) == c_nStreamSize);
			REQUIRE(pStream->getPosition() == 0);
		};
		REQUIRE_NOTHROW(fn3());
	}

	SECTION("setPositionEndFail")
	{
		auto fn1 = [&]()
		{
			pStream->setPosition(-(nOffset + 1), io::StreamPos::End);
		};
		REQUIRE_THROWS_AS(fn1(), error::InvalidArgument);
	
		auto fn2 = [&]()
		{
			pStream->setPosition(1, io::StreamPos::End);
		};
		REQUIRE_THROWS_AS(fn2(), error::InvalidArgument);
	}

	SECTION("setPositionCurrent")
	{
		auto fn1 = [&]()
		{
			pStream->setPosition(nOffset / 2);
			REQUIRE(pStream->setPosition(nOffset / 2, io::StreamPos::Current) == nOffset / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn1());

		auto fn2 = [&]()
		{
			pStream->setPosition(nOffset / 2);
			REQUIRE(pStream->setPosition(-nOffset / 2, io::StreamPos::Current) == nOffset / 2);
			REQUIRE(pStream->getPosition() == 0);
		};
		REQUIRE_NOTHROW(fn2());

		auto fn3 = [&]()
		{
			pStream->setPosition(nOffset / 2);
			REQUIRE(pStream->setPosition(0, io::StreamPos::Current) == nOffset / 2);
			REQUIRE(pStream->getPosition() == nOffset / 2);
		};
		REQUIRE_NOTHROW(fn3());
	}

	SECTION("setPositionCurrentFail")
	{
		pStream->setPosition(nOffset / 2);

		auto fn1 = [&]()
		{
			pStream->setPosition(nOffset, io::StreamPos::Current);
		};
		REQUIRE_THROWS_AS(fn1(), error::InvalidArgument);

		auto fn2 = [&]()
		{
			pStream->setPosition(-nOffset, io::StreamPos::Current);
		};
		REQUIRE_THROWS_AS(fn2(), error::InvalidArgument);
	}
}

//
//
//
void streamSize(ObjPtr<io::IWritableStream> pStream)
{
	Byte pBuffer[c_nStreamSize];
	REQUIRE(pStream->getSize() == 0);

	SECTION("SetSize")
	{
		auto fn = [&]()
		{
			pStream->setSize(c_nStreamSize);
			REQUIRE(pStream->getSize() == c_nStreamSize);
			REQUIRE(pStream->getPosition() == c_nStreamSize);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("Resize")
	{
		auto fn = [&]()
		{
			pStream->write(pBuffer, c_nStreamSize);
			REQUIRE(pStream->getSize() == c_nStreamSize);
			pStream->setSize(c_nStreamSize / 2);
			REQUIRE(pStream->getSize() == c_nStreamSize / 2);
			REQUIRE(pStream->getPosition() == c_nStreamSize / 2);
		};
		REQUIRE_NOTHROW(fn());
	}

}