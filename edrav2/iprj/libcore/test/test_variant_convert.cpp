//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (20.02.2019)
// Reviewer:
//

#include "pch.h"

using namespace openEdr;

//
//
//
class SimpleTestObjectWithClsid12345678 : public ObjectBase<ClassId(0x12345678)>
{
};

//
//
//
TEST_CASE("Variant.convert_Boolean")
{
	auto fnScenario = [&](Variant vValue, Variant vEthalon)
	{
		Variant vResult = variant::convert<variant::ValueType::Boolean>(vValue);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vValue, vEthalon
		{ "dictionary_1", { Dictionary({ { "data", 42 } }), true } },
		{ "dictionary_2", { Dictionary(), false } },
		{ "sequence_1", { Sequence({ 42 }), true } },
		{ "sequence_2", { Sequence(), false } },
		{ "null", { {}, false } },
		{ "integer_1", { 42, true } },
		{ "integer_2", { 0, false } },
		{ "integer_3", { -42, true } },
		{ "string_1", { "", false } },
		{ "string_2", { "42", true } },
		{ "wstring_1", { L"", false } },
		{ "wstring_2", { L"42", true } },
		{ "boolean_true", { true, true } },
		{ "boolean_false", { false, false } },
		{ "object_true", { createObject<SimpleTestObjectWithClsid12345678>(), true } },
		{ "object_false", { Variant(ObjPtr<IObject>()), false } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.convert_Integer")
{
	auto fnScenario = [&](Variant vValue, Variant vEthalon)
	{
		Variant vResult = variant::convert<variant::ValueType::Integer>(vValue);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vValue, vEthalon
		{ "dictionary_1", { Dictionary({ { "data", 42 }, { "data2", 42 } }), 2 } },
		{ "dictionary_2", { Dictionary(), 0 } },
		{ "sequence_1", { Sequence({ 42 }), 1 } },
		{ "sequence_2", { Sequence(), 0 } },
		{ "null", { {}, 0 } },
		{ "integer_1", { 42, 42 } },
		{ "integer_2", { 0, 0 } },
		{ "integer_3", { -42, -42 } },
		{ "string_1", { "", 0 } },
		{ "string_2", { "42", 42 } },
		{ "string_3", { "42.33", 42 } },
		{ "string_4", { "42.33 xyz", 42 } },
		{ "string_5", { "xyz", 0 } },
		{ "string_oct", { "052", 42 } },
		{ "string_hex_1", { "0x2a", 42 } },
		{ "string_hex_2", { "0X2A", 42 } },
		{ "wstring_1", { L"", 0} },
		{ "wstring_2", { L"-42", -42 } },
		{ "wstring_3", { L"-42.33", -42 } },
		{ "boolean_true", { true, 1 } },
		{ "boolean_false", { false, 0 } },
		{ "object_1", { createObject<SimpleTestObjectWithClsid12345678>(), 1 } },
		{ "object_0", { Variant(ObjPtr<IObject>()), 0 } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.convert_String")
{
	auto fnScenario = [&](Variant vValue, Variant vEthalon)
	{
		Variant vResult = variant::convert<variant::ValueType::String>(vValue);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vValue, vEthalon
		{ "dictionary_1", { Dictionary({ { "data", 42 }, { "data2", 42 } }), "{}" } },
		{ "dictionary_2", { Dictionary(), "{}" } },
		{ "sequence_1", { Sequence({ 42 }), "[]" } },
		{ "sequence_2", { Sequence(), "[]" } },
		{ "null", { {}, "" } },
		{ "integer_1", { 42, "42" } },
		{ "integer_2", { 0, "0" } },
		{ "integer_3", { -42, "-42" } },
		{ "string_1", { "", "" } },
		{ "string_2", { "42", "42" } },
		{ "wstring_1", { L"", L"" } },
		{ "wstring_2", { L"-42", L"-42" } },
		{ "wstring_3", { L"-42.33", L"-42.33" } },
		{ "boolean_true", { true, "true" } },
		{ "boolean_false", { false, "false" } },
		{ "object", { createObject<SimpleTestObjectWithClsid12345678>(), "0x12345678" } },
		{ "object_null", { Variant(ObjPtr<IObject>()), "" } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.convert_Dictionary")
{
	auto fnScenario = [&](Variant vValue, Variant vEthalon)
	{
		Variant vResult = variant::convert<variant::ValueType::Dictionary>(vValue);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vValue, vEthalon
		{ "dictionary", { Dictionary({ { "data", 42 } }), Dictionary({ { "data", 42 } }) } },
		{ "null", { {}, Dictionary() } },
		{ "other", { 42, Dictionary({ { "data", 42 } }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.convert_Sequence")
{
	auto fnScenario = [&](Variant vValue, Variant vEthalon)
	{
		Variant vResult = variant::convert<variant::ValueType::Sequence>(vValue);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vValue, vEthalon
		{ "sequence", { Sequence({ "data", 42 }), Sequence({ "data", 42 }) } },
		{ "null", { {}, Sequence() } },
		{ "other", { 42, Sequence({ 42 }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vEthalon));
		}
	}
}
