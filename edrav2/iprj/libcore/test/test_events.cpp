//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (06.11.2019)
// Reviewer:
//

#include "pch.h"

using namespace cmd;

//
//
//
TEST_CASE("Event.getEventTypeString")
{
	auto fnScenario = [](Event eventType, std::string_view sExpected)
	{
		auto sResult = getEventTypeString(eventType);
		REQUIRE(sResult == sExpected);
	};

	const std::map<const std::string, std::tuple<Event, std::string>> mapData{
		// sName: eventType, sExpected
		{ "LLE_NONE", { Event::LLE_NONE, "LLE_NONE" } }
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [eventType, sExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(eventType, sExpected));
		}
	}

	const std::map<std::string, std::tuple<std::string, Event>> mapBad{
		// sName: sMsg, eventType
		{ "under_1m", { "Event type <424242> has no text string", (Event)424242 } },
		{ "above_1m", { "Event type <1000001> has no text string", (Event)1000001 } }
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, eventType] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(eventType, ""), sMsg);
		}
	}
}
