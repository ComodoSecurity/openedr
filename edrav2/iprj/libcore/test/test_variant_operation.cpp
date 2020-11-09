//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (14.02.2019)
// Reviewer:
//

#include "pch.h"

using namespace openEdr;
using namespace openEdr::variant::operation;


//
//
//
CMD_DECLARE_LIBRARY_CLSID(CLSID_TestObject, 0x12345678);
class TestObject : public ObjectBase<CLSID_TestObject>, public ICommandProcessor
{
public:
	// ICommandProcessor
	Variant execute(Variant vCommand, Variant vParams) override
	{
		if (vCommand == "echo")
			return vParams;
		return nullptr;
	}
};

//
//
//
TEST_CASE("Variant.operation_logicalNot")
{
	auto fnScenario = [&](Variant vValue, bool fEthalon)
	{
		bool fResult = logicalNot(vValue);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, bool>> mapData {
		// sName: vValue1, vValue2, fEthalon
		{ "true", { true, false } },
		{ "false", { false, true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_logicalAnd")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = logicalAnd(vValue1, vValue2);
		REQUIRE(fResult == fEthalon);
		fResult = logicalAnd(vValue2, vValue1);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// sName: vValue1, vValue2, fEthalon
		{ "true_true", { true, true, true } },
		{ "true_false", { true, false, false } },
		{ "false_false", { true, false, false } },
		{ "boolean_other", { true, 42, true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_logicalOr")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = logicalOr(vValue1, vValue2);
		REQUIRE(fResult == fEthalon);
		fResult = logicalOr(vValue2, vValue1);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// name: vValue1, vValue2, fEthalon
		{ "true_true", { true, true, true } },
		{ "true_false", { true, false, true } },
		{ "false_false", { false, false, false } },
		{ "boolean_other", { true, 42, true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_equal")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = equal(vValue1, vValue2);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// sName: vValue1, vValue2, fEthalon
		{ "dictionary_true", { Dictionary({ { "data", 42 } }), 42, true } },
		{ "dictionary_false", { Dictionary({ { "data", 42 } }), nullptr, false } },
		{ "sequence_true", { Sequence({ 42 }), 42, true } },
		{ "sequence_false", { Sequence({ 42 }), nullptr, false } },
		{ "boolean_true", { true, true, true } },
		{ "boolean_false", { true, nullptr, false } },
		{ "integer_true", { 42, 42, true } },
		{ "integer_false", { 42, nullptr, false } },
		{ "string_true", { "42", 42, true } },
		{ "string_false", { "42", Dictionary(), false } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_greater")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = greater(vValue1, vValue2);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// sName: vValue1, vValue2, fEthalon
		// Omit checks for different string conversion because it's done by Equal_* cases.
		{ "string_integer", { "442", 42, true } },
		{ "integer_string", { 42, "442", false } },
		// Do simple check here for other types of comparison because it's already
		// covered by Variant's own unittests.
		{ "integer_integer", { 442, 42, true } },
		{ "integer_other", { 42, nullptr, true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_less")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = less(vValue1, vValue2);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// sName: vValue1, vValue2, fEthalon
		// Omit checks for different string conversion because it's done by Equal_* cases.
		{ "string_integer", { "442", 42, false } },
		{ "integer_string", { 42, "422", true } },
		// Do simple check here for other types of comparison because it's already
		// covered by Variant's own unittests.
		{ "integer_integer", { 42, 442, true } },
		{ "integer_other", { 42, nullptr, false } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_match")
{
	auto fnScenario = [&](Variant vValue, Variant vPattern, bool fEthalon)
	{
		bool fResult = match(vValue, vPattern);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// sName: vValue, vPattern, fEthalon
		{ "null_null", { nullptr, nullptr, false } },
		{ "notnull_null", { 42, nullptr, true } },
		{ "null_notnull", { nullptr, 42, false } },
		{ "x_other", { "42", 42, false } },
		{ "other_x", { 42, "42", false } },
		{ "string_string_1", { "xyz", "x.*", true } },
		{ "string_string_2", { "xyz", "42.*", false } },
		{ "string_string_blank_1", { "", "", true } },
		{ "string_string_blank_2", { "xyz", "", true } },
		{ "string_string_blank_3", { "", ".", false } },
		{ "string_sequence_1", { "xyz", Sequence({ "x.*", "42" }), true } },
		{ "string_sequence_2", { "xyz", Sequence({ "42.*", "42" }), false } },
		{ "string_sequence_blank_1", { "xyz", Sequence(), true } },
		{ "string_sequence_blank_2", { "xyz", Sequence({ "", nullptr }), true } },
		{ "string_sequence_blank_3", { "xyz", Sequence({ "", ".*42" }), false } },
		{ "sequence_string_1", { Sequence({ "42", "xyz" }), "x.*", true } },
		{ "sequence_string_2", { Sequence({ "42", "xyz" }), "zyx.*", false } },
		{ "sequence_sequence_1", { Sequence({ "42", "xyz" }), Sequence({ "", "42" }), true } },
		{ "sequence_sequence_2", { Sequence({ "", "xyz" }), Sequence({ "42.*", nullptr }), false } },
		{ "sequence_sequence_blank_1", { Sequence({ "42", "xyz" }), Sequence({ "", nullptr }), true } },
		{ "sequence_sequence_blank_2", { Sequence({ "", "xyz" }), Sequence({ "", ".*42" }), false } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vPattern, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vPattern, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_contain")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = contain(vValue1, vValue2);
		REQUIRE(fResult == fEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData {
		// sName: vValue1, vValue2, fEthalon
		{ "string_string_1", { "xyzyx", "yz", true } },
		{ "string_string_2", { "xyZyx", "yz", false } },
		{ "string_string_3", { "xyZyx", "", false } },
		{ "string_string_4", { "", "42", false } },
		{ "string_sequence_1", { "xyzyx",  Sequence({ 42, "xy", false }), true } },
		{ "string_sequence_2", { "xytruex",  Sequence({ 42, "xy", false }), true } },
		{ "string_sequence_3", { "xy42x",  Sequence({ 42, "xy", false }), true } },
		{ "string_sequence_4", { "xy42x",  Sequence(), false } },
		{ "string_other", { "xyz", Dictionary(), false } },
		{ "sequence_boolean_1", { Sequence({ 42, "xyz", false }), false, true } },
		{ "sequence_boolean_2", { Sequence(), true, false } },
		{ "sequence_integer_1", { Sequence({ 42, "xyz", false }), 42, true } },
		{ "sequence_integer_1", { Sequence({ 42, "xyz", false }), 42, true } },
		{ "sequence_null_1", { Sequence({ 42, "xyz", nullptr }), nullptr, true } },
		{ "sequence_null_2", { Sequence({ 42, "xyz", "" }), nullptr, true } },
		{ "sequence_null_3", { Sequence({ 42, "xyz" }), nullptr, false } },
		{ "sequence_string_1", { Sequence({ 42, "xyz", "" }), "yz", false } },
		{ "sequence_string_2", { Sequence({ 42, "xyz", "" }), "", true } },
		{ "sequence_string_3", { Sequence({ 42, "xyz", "" }), "absent", false } },
		{ "sequence_string_4", { Sequence({ 42, "xyz", "" }), "xyz", true } },
		{ "sequence_sequence_1", { Sequence({ 42, "xyz", "" }), Sequence(), false } },
		{ "sequence_sequence_2", { Sequence({ 42, "xyz", "" }), Sequence({ "" }), true } },
		{ "sequence_sequence_3", { Sequence(), Sequence({ 42 }), false } },
		{ "sequence_sequence_integer_1", { Sequence({ 42, "xyz", "" }), Sequence({ "zyx", 42 }), true } },
		{ "sequence_sequence_integer_2", { Sequence({ 42, "xyz", "" }), Sequence({ "zyx", 442 }), false } },
		{ "sequence_sequence_boolean_1", { Sequence({ 42, "xyz", false }), Sequence({ false, 422 }), true } },
		{ "sequence_sequence_boolean_2", { Sequence({ 42, "xyz", false }), Sequence({ 442, true }), false } },
		{ "sequence_sequence_null_1", { Sequence({ 42, "xyz", nullptr, false }), Sequence({ 442, nullptr }), true } },
		{ "sequence_sequence_null_2", { Sequence({ 42, "xyz", false }), Sequence({ nullptr, 442 }), true } },
		{ "sequence_sequence_string_1", { Sequence({ 42, "xyz", "" }), Sequence({ "yz" }), false } },
		{ "sequence_sequence_string_2", { Sequence({ 42, "xyz", "" }), Sequence({ "absent" }), false } },
		{ "sequence_sequence_string_3", { Sequence({ 42, "xyz", "" }), Sequence({ "xyz" }), true } },
		{ "sequence_other", { Sequence({ 42, "xyz", "" }), Dictionary(), false } },
		{ "other_1", { 42, "xyz", false } },
		{ "other_2", { 42, "42", true } },
		{ "other_sequence_1", { 42, Sequence({ 42, "xyz", "" }), true } },
		{ "other_sequence_2", { 88, Sequence({ 42, "xyz", "" }), false } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, fEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_replace")
{
	auto fnScenario = [&](Variant vValue, Variant vSchema, Variant vExpected)
	{
		Variant vResult = replace(vValue, vSchema);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vValue, vExpected, vSchema
		{ "string_1", { "4zzz2", "4xyz2",
			Dictionary({ { "target", "zz" }, { "value", "xy" } })
			} },
		{ "string_2", { "4zzz2", "88xyz2", Sequence({
			Dictionary({ { "target", "zz" }, { "value", "xy" } }),
			Dictionary({ { "target", "4" }, { "value", "88" } })
			}) } },
		{ "string_3", { "4zzz2", "4z888", Sequence({
			Dictionary({ { "target", "zz" }, { "value", "xy" } }),
			Dictionary({ { "target", "zz2", }, { "value", L"888" } })
			}) } },
		{ "string_4", { L"4zzz2", "4xyz2",
			Dictionary({ { "target", "zz" }, { "value", "xy" } })
			} },
		{ "string_blank", { "", "",
			Dictionary({ { "target", "zz" }, { "value", "xy" } }) 
			} },
		{ "sequence_1", { Sequence({ "4zzz2" }), Sequence({ "4xyz2" }),
			Dictionary({ { "target", "zz" }, { "value", "xy" } })
			} },
		{ "sequence_2", { Sequence({ "888xzzxx", 42, "4zzz2" }),
			Sequence({ "888xxyxx", 42, "4xyz2" }),
			Dictionary({ { "target", "zz" }, { "value", "xy" } })
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vExpected, vSchema] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vSchema, vExpected));
		}
	}

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapBad{
		// sName: vValue, vExpected, vSchema
		{ "string_blank", { "4zzz2", "4zzz2", Dictionary() } },
		{ "other_schema", { "4zzz2", "4zzz2", "" } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [vValue, vExpected, vSchema] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_AS(fnScenario(vValue, vSchema, vExpected), error::InvalidArgument);
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_add")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, Variant vEthalon)
	{
		Variant vResult = add(vValue1, vValue2);
		REQUIRE(vResult == vEthalon);
		if (vEthalon.isString())
		{
			Variant::StringValuePtr pEthalonStrValue = vEthalon;
			Variant::StringValuePtr pResultStrValue = vResult;
			REQUIRE(pResultStrValue->hasWChar() == pEthalonStrValue->hasWChar());
			REQUIRE(pResultStrValue->hasUtf8() == pEthalonStrValue->hasUtf8());
		}
	};

	const std::map<const std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vValue1, vValue2, vEthalon
		{ "null_1", { nullptr, 42, 42 } },
		{ "null_2", { 42, nullptr, 42 } },
		{ "null_3", { nullptr, nullptr, nullptr } },
		{ "integer_unsigned_unsigned", { 42, 100, 142 } },
		{ "integer_unsigned_signed", { 42, -100, -58 } },
		{ "integer_signed_unsigned", { -42, 100, 58 } },
		{ "integer_signed_signed", { -42, -100, -142 } },
		{ "integer_string", { -42, "100", 58 } },
		// TODO: Fix when an overflow detection will be implemented
		{ "integer_overflow", { 9999999999999999999u, 9999999999999999999u, 1553255926290448382u } },
		{ "string_1", { "xy", "z", "xyz" } },
		{ "string_2", { "xyz", "", "xyz" } },
		{ "string_3", { "xyz", 42, "xyz42" } },
		{ "string_4", { "xyz", L"42", "xyz42" } },
		{ "wstring_1", { L"xyz", 42, L"xyz42" } },
		{ "wstring_2", { L"xyz", "42", L"xyz42" } },
		{ "wstring_3", { L"", "xyz", L"xyz" } },
		{ "boolean_1", { true, true, true } },
		{ "boolean_2", { true, false, true } },
		{ "boolean_3", { false, true, true } },
		{ "boolean_4", { false, false, false } },
		{ "boolean_5", { false, 1, true } },
		{ "dictionary_1", { 
			Dictionary({ { "a", "42" } }),
			Dictionary({ { "b", "xyz" } }), 
			Dictionary({ { "a", "42" }, { "b", "xyz" } }),
			} },
		{ "dictionary_2", { 
			Dictionary({ { "a", "42" } }),
			"xyz", 
			Dictionary({ { "a", "42" }, { "data", "xyz" } }),
			} },
		{ "dictionary_3", { 
			Dictionary({ { "data", "42" } }),
			"xyz", 
			Dictionary({ { "data", "xyz" } }),
			} },
		{ "sequence_1", {
			Sequence({ 42, "" }),
			Sequence({ "xyz" }),
			Sequence({ 42, "", "xyz" }),
			} },
		{ "sequence_2", {
			Sequence({ 42, "" }),
			"xyz",
			Sequence({ 42, "", "xyz" }),
			} },
		{ "object", { createObject<TestObject>(), 42, nullptr } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue1, vValue2, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_extract")
{
	auto fnScenario = [&](Variant vData, std::string_view sPath, Variant vEthalon)
	{
		Variant vResult = extract(vData, sPath);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, std::string, Variant>> mapData{
		// sName: vData, sPath, vEthalon
		// Do simple check here because it's already covered by unittests
		// of getByPath() function.
		{ "dictionary", { Dictionary({ { "xyz", 42 } }), "xyz", 42 } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, sPath, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, sPath, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.operation_transform")
{
	auto fnScenario = [&](Variant vData, Variant vSchema, Variant vEthalon)
	{
		Variant vResult = transform(vData, vSchema, TransformSourceMode::CloneSource);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vData, vSchema, vEthalon
		{ "value_update", {
			Dictionary({ { "a", "xyz" } }),
			Dictionary({ { "item", "a" }, { "value", 42 } }),
			Dictionary({ { "a", 42 } })
			} },
		{ "value_add", {
			Dictionary({ { "a", "xyz" } }),
			Dictionary({ { "item", "b" }, { "value", 42 } }),
			Dictionary({ { "a", "xyz" }, { "b", 42 } })
			} },
		{ "value_delete", {
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "a" }, { "value", nullptr } }),
			Dictionary({ { "b", 42 } })
			} },
		{ "localPath", {
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "a" }, { "localPath", "b" } }),
			Dictionary({ { "a", 42 }, { "b", 42 } })
			} },
		{ "localPath_null", {
			Dictionary({ { "a", "xyz" }, { "b", {} } }),
			Dictionary({ { "item", "a" }, { "localPath", "b" } }),
			Dictionary({ { "a", "xyz" }, { "b", {} } })
			} },
		{ "localPath_null_default", {
			Dictionary({ { "a", "xyz" }, { "b", {} } }),
			Dictionary({ { "item", "a" }, { "localPath", "b" }, { "default", 24 } }),
			Dictionary({ { "a", 24 }, { "b", {} } })
			} },
		{ "localPath_absent", {
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "a" }, { "localPath", "absent" } }),
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			} },
		{ "localPath_absent_default", {
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "a" }, { "localPath", "absent" }, { "default", 24 } }),
			Dictionary({ { "a", 24 }, { "b", 42 } }),
			} },
		{ "command", {
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "b" }, { "command", Dictionary({
					{ "clsid", "0xACE892B6" },
					{ "command", "echo" },
					{ "processor", createObject<TestObject>() },
					{ "params", "xyz" }
					})
				} }),
			Dictionary({ { "a", "xyz" }, { "b", "xyz" } }),
			} },
		{ "sequence", {
			Dictionary({ { "a", "xyz" } }),
			Sequence({
				Dictionary({ { "item", "b" }, { "value", 42 } }),
				Dictionary({ { "item", "a" }, { "value", false } }),
				}),
			Dictionary({ { "a", false }, { "b", 42 } })
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, vSchema, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, vSchema, vEthalon));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant, Variant>> mapBad{
		// sName: sMsg, vData, vSchema, vEthalon
		{ "data_bad_type", { "Value must be a dictionary or a sequence",
			nullptr, nullptr, nullptr
			} },
		{ "schema_bad_type", { "Schema must be a dictionary or sequence",
			Dictionary(), nullptr, nullptr
			} },
		{ "rule_path", { "Missing field <item> in a rule",
			Dictionary(), Dictionary(), nullptr
			} },
		{ "rule_missing_param", { "Missing value-related field (<value>, <localPath>, <catalogPath>, <command>)",
			Dictionary(), Dictionary({ { "item", "" } }), nullptr
			} },
		{ "command_bad", { "Invalid dictionary index <processor>",
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "b" }, { "command", Dictionary({
					{ "clsid", "0xACE892B6" },
					{ "command", "echo" },
					}) }
				}),
			Dictionary({ { "a", "xyz" }, { "b", "xyz" } }),
			} },
		{ "localPath_InvalidArgument", { "Invalid Variant path format <[>",
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			Dictionary({ { "item", "a" }, { "localPath", "[" } }),
			Dictionary({ { "a", "xyz" }, { "b", 42 } }),
			} },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vData, vSchema, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vData, vSchema, vEthalon), sMsg);
		}
	}
}
