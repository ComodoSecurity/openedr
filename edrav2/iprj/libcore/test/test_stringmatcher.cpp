//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (27.02.2019)
// Reviewer:
//

#include "pch.h"

using namespace openEdr;

//
//
//
TEST_CASE("StringMatcher.create")
{
	auto fnScenario = [&](Variant vConfig)
	{
		auto pObj = createObject(CLSID_StringMatcher, vConfig);
	};

	const std::map<const std::string, std::tuple<Variant>> mapData{
		// sName: vConfig
		{ "schema_dictionary", { Dictionary({ { "schema", Dictionary({ { "target", "x" }, { "value", "y" }}) } }) } },
		{ "schema_sequence", { Dictionary({ { "schema", Sequence({
			Dictionary({ { "target", "x" }, { "value", "y" }}),
			Dictionary({ { "target", "z" }, { "value", "y" }})
			}) } }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant>> mapBad{
		// sName: sMsg, vConfig
		// FIXME: if behaviour of has() on non-dictionary will be changed
		{ "config_not_dictionary", { "has() is not applicable for current type <Integer>. Key: <schema>", 42 } },
		{ "missing_schema", { "Missing field <schema>", Dictionary() } },
		{ "invalid_schema", { "Schema must be a sequence or dictionary", Dictionary({ { "schema", 42 } }) } },
		{ "rule_not_dictionary", { "Rule must be a dictionary", Dictionary({ { "schema",
			Sequence({ 42 }) } }) } },
		{ "rule_missing_target", { "Invalid/missing field <target> in the rule", Dictionary({ { "schema",
			Dictionary({ { "value", "" } }) } }) } },
		{ "rule_invalid_target", { "Invalid/missing field <target> in the rule", Dictionary({ { "schema",
			Dictionary({ { "target", 42 }, { "value", "" } }) } }) } },
		{ "rule_invalid_mode", { "Mode must be a string or integer", Dictionary({ { "schema",
			Dictionary({ { "target", "x" }, { "value", "y" }, { "mode", nullptr} }) } }) } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig), sMsg);
		}
	}
}

//
//
//
TEST_CASE("StringMatcher.match_string")
{
	auto fnScenario = [&](Variant vConfig, std::string_view sValue, bool fExpected)
	{
		auto pObj = createObject(CLSID_StringMatcher, vConfig);
		auto pMatcher = queryInterface<IStringMatcher>(pObj);
		bool fResult = pMatcher->match(sValue, false);
		REQUIRE(fResult == fExpected);
	};

	using namespace openEdr::variant::operation;
	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData{
		// sName: vConfig, sValue, fExpected
		{ "plain_begin_true", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "mode", MatchMode::Begin } })
			}) }, }), "xxxyyy", true } },
		{ "plain_begin_false", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "42" }, { "mode", MatchMode::Begin } })
			}) }, }), "yyyxxx", false } },
		{ "plain_end_true", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "mode", MatchMode::End } })
			}) }, }), "yyyxxx", true } },
		{ "plain_begin_false", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "42" }, { "mode", MatchMode::End } })
			}) }, }), "xxxyyy", false } },
		{ "plain_all_true_1", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "mode", MatchMode::All } })
			}) }, }), "xxxyyy", true } },
		{ "plain_all_true_2", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "mode", MatchMode::All } })
			}) }, }), "zzzxxxyyy", true } },
		{ "plain_all_true_3", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "mode", MatchMode::All } })
			}) }, }), "yyyxxx", true } },
		{ "plain_all_false", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "42" }, { "mode", MatchMode::All } })
			}) }, }), "xxxyyy", false } },
		{ "skip_larger_target", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" } }),
				Dictionary({ { "target", "xxx" } })
			}) }, }), "zzzxxxyyy", true } },
		{ "wstring_string", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", L"xxx" } })
			}) }, }), "zzzxxxyyy", true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sValue, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sValue, fExpected));
		}
	}
}

//
//
//
TEST_CASE("StringMatcher.match_wstring")
{
	auto fnScenario = [&](Variant vConfig, std::wstring_view sValue, bool fExpected)
	{
		auto pObj = createObject(CLSID_StringMatcher, vConfig);
		auto pMatcher = queryInterface<IStringMatcher>(pObj);
		bool fResult = pMatcher->match(sValue, false);
		REQUIRE(fResult == fExpected);
	};

	using namespace openEdr::variant::operation;
	const std::map<const std::string, std::tuple<Variant, std::wstring, bool>> mapData{
		// sName: vConfig, sValue, fExpected
		// Do only simple check here because others are already
		// covered by StringMatcher.match_string
		{ "string_wstring", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" } })
			}) }, }), L"zzzxxxyyy", true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sValue, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sValue, fExpected));
		}
	}
}

//
//
//
TEST_CASE("StringMatcher.replace_string")
{
	auto fnScenario = [&](Variant vConfig, std::string_view sValue, std::string_view sExpected)
	{
		auto pObj = createObject(CLSID_StringMatcher, vConfig);
		auto pMatcher = queryInterface<IStringMatcher>(pObj);
		auto sResult = pMatcher->replace(sValue);
		REQUIRE(sResult == sExpected);
	};

	using namespace openEdr::variant::operation;
	const std::map<const std::string, std::tuple<Variant, std::string, std::string>> mapData{
		// sName: vConfig, sValue, sExpected
		{ "plain_begin", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::Begin } })
			}) }, }), "xxxyyy", "42yyy" } },
		{ "plain_begin_greed", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::Begin } }),
				Dictionary({ { "target", "xxx42" }, { "value", "88" }, { "mode", MatchMode::Begin } })
			}) }, }), "xxx42yyy", "88yyy" } },
		{ "plain_begin_false", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "42" }, { "value", "" }, { "mode", MatchMode::Begin } })
			}) }, }), "yyyxxx", "yyyxxx" } },
		{ "plain_end", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::End } })
			}) }, }), "yyyxxx", "yyy42" } },
		{ "plain_end_greed", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::End } }),
				Dictionary({ { "target", "xxx42" }, { "value", "88" }, { "mode", MatchMode::End } })
			}) }, }), "yyyxxx42", "yyy88" } },
		{ "plain_end_false", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "42" }, { "mode", MatchMode::End } })
			}) }, }), "xxxyyy", "xxxyyy" } },
		{ "plain_all_begin", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::All } })
			}) }, }), "xxxyyy", "42yyy" } },
		{ "plain_all_middle", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::All } })
			}) }, }), "zzzxxxyyy", "zzz42yyy" } },
		{ "plain_all_end", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "xxx" }, { "value", "42" }, { "mode", MatchMode::All } })
			}) }, }), "yyyxxx", "yyy42" } },
		{ "plain_all_false", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "42" }, { "value", "42" }, { "mode", MatchMode::All } })
			}) }, }), "xxxyyy", "xxxyyy" } },
		{ "skip_larger_target", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" }, { "value", "123" } }),
				Dictionary({ { "target", "xxx" }, { "value", "42" } })
			}) }, }), "zzzxxxyyy", "zzz42yyy" } },
		{ "wstring_string", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", L"xxx" }, { "value", L"42" } })
			}) }, }), "zzzxxxyyy", "zzz42yyy" } },
	};

	for (const auto&[sName, params] : mapData)
	{
		const auto&[vConfig, sValue, sExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sValue, sExpected));
		}
	}
}

//
//
//
TEST_CASE("StringMatcher.replace_wstring")
{
	auto fnScenario = [&](Variant vConfig, std::wstring_view sValue, std::wstring_view sExpected)
	{
		auto pObj = createObject(CLSID_StringMatcher, vConfig);
		auto pMatcher = queryInterface<IStringMatcher>(pObj);
		auto sResult = pMatcher->replace(sValue);
		REQUIRE(sResult == sExpected);
	};

	using namespace openEdr::variant::operation;
	const std::map<const std::string, std::tuple<Variant, std::wstring, std::wstring>> mapData{
		// sName: vConfig, sValue, sExpected
		// Do only simple check here because others are already
		// covered by StringMatcher.replace_string
		{ "plain_begin", { Dictionary({ { "schema", Sequence({
				Dictionary({ { "target", L"xxx" }, { "value", L"42" }, { "mode", MatchMode::Begin } })
			}) }, }), L"xxxyyy", L"42yyy" } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sValue, sExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sValue, sExpected));
		}
	}
}