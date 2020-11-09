//
// edrav2.libedr
// 
// Author: Podpruzhnikov Yury (10.09.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
/// @addtogroup edr
/// @{
#include "pch.h"
#include "lane.h"
#include <lane.hpp>

#undef CMD_COMPONENT
#define CMD_COMPONENT "edr"

namespace openEdr {
namespace edr {

using namespace std::literals;

//
//
//
bool isValidLaneType(LaneType eLaneType)
{
	switch (eLaneType)
	{
	case LaneType::Fast:
	case LaneType::SlowLocalFS:
	case LaneType::SlowNetwork:
		return true;
	}

	return false;
}

//
//
//
std::string_view convertLaneToString(LaneType eLaneType)
{
	switch (eLaneType)
	{
	case LaneType::Fast: return "Fast"sv;
	case LaneType::SlowLocalFS: return "SlowLocalFS"sv;
	case LaneType::SlowNetwork: return "SlowNetwork"sv;
	}
	return ""sv;
}

//
//
//
LaneType convertStringToLane(std::string_view sLaneType)
{
	if (sLaneType == "Fast"sv) return LaneType::Fast;
	if (sLaneType == "SlowLocalFS"sv) return LaneType::SlowLocalFS;
	if (sLaneType == "SlowNetwork"sv) return LaneType::SlowNetwork;

	return LaneType::Undefined;
}

//
//
//
LaneType convertVariantToLane(Variant vLaneType)
{
	LaneType eLaneType = LaneType::Undefined;
	if (vLaneType.isString())
	{
		eLaneType = convertStringToLane(vLaneType);
		if (eLaneType == LaneType::Undefined)
			error::InvalidArgument(SL, FMT("Invalid lane: " << vLaneType)).throwException();
	}
	else if (vLaneType.getType() == Variant::ValueType::Integer)
	{
		eLaneType = vLaneType;
	}
	if (!isValidLaneType(eLaneType))
		error::InvalidArgument(SL, FMT("Invalid lane: " << vLaneType)).throwException();
	return eLaneType;
}


//
// LaneType of current thread
//
thread_local LaneType g_eCurLaneType = LaneType::Undefined;

////////////////////////////////////////////////////////////////
//
// Lane invalid checkCurLane() statistic
//

struct LaneStat
{
	uint32_t nCounter = 0;
};
typedef std::unordered_map<std::string, LaneStat> MapLaneStat;

static std::atomic_bool g_fEnableLaneStat = false;
static std::mutex g_mtxLaneStat;
static MapLaneStat g_mapLaneStat;

//
//
//
static void incLaneStat(const std::string& sTag)
{
	if (!g_fEnableLaneStat)
		return;
	std::scoped_lock lock(g_mtxLaneStat);
	++(g_mapLaneStat[sTag].nCounter);
}

//
//
//
static void enableLaneStat(bool fEnable)
{
	g_fEnableLaneStat = fEnable;

	if (fEnable)
		return;

	std::scoped_lock lock(g_mtxLaneStat);
	g_mapLaneStat.clear();
}

//
//
//
static Variant getLaneStat()
{
	if (!g_fEnableLaneStat)
		return {};

	struct StatItem
	{
		std::string sTag;
		LaneStat stat;
	};

	// Create copy to minimize loking time
	std::vector<StatItem> statInfo;
	{
		std::scoped_lock lock(g_mtxLaneStat);
		statInfo.reserve(g_mapLaneStat.size());
		for (const auto& iter : g_mapLaneStat)
			statInfo.push_back({ iter.first, iter.second });
	}

	// Sort
	std::sort(statInfo.begin(), statInfo.end(), [](const StatItem& a, const StatItem& b) -> bool {
		return a.stat.nCounter > b.stat.nCounter;
	});

	Dictionary vLaneStat;

	// Create top 10 tag
	Sequence vReport;
	Size nLimit = std::min<Size>(10, statInfo.size());
	for (Size i = 0; i < nLimit; ++i)
		vReport.push_back(Dictionary({ {"tag", statInfo[i].sTag}, {"count", statInfo[i].stat.nCounter } }));
	vLaneStat.put("tagsByCount", vReport);

	return vLaneStat;
}


////////////////////////////////////////////////////////////////
//
// External API
//

//
//
//
LaneType getCurLane()
{
	return g_eCurLaneType;
}

//
//
//
void setCurLane(LaneType eLaneType)
{
	g_eCurLaneType = eLaneType;
}

//
//
//
void checkCurLane(LaneType eLaneType, std::string_view sTag, SourceLocation srcLoc)
{
	LaneType eCurLaneType = g_eCurLaneType;

	if (eCurLaneType >= eLaneType)
		return;

	SlowLaneOperation(eCurLaneType, eLaneType, sTag, srcLoc).throwException();
}

//
//
//
void SlowLaneOperation::incStat() const
{
	incLaneStat(m_sTag);
}


///
/// Object final construction.
///
/// @param vConfig - the object configuration includes the following fields:
///   * operation [str] - operation (command) name.
///   * Other operation-specified arguments (see below).
///
void LaneOperation::finalConstruct(Variant vConfig)
{
	std::string sOperation = vConfig.get("operation");

	///
	/// Operation `get`.
	/// Wrapper of setCurLane()
	/// Parameters:
	///   * asStr [bool, opt] - return string representation of LaneType (default: `false`).
	///
	if (sOperation == "get")
	{
		bool fAsStr = vConfig.get("asStr", false);

		m_fnOperation = [fAsStr]() -> Variant
		{
			LaneType eLaneType = getCurLane();
			if (!fAsStr)
				return eLaneType;
			return convertLaneToString(eLaneType);
		};
	}

	///
	/// Operation `set`.
	/// Wrapper of setCurLane()
	/// Parameters:
	///   * lane [int | str] - lane type.
	///
	else if (sOperation == "set")
	{
		LaneType eLaneType = convertVariantToLane(vConfig.get("lane"));

		m_fnOperation = [eLaneType]() -> Variant
		{
			setCurLane(eLaneType);
			return {};
		};
	}

	///
	/// Operation `check`.
	/// Calls checkCurLane().
	/// Parameters:
	///   * lane [int | str] - lane type.
	///   * tag [str] - sTag parameter.
	///
	else if (sOperation == "check")
	{
		LaneType eLaneType = convertVariantToLane(vConfig.get("lane"));
		std::string sTag = vConfig.get("tag");

		m_fnOperation = [eLaneType, sTag]() -> Variant
		{
			checkCurLane(eLaneType, sTag, SL);
			return {};
		};
	}

	///
	/// Operation `getStatistic`.
	/// Returns statistic of SlowLaneOperation throwing.
	///
	else if (sOperation == "getStatistic")
	{
		m_fnOperation = []() -> Variant
		{
			return getLaneStat();
		};
	}

	///
	/// Operation `enableStatistic`.
	/// Enables collection of statistic of SlowLaneOperation throwing.
	/// Parameters:
	///   * enable [bool, opt] - enable/Disable stat (default: `true`).
	///
	else if (sOperation == "enableStatistic")
	{
		bool fEnable = vConfig.get("enable", true);

		m_fnOperation = [fEnable]() -> Variant
		{
			enableLaneStat(fEnable);
			return {};
		};
	}

	///
	/// Operation `convertToInt`.
	/// Wrapper of convertVariantToLane().
	/// Scenario should use string values contstants
	/// Parameters:
	///   * lane [int | str] - lane type.
	///
	else if (sOperation == "convertToInt")
	{
		LaneType eLaneType = convertVariantToLane(vConfig.get("lane"));
		m_fnOperation = [eLaneType]() -> Variant
		{
			return eLaneType;
		};
	}

	else
	{
		error::InvalidArgument(SL, FMT("Operation <" << sOperation << "> is not supported")).throwException();
	}



	//m_pOperation = createOperation(vConfig.get("operation"), vConfig);
}

//
//
//
Variant LaneOperation::execute(Variant vParam)
{
	if (!m_fnOperation)
		error::LogicError(SL).throwException();

	return m_fnOperation();
}

//
//
//
std::string LaneOperation::getDescriptionString()
{
	return "LaneOperation";
}

} // namespace edr
} // namespace openEdr
/// @}

