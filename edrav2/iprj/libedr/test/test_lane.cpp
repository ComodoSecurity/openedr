//
// edrav2.ut_libedr project
//
// Author: Yury Ermakov (16.04.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"
#include "common.h"

using namespace cmd;

//
//
//
TEST_CASE("lanes.getCurLane_setCurLane")
{
	using namespace cmd::edr;

	SECTION("simple")
	{
		setCurLane(LaneType::Fast);
		REQUIRE(getCurLane() == LaneType::Fast);
		setCurLane(LaneType::SlowLocalFS);
		REQUIRE(getCurLane() == LaneType::SlowLocalFS);
	}

	SECTION("several_threads")
	{
		bool fOtherThreadResult = false;

		auto fnOtherThread = [&fOtherThreadResult]() -> void
		{
			setCurLane(LaneType::Fast);
			if (getCurLane() != LaneType::Fast)
				return;
			setCurLane(LaneType::SlowLocalFS);
			if (getCurLane() != LaneType::SlowLocalFS)
				return;
			fOtherThreadResult = true;
		};


		setCurLane(LaneType::SlowNetwork);
		REQUIRE(getCurLane() == LaneType::SlowNetwork);
		std::thread testThread(fnOtherThread);
		testThread.join();
		REQUIRE(fOtherThreadResult);
		REQUIRE(getCurLane() == LaneType::SlowNetwork);
	}

	SECTION("in_new_thread")
	{
		bool fOtherThreadResult = false;

		auto fnOtherThread = [&fOtherThreadResult]() -> void
		{
			fOtherThreadResult = getCurLane() == LaneType::Undefined;
		};

		setCurLane(LaneType::SlowNetwork);
		REQUIRE(getCurLane() == LaneType::SlowNetwork);
		std::thread testThread(fnOtherThread);
		testThread.join();
		REQUIRE(fOtherThreadResult);
		REQUIRE(getCurLane() == LaneType::SlowNetwork);
	}
}

//
//
//
TEST_CASE("lanes.checkCurLane")
{
	using namespace cmd::edr;

	SECTION("less")
	{
		setCurLane(LaneType::SlowLocalFS);
		REQUIRE_NOTHROW(checkCurLane(LaneType::Fast, "tag", SL));
	}

	SECTION("equal")
	{
		setCurLane(LaneType::SlowLocalFS);
		REQUIRE_NOTHROW(checkCurLane(LaneType::SlowLocalFS, "tag", SL));
	}

	SECTION("more")
	{
		setCurLane(LaneType::Fast);
		REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag", SL), error::SlowLaneOperation);
	}

	SECTION("testExceptionParameters")
	{
		setCurLane(LaneType::Fast);

		Variant vException;

		try
		{
			checkCurLane(LaneType::SlowLocalFS, "tag", SL);
		}
		catch (const error::SlowLaneOperation& e)
		{
			vException = e.serialize();
			REQUIRE(e.getErrorCode() == error::ErrorCode::SlowLaneOperation);
			REQUIRE(e.getRequestedLaneType() == LaneType::SlowLocalFS);
			REQUIRE(e.getThreadLaneType() == LaneType::Fast);
			REQUIRE(e.getTag() == "tag");
		}

		REQUIRE(vException.isDictionaryLike());
		REQUIRE(vException["errCode"] == error::ErrorCode::SlowLaneOperation);
		REQUIRE(vException["tag"] == "tag");
		REQUIRE(vException["curLane"] == LaneType::Fast);
		REQUIRE(vException["curLaneStr"] == "Fast");
		REQUIRE(vException["needLane"] == LaneType::SlowLocalFS);
		REQUIRE(vException["needLaneStr"] == "SlowLocalFS");
	}
}


//
//
//
TEST_CASE("lanes.convertLaneToString")
{
	using namespace cmd::edr;

	REQUIRE(convertLaneToString(LaneType::Fast) == "Fast");
	REQUIRE(convertLaneToString(LaneType::SlowLocalFS) == "SlowLocalFS");
	REQUIRE(convertLaneToString(LaneType::Undefined) == "");
	REQUIRE(convertLaneToString(LaneType(0x1234567)) == "");
}

//
//
//
TEST_CASE("lanes.convertStringToLane")
{
	using namespace cmd::edr;

	REQUIRE(convertStringToLane("Fast") == LaneType::Fast);
	REQUIRE(convertStringToLane("SlowLocalFS") == LaneType::SlowLocalFS);
	REQUIRE(convertStringToLane("Undefined") == LaneType::Undefined);
	REQUIRE(convertStringToLane("Invalid") == LaneType::Undefined);
}

//
//
//
TEST_CASE("lanes.convertVariantToLane")
{
	using namespace cmd::edr;

	REQUIRE(convertVariantToLane(Variant("Fast")) == LaneType::Fast);
	REQUIRE(convertVariantToLane(Variant(LaneType::Fast)) == LaneType::Fast);
	REQUIRE_THROWS(convertVariantToLane(Variant(0x1234567)));
	REQUIRE_THROWS(convertVariantToLane(Variant("Invalid")));
	REQUIRE_THROWS(convertVariantToLane(Variant("Undefined")));
}

//
//
//
Variant execLaneOperation(std::string_view sOperation, Variant vParams = Variant())
{
	Variant vCmd = Dictionary({
		{"clsid", CLSID_LaneOperation},
		{"operation", sOperation}
		});
	if (!vParams.isNull())
		vCmd.merge(vParams, Variant::MergeMode::All);
	return execCommand(vCmd);
}

//
//
//
TEST_CASE("LaneOperation.get")
{
	using namespace cmd::edr;

	SECTION("asStr_is_absent")
	{
		setCurLane(LaneType::Fast);
		REQUIRE(execLaneOperation("get") == LaneType::Fast);
	}

	SECTION("asStr_is_true")
	{
		setCurLane(LaneType::Fast);
		REQUIRE(execLaneOperation("get", Dictionary({ {"asStr", true} })) == "Fast");
	}

	SECTION("asStr_is_false")
	{
		setCurLane(LaneType::Fast);
		REQUIRE(execLaneOperation("get", Dictionary({ {"asStr", false} })) == LaneType::Fast);
	}
}

//
//
//
TEST_CASE("LaneOperation.set")
{
	using namespace cmd::edr;

	SECTION("simple")
	{
		setCurLane(LaneType::Fast);
		execLaneOperation("set", Dictionary({ {"lane", LaneType::SlowLocalFS} }));
		REQUIRE(getCurLane() == LaneType::SlowLocalFS);
	}

	SECTION("lane_is_str")
	{
		setCurLane(LaneType::Fast);
		execLaneOperation("set", Dictionary({ {"lane", "SlowLocalFS"} }));
		REQUIRE(getCurLane() == LaneType::SlowLocalFS);
	}

	SECTION("lane_is_invalid")
	{
		setCurLane(LaneType::Fast);
		REQUIRE_THROWS(execLaneOperation("set", Dictionary({ {"lane", "Invalid"} })));
		REQUIRE(getCurLane() == LaneType::Fast);
	}

	SECTION("lane_is_absent")
	{
		setCurLane(LaneType::Fast);
		REQUIRE_THROWS(execLaneOperation("set"));
		REQUIRE(getCurLane() == LaneType::Fast);
	}
}

//
//
//
TEST_CASE("LaneOperation.check")
{
	using namespace cmd::edr;

	SECTION("less")
	{
		setCurLane(LaneType::SlowLocalFS);
		REQUIRE_NOTHROW(execLaneOperation("check", Dictionary({ {"lane", LaneType::Fast}, {"tag", "xxx"} })));
	}

	SECTION("more")
	{
		setCurLane(LaneType::Fast);

		try
		{
			execLaneOperation("check", Dictionary({ {"lane", LaneType::SlowLocalFS}, {"tag", "xxx"} }));
		}
		catch (const error::SlowLaneOperation & e)
		{
			REQUIRE(e.getErrorCode() == error::ErrorCode::SlowLaneOperation);
			REQUIRE(e.getRequestedLaneType() == LaneType::SlowLocalFS);
			REQUIRE(e.getTag() == "xxx");
			return;
		}

		FAIL("Exception is occured");
	}
}

//
//
//
TEST_CASE("LaneOperation.enableStatistic")
{
	using namespace cmd::edr;

	SECTION("enable_false")
	{
		execLaneOperation("enableStatistic", Dictionary({ {"enable", false} }));
		auto vStatInfo = execLaneOperation("getStatistic");
		REQUIRE(vStatInfo.isNull());
	}

	SECTION("enable_true")
	{
		setCurLane(LaneType::Fast);
		execLaneOperation("enableStatistic", Dictionary({ {"enable", false} }));
		REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag1", SL), error::SlowLaneOperation);

		execLaneOperation("enableStatistic", Dictionary({ {"enable", true} }));

		REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag1", SL), error::SlowLaneOperation);
		auto vStatInfo = execLaneOperation("getStatistic");
		REQUIRE(vStatInfo.isDictionaryLike());
		REQUIRE(vStatInfo.has("tagsByCount"));
		auto vReport = vStatInfo.get("tagsByCount");
		REQUIRE(vReport == Sequence({ Dictionary({ {"tag", "tag1"}, {"count", 1 } }) }));
	}


	SECTION("enable_absent")
	{
		setCurLane(LaneType::Fast);
		execLaneOperation("enableStatistic", Dictionary({ {"enable", false} }));
		REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag1", SL), error::SlowLaneOperation);

		execLaneOperation("enableStatistic");

		REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag1", SL), error::SlowLaneOperation);
		auto vStatInfo = execLaneOperation("getStatistic");
		REQUIRE(vStatInfo.isDictionaryLike());
		REQUIRE(vStatInfo.has("tagsByCount"));
		auto vReport = vStatInfo.get("tagsByCount");
		REQUIRE(vReport == Sequence({ Dictionary({ {"tag", "tag1"}, {"count", 1 } }) }));
	}
}

//
//
//
TEST_CASE("LaneOperation.getStatistic")
{
	using namespace cmd::edr;

	setCurLane(LaneType::Fast);
	execLaneOperation("enableStatistic", Dictionary({ {"enable", false} }));

	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag1", SL), error::SlowLaneOperation);
	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag2", SL), error::SlowLaneOperation);

	execLaneOperation("enableStatistic", Dictionary({ {"enable", true} }));

	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag3", SL), error::SlowLaneOperation);
	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag1", SL), error::SlowLaneOperation);
	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag2", SL), error::SlowLaneOperation);
	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag3", SL), error::SlowLaneOperation);
	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag2", SL), error::SlowLaneOperation);
	REQUIRE_THROWS_AS(checkCurLane(LaneType::SlowLocalFS, "tag3", SL), error::SlowLaneOperation);

	auto vStatInfo = execLaneOperation("getStatistic");
	REQUIRE(vStatInfo.isDictionaryLike());
	REQUIRE(vStatInfo.has("tagsByCount"));
	auto vReport = vStatInfo.get("tagsByCount");
	REQUIRE(vReport == Sequence({ 
		Dictionary({ {"tag", "tag3"}, {"count", 3 } }),
		Dictionary({ {"tag", "tag2"}, {"count", 2 } }),
		Dictionary({ {"tag", "tag1"}, {"count", 1 } }),
	}));


}

