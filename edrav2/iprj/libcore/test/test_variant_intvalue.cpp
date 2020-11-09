//
// EDRAv2.ut_libcore project
//
// Author: Yury Ermakov (14.02.2019)
// Reviewer:
//

#include "pch.h"

using namespace openEdr;

//
//
//
TEST_CASE("Variant.IntValue_operator_greater")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = ((Variant::IntValue)vValue1 > (Variant::IntValue)vValue2);
		REQUIRE(fResult == fEthalon);
	};

	std::vector<std::tuple<const std::string, Variant, Variant, bool>> g_vecData = {
		// name, vValue1, vValue2, fEthalon
		{ "signed_signed_1", -42, -2, false },
		{ "signed_signed_2", -2, -42, true },
		{ "signed_zero", -42, 0, false },
		{ "zero_signed", 0, -42, true },
		{ "unsigned_unsigned_1", 42, 2, true },
		{ "unsigned_unsigned_2", 2, 42, false },
		{ "unsigned_zero", 42, 0, true },
		{ "zero_unsigned", 0, 42, false },
		{ "signed_unsigned", -42, 42, false },
		{ "unsigned_signed", 42, -42, true },
	};

	for (auto [ name, vValue1, vValue2, fEthalon ] : g_vecData)
	{
		DYNAMIC_SECTION(name)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.IntValue_operator_less")
{
	auto fnScenario = [&](Variant vValue1, Variant vValue2, bool fEthalon)
	{
		bool fResult = ((Variant::IntValue)vValue1 < (Variant::IntValue)vValue2);
		REQUIRE(fResult == fEthalon);
	};

	std::vector<std::tuple<const std::string, Variant, Variant, bool>> g_vecData = {
		// name, vValue1, vValue2, fEthalon
		{ "signed_signed_1", -42, -2, true },
		{ "signed_signed_2", -2, -42, false },
		{ "signed_zero", -42, 0, true },
		{ "zero_signed", 0, -42, false },
		{ "unsigned_unsigned_1", 42, 2, false },
		{ "unsigned_unsigned_2", 2, 42, true },
		{ "unsigned_zero", 42, 0, false },
		{ "zero_unsigned", 0, 42, true },
		{ "signed_unsigned", -42, 42, true },
		{ "unsigned_signed", 42, -42, false },
	};

	for (auto [ name, vValue1, vValue2, fEthalon ] : g_vecData)
	{
		DYNAMIC_SECTION(name)
		{
			REQUIRE_NOTHROW(fnScenario(vValue1, vValue2, fEthalon));
		}
	}
}
