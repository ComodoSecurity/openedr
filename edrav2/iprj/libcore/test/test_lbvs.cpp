//
//  EDRAv2. Unit test
//    LBVS.
//
#include "pch.h"

using namespace openEdr;
using namespace openEdr::variant::lbvs;

enum class TestId : FieldId
{
	Value,
	Value1,
	Value2,
	D_Value,
	D_D_Value,
	Seq,
	Seq_Value,
	Seq_D_Value,
	Seq_Seq,
	Seq1,
	Seq2,
};

typedef variant::LbvsSerializer<TestId> LbvsSerializer;
typedef variant::LbvsDeserializer<TestId> LbvsDeserializer;

const char* c_sSchema = R"({
	"version": 1,
	"fields": [
		{ "name": "value" },
		{ "name": "value1" },
		{ "name": "value2" },
		{ "name": "d.value" },
		{ "name": "d.d.value" },
		{ "name": "seq[]" },
		{ "name": "seq[].value" },
		{ "name": "seq[].d.value" },
		{ "name": "seq[][]" },
		{ "name": "seq1[]" },
		{ "name": "seq2[]" }
	]})";

//
//
//
inline Variant deserialize(LbvsSerializer* pSerializer)
{
	void* pBuffer = nullptr;
	size_t nBufferSize = 0;
	REQUIRE(pSerializer->getData(&pBuffer, &nBufferSize));
	REQUIRE((pBuffer != nullptr && nBufferSize > 0));
	return variant::deserializeFromLbvs(pBuffer, nBufferSize, variant::deserializeFromJson(c_sSchema));
}

//
//
//
inline size_t deserialize(LbvsSerializer* pSerializer, LbvsDeserializer* pDeserializer)
{
	void* pBuffer = nullptr;
	size_t nBufferSize = 0;
	REQUIRE(pSerializer->getData(&pBuffer, &nBufferSize));
	REQUIRE(pDeserializer->reset(pBuffer, nBufferSize));
	return pDeserializer->getCount();
}
//
//
//
TEST_CASE("LBVS.string")
{
	static const char sStr[] = "Hello world!";
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), sStr));
 			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("value") == sStr);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Value));
			REQUIRE(strncmp(sStr, (char*)iter->data, iter->size) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("withSize")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), sStr, strlen(sStr)));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("value") == sStr);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Value));
			REQUIRE(strncmp(sStr, (char*)iter->data, iter->size) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("rewriteValue")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), sStr, strlen(sStr)));
			REQUIRE(serializer.write((TestId::Value), 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 2);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Value));
			REQUIRE(strncmp(sStr, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.wstring")
{
	static const wchar_t sStr[] = L"Hello world!";
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), sStr));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("value") == sStr);
	
			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::WString && iter->id == TestId::Value));
			REQUIRE(wcsncmp(sStr, (wchar_t*)iter->data, iter->size) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("withSize")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), sStr, wcslen(sStr)));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("value") == sStr);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::WString && iter->id == TestId::Value));
			REQUIRE(wcsncmp(sStr, (wchar_t*)iter->data, iter->size) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.buffer")
{
	static const uint8_t pBuffer[] = "Hello world!";
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), (void*)pBuffer, sizeof(pBuffer)));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Object);
			auto pStream = queryInterfaceSafe<io::IReadableStream>(vResult.get("value"));
			REQUIRE(pStream != nullptr);
			REQUIRE(pStream->getSize() == sizeof(pBuffer));
			
			auto pMemBuffer = queryInterfaceSafe<io::IMemoryBuffer>(pStream);
			REQUIRE(pMemBuffer != nullptr);
			auto Pair = pMemBuffer->getData();
			REQUIRE(Pair.second == sizeof(pBuffer));
			REQUIRE(0 == memcmp(pBuffer, Pair.first, sizeof(pBuffer)));
	
			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Stream && iter->id == TestId::Value));
			REQUIRE((sizeof(pBuffer) == iter->size && memcmp(pBuffer, iter->data, iter->size) == 0));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("empty")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE_FALSE(serializer.write((TestId::Value), (void*)nullptr, 1));
			REQUIRE(serializer.write((TestId::Value), (void*)nullptr, 0));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Object);
			auto pStream = queryInterfaceSafe<io::IReadableStream>(vResult.get("value"));
			REQUIRE(pStream != nullptr);
			REQUIRE(pStream->getSize() == 0);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Stream && iter->id == TestId::Value));
			REQUIRE(iter->size == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.bool")
{
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), true));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Boolean);
			REQUIRE(vResult.get("value") == true);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Bool && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(bool) && *(bool*)iter->data == true));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.uint32")
{
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write(TestId::Value, 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("byte")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write(TestId::Value, 0x80));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0x00000080);
	
			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0x00000080));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("negative")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), -1));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xFFFFFFFF);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xFFFFFFFF));
		};
		REQUIRE_NOTHROW(fn());
	}

	static const char sStr[] = "Hello world!";

	SECTION("rewriteValue")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::Value), sStr, strlen(sStr)));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("value") == sStr);
	
			REQUIRE(deserialize(&serializer, &deserializer) == 2);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Value));
			REQUIRE(strncmp(sStr, (char*)iter->data, iter->size) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.uint64")
{
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), 0xDEADBEAFDEADBEAFUI64));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xDEADBEAFDEADBEAFUI64);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint64 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint64_t) && *(uint64_t*)iter->data == 0xDEADBEAFDEADBEAFUI64));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("negative")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), -1I64));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.has("value"));
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xFFFFFFFFFFFFFFFFUI64);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint64 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint64_t) && *(uint64_t*)iter->data == 0xFFFFFFFFFFFFFFFFUI64));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.manyValues")
{
	static const char sStr[] = "Hello world!";
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::Value1), sStr));
			REQUIRE(serializer.write((TestId::Value2), (void*)sStr, sizeof(sStr)));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xDEADBEAF);
			REQUIRE(vResult.get("value1").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("value1") == sStr);
			REQUIRE(vResult.get("value2").getType() == variant::ValueType::Object);
			REQUIRE(queryInterfaceSafe<io::IReadableStream>(vResult.get("value2")) != nullptr);

			REQUIRE(deserialize(&serializer, &deserializer) == 3);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Value1));
			REQUIRE(strncmp(sStr, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Stream && iter->id == TestId::Value2));
			REQUIRE((sizeof(sStr) == iter->size && memcmp(sStr, iter->data, iter->size) == 0));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.dictionary")
{
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::D_Value), 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(!vResult.has("value"));
			REQUIRE(variant::getByPath(vResult, "d.value").getType() == variant::ValueType::Integer);
			REQUIRE(variant::getByPath(vResult, "d.value") == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("withValue")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::D_Value), 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xDEADBEAF);
			REQUIRE(variant::getByPath(vResult, "d.value").getType() == variant::ValueType::Integer);
			REQUIRE(variant::getByPath(vResult, "d.value") == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 2);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.dictionaryDictionary")
{
	LbvsDeserializer deserializer;

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::D_D_Value), 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(!vResult.has("value"));
			REQUIRE(variant::getByPath(vResult, "d.d.value").getType() == variant::ValueType::Integer);
			REQUIRE(variant::getByPath(vResult, "d.d.value") == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 1);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::D_D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("withValue")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::D_D_Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::D_Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::Value), 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("value").getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("value") == 0xDEADBEAF);
			REQUIRE(variant::getByPath(vResult, "d.value").getType() == variant::ValueType::Integer);
			REQUIRE(variant::getByPath(vResult, "d.value") == 0xDEADBEAF);
			REQUIRE(variant::getByPath(vResult, "d.d.value").getType() == variant::ValueType::Integer);
			REQUIRE(variant::getByPath(vResult, "d.d.value") == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 3);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::D_D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.sequence")
{
	LbvsDeserializer deserializer;

	SECTION("numbers")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Seq), 0x1234));
			REQUIRE(serializer.write((TestId::Seq), 0x4567));
			REQUIRE(serializer.write((TestId::Seq), 0x7890));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("seq").getType() == variant::ValueType::Sequence);
			REQUIRE(vResult.get("seq").getSize() == 3);
			REQUIRE(vResult.get("seq")[0].getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("seq")[0] == 0x1234);
			REQUIRE(vResult.get("seq")[1].getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("seq")[1] == 0x4567);
			REQUIRE(vResult.get("seq")[2].getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("seq")[2] == 0x7890);
	
			REQUIRE(deserialize(&serializer, &deserializer) == 3);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Seq));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0x1234));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Seq));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0x4567));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Seq));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0x7890));
		};
		REQUIRE_NOTHROW(fn());
	}

	static const char sStr1[] = "Hello world!";
	static const char sStr2[] = "It's my life";
	static const char sStr3[] = "White rabbit";
	
	SECTION("strings")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Seq), sStr1));
			REQUIRE(serializer.write((TestId::Seq), sStr2));
			REQUIRE(serializer.write((TestId::Seq), sStr3));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("seq").getType() == variant::ValueType::Sequence);
			REQUIRE(vResult.get("seq").getSize() == 3);
			REQUIRE(vResult.get("seq")[0].getType() == variant::ValueType::String);
			REQUIRE(vResult.get("seq")[0] == sStr1);
			REQUIRE(vResult.get("seq")[1].getType() == variant::ValueType::String);
			REQUIRE(vResult.get("seq")[1] == sStr2);
			REQUIRE(vResult.get("seq")[2].getType() == variant::ValueType::String);
			REQUIRE(vResult.get("seq")[2] == sStr3);

			REQUIRE(deserialize(&serializer, &deserializer) == 3);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Seq));
			REQUIRE(strncmp(sStr1, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Seq));
			REQUIRE(strncmp(sStr2, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Seq));
			REQUIRE(strncmp(sStr3, (char*)iter->data, iter->size) == 0);
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("mixed")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Seq), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::Seq), sStr1));
			REQUIRE(serializer.write((TestId::Seq), (void*)sStr2, sizeof(sStr2)));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("seq").getType() == variant::ValueType::Sequence);
			REQUIRE(vResult.get("seq").getSize() == 3);
			REQUIRE(vResult.get("seq")[0].getType() == variant::ValueType::Integer);
			REQUIRE(vResult.get("seq")[0] == 0xDEADBEAF);
			REQUIRE(vResult.get("seq")[1].getType() == variant::ValueType::String);
			REQUIRE(vResult.get("seq")[1] == sStr1);
			REQUIRE(vResult.get("seq")[2].getType() == variant::ValueType::Object);
			REQUIRE(queryInterfaceSafe<io::IReadableStream>(vResult.get("seq")[2]) != nullptr);

			REQUIRE(deserialize(&serializer, &deserializer) == 3);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Seq));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Seq));
			REQUIRE(strncmp(sStr1, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Stream && iter->id == TestId::Seq));
			REQUIRE((sizeof(sStr2) == iter->size && memcmp(sStr2, iter->data, iter->size) == 0));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
//
TEST_CASE("LBVS.sequenceDictionaries")
{
	LbvsDeserializer deserializer;

	SECTION("empty")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Seq)));
			REQUIRE(serializer.write((TestId::Seq)));
			REQUIRE(serializer.write((TestId::Seq)));
			Variant vResult = deserialize(&serializer);
			Variant vEthalon = Dictionary({ 
				{ "seq", Sequence({Dictionary(), Dictionary(), Dictionary()})}
			});

			REQUIRE(vResult == vEthalon);

			REQUIRE(deserialize(&serializer, &deserializer) == 3);
			for (auto& item : deserializer)
			{
				REQUIRE(item.id == TestId::Seq);
				REQUIRE(item.type == FieldType::SeqDict);
				REQUIRE(item.size == 0);
			}
		};
		REQUIRE_NOTHROW(fn());
	}

	static const char sStr1[] = "Hello world!";
	static const char sStr2[] = "It's my life";
	static const char sStr3[] = "White rabbit";

	SECTION("normal")
	{
		auto fn = [&]()
		{
			LbvsSerializer serializer;
			REQUIRE(serializer.write((TestId::Seq)));
			REQUIRE(serializer.write((TestId::Seq_Value), sStr1));
			REQUIRE(serializer.write((TestId::Seq_D_Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::Seq)));
			REQUIRE(serializer.write((TestId::Seq_D_Value), 0xDEADBEAF));
			REQUIRE(serializer.write((TestId::Seq_Value), sStr2));
			REQUIRE(serializer.write((TestId::Seq)));
// 			REQUIRE(serializer.write((TestId::Seq_Seq), sStr1));
// 			REQUIRE(serializer.write((TestId::Seq_Seq), 0xDEADBEAF));
			Variant vResult = deserialize(&serializer);
			REQUIRE(vResult.get("seq").getType() == variant::ValueType::Sequence);
			REQUIRE(vResult.get("seq").getSize() == 3);
			REQUIRE(vResult.get("seq")[0].getType() == variant::ValueType::Dictionary);
			REQUIRE(vResult.get("seq")[0].get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("seq")[0].get("value") == sStr1);
			REQUIRE(getByPath(vResult.get("seq")[0], "d.value").getType() == variant::ValueType::Integer);
			REQUIRE(getByPath(vResult.get("seq")[0], "d.value") == 0xDEADBEAF);
			REQUIRE(vResult.get("seq")[1].getType() == variant::ValueType::Dictionary);
			REQUIRE(vResult.get("seq")[1].get("value").getType() == variant::ValueType::String);
			REQUIRE(vResult.get("seq")[1].get("value") == sStr2);
			REQUIRE(getByPath(vResult.get("seq")[1], "d.value").getType() == variant::ValueType::Integer);
			REQUIRE(getByPath(vResult.get("seq")[1], "d.value") == 0xDEADBEAF);
// 			REQUIRE(vResult.get("seq")[2].getType() == variant::ValueType::Sequence);
// 			REQUIRE(vResult.get("seq")[2].getSize() == 2);
// 			REQUIRE(vResult.get("seq")[2][0] == variant::ValueType::String);
// 			REQUIRE(vResult.get("seq")[2][0] == sStr1);
// 			REQUIRE(vResult.get("seq")[2][1] == variant::ValueType::Integer);
// 			REQUIRE(vResult.get("seq")[2][1] == 0xDEADBEAF);

			REQUIRE(deserialize(&serializer, &deserializer) == 7);
			auto iter = deserializer.begin();
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::SeqDict && iter->id == TestId::Seq));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Seq_Value));
			REQUIRE(strncmp(sStr1, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Seq_D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::SeqDict && iter->id == TestId::Seq));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::Uint32 && iter->id == TestId::Seq_D_Value));
			REQUIRE((iter->size == sizeof(uint32_t) && *(uint32_t*)iter->data == 0xDEADBEAF));
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::String && iter->id == TestId::Seq_Value));
			REQUIRE(strncmp(sStr2, (char*)iter->data, iter->size) == 0);
			++iter;
			REQUIRE(iter != deserializer.end());
			REQUIRE((iter->type == FieldType::SeqDict && iter->id == TestId::Seq));
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
//
TEST_CASE("LBVS.serializeFromVariant")
{
	static const char sStr1[] = "Hello world!";
	static const char sStr2[] = "It's my life";
	static const char sStr3[] = "White rabbit";

// 		{ "name": "value" },
// 		{ "name": "value1" },
// 		{ "name": "value2" },
// 		{ "name": "d.value" },
// 		{ "name": "d.d.value" },
// 		{ "name": "seq[]" },
// 		{ "name": "seq[].value" },
// 		{ "name": "seq[].d.value" },
// 		{ "name": "seq[][]" }
// 		{ "name": "seq1[]" },
// 		{ "name": "seq2[]" },


	SECTION("normal")
	{
		auto vVariant(Dictionary({
			{"value", 0xDEADBEAF},
			{"d", Dictionary({
				{"value", 0xDEADBEAF},
				{"value1", 0xDEADBEAF},
				{"d", Dictionary({
					{"value", 0xDEADBEAF},
					{"value1", 0xDEADBEAF},
				})},
			})},
			{"value1", sStr1},
			{"seq", Sequence({
				Dictionary({ {"value", 0xDEADBEAF} }),
				Dictionary({ {"value", 0xDEADBEAF}, {"d", Dictionary({ {"value", 0xDEADBEAF} })} }),
				Dictionary({ {"value", 0xDEADBEAF}, {"value1", 0xDEADBEAF} }),
			})},
			{"value2", sStr2},
			{"seq1", Sequence({
				0x1234, 0x4567, 0x7890
			})},
			{"value3", sStr3},
			{"seq2", Sequence({
				sStr1, sStr2, sStr3
			})},
		}));

		auto fn = [&]()
		{
			Variant vSchema = variant::deserializeFromJson(c_sSchema);
			std::vector<uint8_t> vData;
			REQUIRE(!serializeToLbvs(vVariant, vSchema, vData));
			Variant vNewVariant(deserializeFromLbvs(vData.data(), vData.size(), vSchema));
			vVariant.erase("value3");
			vVariant.get("d").erase("value1");
			vVariant.get("d").get("d").erase("value1");
			Sequence vSeq(vVariant.get("seq"));
			vSeq.get(2).erase("value1");
			REQUIRE(vVariant == vNewVariant);
		};
		REQUIRE_NOTHROW(fn());
	}
}
