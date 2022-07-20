//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"

using namespace cmd;

//////////////////////////////////////////////////////////////////////////
//
// Serialization
//

//
//
//
class ExampleNotSerializableObject: public ObjectBase <>
{
};

CMD_DECLARE_LIBRARY_CLSID(CLSID_ExampleSerializableObject, 0xD8656F52);

//
// Object without special interfaces
//
class ExampleSerializableObject : public ObjectBase <CLSID_ExampleSerializableObject>, 
	ISerializable
{
	Variant m_vCfg;

public:
	void finalConstruct(Variant vCfg)
	{
		m_vCfg = vCfg;
	}

	Variant getFinalConstructParameter() 
	{ 
		return m_vCfg; 
	}

	// variant::ISerializable
	virtual Variant serialize() override
	{
		auto vData = m_vCfg.clone();
		vData.put("$$clsid", getClassId());
		return vData;
	}

};

CMD_DEFINE_LIBRARY_CLASS(ExampleSerializableObject)


TEST_CASE("Variant.serializeToJson")
{
	SECTION("supported_JSON_types")
	{
		const Variant vEthalon = Dictionary({
			{"a", nullptr},
			{"b", true},
			{"c", false},
			{"d", 1},
			{"e", -1},
			{"f", "str"},
			{"g", Dictionary({{"a", 1}, {"b", false}}) },
			{"h", Sequence({ 1, "a" })}
			});

		std::string sSerialized = variant::serializeToJson(vEthalon);
		Variant vTest = cmd::variant::deserializeFromJson(sSerialized);
		REQUIRE(vTest == vEthalon);
	}

	SECTION("ISerializable_object")
	{
		const Variant vSrc = Dictionary({
			{"a", createObject<ExampleSerializableObject>(Dictionary({{"b",2}}))}
			});

		std::string sSerialized = variant::serializeToJson(vSrc);
		Variant vAfterDeserialization = variant::deserializeFromJson(sSerialized);
		Variant vObj = vAfterDeserialization["a"];

		auto pObj = queryInterface<ExampleSerializableObject>(vObj);
		REQUIRE(pObj->getFinalConstructParameter()["b"] == 2);
	}

	SECTION("IReadableStream_object")
	{
		std::vector<uint8_t> srcData{ 0,1,2,3,5 };
		auto pSrcStream = io::createMemoryStream(&srcData[0], srcData.size());
		pSrcStream->setPosition(2); // change position

		const Variant vSrc = Dictionary({
			{"a", pSrcStream}
			});

		std::string sSerialized = variant::serializeToJson(vSrc);
		Variant vAfterDeserialization = variant::deserializeFromJson(sSerialized);
		Variant vObj = vAfterDeserialization["a"];

		auto pObj = queryInterface<io::IReadableStream>(vObj);
		REQUIRE(pObj->getSize() == srcData.size());

		pObj->setPosition(0);
		std::vector<uint8_t> resData(srcData.size(), 0);
		REQUIRE(pObj->read(&resData[0], resData.size()) == resData.size());
		REQUIRE(resData == srcData);
	}

	SECTION("IReadableStream_object_empty")
	{
		const Variant vSrc = Dictionary({
			{"a", io::createMemoryStream(0)}
			});

		std::string sSerialized = variant::serializeToJson(vSrc);
		Variant vAfterDeserialization = variant::deserializeFromJson(sSerialized);
		Variant vObj = vAfterDeserialization["a"];

		auto pObj = queryInterface<io::IReadableStream>(vObj);
		REQUIRE(pObj->getSize() == 0);
	}

	SECTION("nonserializable_object")
	{
		const Variant vSrc = Dictionary({
			{"a", Variant(createObject<ExampleNotSerializableObject>())}
		});

		REQUIRE_THROWS_AS([&]() {
			std::string sSerialized = variant::serializeToJson(vSrc);
		}(), error::TypeError);
	}
}

TEST_CASE("Variant.deserializeFromJson")
{
	SECTION("supported_JSON_types")
	{
		const std::string_view sJson = R"json({
			"a": null,
			"b": true,
			"c": false,
			"d": 1,
			"e": -1,
			"f": "str",
			"g": {"a":1, "b":false},
			"h": [1, "a"]
		})json";

		const Variant vEthalon = Dictionary({
			{"a", nullptr},
			{"b", true},
			{"c", false},
			{"d", 1},
			{"e", -1},
			{"f", "str"},
			{"g", Dictionary({{"a", 1}, {"b", false}}) },
			{"h", Sequence({ 1, "a" })}
			});

		cmd::Variant vTest = cmd::variant::deserializeFromJson(sJson);
		REQUIRE(vTest == vEthalon);
	}

	SECTION("commentaries_support")
	{
		const std::string_view sJson = R"json({
			"a": null, // comment
			"b": true,
			// comment
			"c": false,
			"d": /*comment*/ 1,
			"e": -1,
			"f": "str", // comment // comment 
			"g": {"a":1, "b":false},
			"h": [1, "a"]
		})json";

		const Variant vEthalon = Dictionary({
			{"a", nullptr},
			{"b", true},
			{"c", false},
			{"d", 1},
			{"e", -1},
			{"f", "str"},
			{"g", Dictionary({{"a", 1}, {"b", false}}) },
			{"h", Sequence({ 1, "a" })}
			});

		cmd::Variant vTest = cmd::variant::deserializeFromJson(sJson);
		REQUIRE(vTest == vEthalon);
	}

	// Now values of fields which have an unsupported type are replace with null value 
	//SECTION("unsupported_JSON_types")
	//{
	//	std::string sJson = R"json({
	//		"a": 1,
	//		"b": 1.08,
	//		"c": "str",
	//		"d": {"a":1},
	//		"e": [1, "a"],
	//		"f": true,
	//		"g": null
	//	})json";

	//	REQUIRE_THROWS_AS([&]() {
	//		Variant vTest = variant::deserializeFromJson(sJson);
	//	}(), error::InvalidFormat);
	//}

	SECTION("invalid_JSON")
	{
		std::string sJson = R"json({
			:::: 1,
		})json";

		REQUIRE_THROWS_AS([&]() {
			Variant vTest = variant::deserializeFromJson(sJson);
		}(), error::InvalidFormat);
	}

	SECTION("object_deserialization_success")
	{
		const std::string_view sJson = R"json({
			"$$clsid": 3630526290,
			"a" : 1
		})json";

		Variant vTest = variant::deserializeFromJson(sJson);
		REQUIRE(vTest.isObject());
		ObjPtr<ExampleSerializableObject> pClass = vTest;
		REQUIRE(pClass->getFinalConstructParameter()["a"] == 1);
	}

	SECTION("object_deserialization_success_with_integer_clsid")
	{
		const std::string_view sJson = R"json({
			"$$clsid": 3630526290,
			"a" : 1
		})json";

		Variant vTest = variant::deserializeFromJson(sJson);
		REQUIRE(vTest.isObject());
		ObjPtr<ExampleSerializableObject> pClass = vTest;
		REQUIRE(pClass->getFinalConstructParameter()["a"] == 1);
	}

	SECTION("object_deserialization_success_with_string_clsid")
	{
		const std::string_view sJson = R"json({
			"$$clsid": "0xD8656F52",
			"a" : 1
		})json";

		Variant vTest = variant::deserializeFromJson(sJson);
		REQUIRE(vTest.isObject());
		ObjPtr<ExampleSerializableObject> pClass = vTest;
		REQUIRE(pClass->getFinalConstructParameter()["a"] == 1);
	}

	SECTION("object_deserialization_with_invalid_json")
	{
		const std::string_view sJson = R"json({
			"$$clsid": {},
			"a" : 1
		})json";

		REQUIRE_THROWS_AS([&]() {
			Variant vTest = variant::deserializeFromJson(sJson);
		}(), error::InvalidUsage);
	}

	SECTION("object_deserialization_with_invalid_integer_clsid")
	{
		const std::string_view sJson = R"json({
			"$$clsid": 0,
			"a" : 1
		})json";

		REQUIRE_THROWS_AS([&]() {
			Variant vTest = variant::deserializeFromJson(sJson);
		}(), error::InvalidArgument);
	}

	SECTION("object_deserialization_with_invalid_string_clsid")
	{
		const std::string_view sJson = R"json({
			"$$clsid": "RRRRR",
			"a" : 1
		})json";

		REQUIRE_THROWS_AS([&]() {
			Variant vTest = variant::deserializeFromJson(sJson);
		}(), error::InvalidArgument);
	}
}

TEST_CASE("Variant.serialize_to_stream")
{
	const Variant vSource = Dictionary({
		{"a", nullptr},
		{"b", true},
		{"c", false},
		{"d", 1},
		{"e", -1},
		{"f", "str"},
		{"g", Dictionary({{"a", 1}, {"b", false}}) },
		{"h", Sequence({ 1, "a" })}
		});

	auto pStream = queryInterface<io::IWritableStream>(createObject(CLSID_MemoryStream));

	pStream->setPosition(0);

	variant::serializeToJson(pStream, vSource);

	pStream->setPosition(0);
	Variant vResult = variant::deserializeFromJson(queryInterface<io::IRawReadableStream>(pStream));

	REQUIRE(vResult == vSource);
}