//
// edrav2.libedr project
// 
// Author: Podpruzhnikov Yury (10.09.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
/// @addtogroup edr
/// @{
#include "pch.h"
#include "eventfilter.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "eventflt"

namespace openEdr {

//
//
//
PerProcessEventFilter::PerProcessEventFilter()
{
}

//
//
//
PerProcessEventFilter::~PerProcessEventFilter()
{
}

//
//
//
void PerProcessEventFilter::finalConstruct(Variant vConfig)
{
	m_pProcProvider = queryInterface<sys::win::IProcessInformation>(queryService("processDataProvider"));

	parseRules(vConfig);

	m_nDelProcTimeout = vConfig.get("delProcTimeout", m_nDelProcTimeout);
	m_nPurgeTimeout = vConfig.get("purgeTimeout", m_nPurgeTimeout);

	// avoid recursive pointing timer <-> PerProcessEventFilter
	// It is better to start timer on the first filtration request
	ObjWeakPtr<PerProcessEventFilter> weakPtrToThis(getPtrFromThis(this));
	m_purgeTimer = runWithDelay(m_nPurgeTimeout, [weakPtrToThis]() -> bool
		{
			auto pThis = weakPtrToThis.lock();
			if (pThis == nullptr)
				return false;
			return pThis->purge();
		});

}

//
//
//
void PerProcessEventFilter::parseRules(Variant vCfg)
{
	TRACE_BEGIN
	if (!vCfg.isDictionaryLike())
		return;
	auto vEvents = vCfg.getSafe("events");
	if (!vEvents.has_value())
		return;
	if (vEvents->isNull())
		return;
	if (!vEvents->isSequenceLike())
		error::InvalidArgument(SL, FMT("Invalid field type <events> (" << vEvents.value() << ")")).throwException();

	for (auto vEventRule : vEvents.value())
	{
		Event eEvent = vEventRule.get("type");
		FilterRule rule;
		rule.nTimeout = vEventRule.get("timeout");
		Variant seqHashedFields = vEventRule.get("hashedFields");

		rule.hashedFields.reserve(seqHashedFields.getSize());
		for (auto vField : seqHashedFields)
		{
			HashedField field;
			if (vField.isDictionaryLike())
			{
				field.sPath = vField.get("path");
				field.vDefault = vField.getSafe("default");
			}
			else
			{
				field.sPath = vField;
			}

			rule.hashedFields.push_back(std::move(field));
		}

		m_mapEventsRules[eEvent] = std::move(rule);
	}
	TRACE_END(FMT("Fail to parse event rules"))
}

//
//
//
std::optional<PerProcessEventFilter::EventFilterInfo> 
PerProcessEventFilter::getEventFilterInfo(Variant vEvent)
{
	// Check type
	auto vEventType = vEvent.getSafe("baseType");
	if (!vEventType.has_value())
		return std::nullopt;
	Event eEvent = vEventType.value();

	// Find rule
	auto iter = m_mapEventsRules.find(eEvent);
	if (iter == m_mapEventsRules.end())
		return std::nullopt;
	FilterRule* pEventRule = &iter->second;

	// calc event hash
	Hasher hasher;
	crypt::updateHash(hasher, eEvent);
	for (auto& hashedField : pEventRule->hashedFields)
	{
		auto vFieldValue = variant::getByPathSafe(vEvent, hashedField.sPath);
		if (!vFieldValue.has_value())
		{
			if (!hashedField.vDefault.has_value())
				return std::nullopt;
			vFieldValue = hashedField.vDefault.value();
		}
		crypt::updateHash(hasher, vFieldValue.value(), crypt::VariantHash::AsEvent);
	}

	// prepare filterInfo
	EventFilterInfo filterInfo;
	filterInfo.nTimeout = pEventRule->nTimeout;
	filterInfo.nHash = hasher.finalize();
	return filterInfo;
}

//
//
//
bool PerProcessEventFilter::purge()
{
	std::list<ProcessEventsStorage> deletedStorages;

	auto timeNow = std::chrono::steady_clock::now();
	{
		std::scoped_lock lock(m_mtxProcessesInfo);

		for (auto iter = m_mapProcessesInfo.begin(); iter != m_mapProcessesInfo.end();)
		{
			auto& procId = iter->first;
			auto& procStorage = iter->second;
			if (procStorage.deleteTime.has_value())
			{
				if (timeNow > procStorage.deleteTime.value())
				{
					// delete process
					deletedStorages.push_back(std::move(procStorage));
					iter = m_mapProcessesInfo.erase(iter);
					continue;
				}
			}
			else
			{
				bool fProcessIsTerminated = true;
				do
				{
					// First time try to get vProcessInfo
					if (procStorage.vProcessInfo.isNull())
					{
						try
						{
							procStorage.vProcessInfo = m_pProcProvider->getProcessInfoById(procId);
						}
						catch (error::NotFound&)
						{
						}

						// If still null - when process already is terminated
						if (procStorage.vProcessInfo.isNull())
							break;
					}


					// Check termination
					if (procStorage.vProcessInfo.has("deletionTime"))
						break;

					fProcessIsTerminated = false;
				} while (false);

				if (fProcessIsTerminated)
					procStorage.deleteTime = timeNow + std::chrono::milliseconds(m_nDelProcTimeout);
			}

			++iter;
		}
	}

	deletedStorages.clear();
	return true;
}

//
//
//
bool PerProcessEventFilter::filterElem(Variant vEvent)
{
	// Get filterInfo for event
	// Pass events without filterInfo
	auto filterInfo = getEventFilterInfo(vEvent);
	if (!filterInfo.has_value())
		return false;

	// Process only events with process information
	auto vProcess = vEvent.getSafe("process");
	if (!vProcess.has_value())
		return false;

	// Get ProcId
	ProcId procId;
	do
	{
		auto vProcId = vProcess->getSafe("id");
		if (vProcId.has_value())
		{
			procId = vProcId.value();
			break;
		}

		CMD_TRY
		{
			auto vProcessInfo = m_pProcProvider->enrichProcessInfo(vProcess.value());
			if (vProcessInfo.isEmpty())
				return false;

			procId = vProcessInfo["id"];
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& ex)
		{
			ex.log(SL, "Can't get process info for event");
			return false;
		}
	} while (false);

	Time nEventTime = vEvent.get("tickTime");

	{
		std::scoped_lock lock(m_mtxProcessesInfo);
		// Get process events storage
		auto& eventStorage = m_mapProcessesInfo[procId];
		auto& eventInfo = eventStorage.mapSentEvents[filterInfo->nHash];

		// Filter event
		if (nEventTime < eventInfo.nExpireTime)
			return true;

		// Pass event
		eventInfo.nExpireTime = (filterInfo->nTimeout == 0) ? Time(-1) : nEventTime + filterInfo->nTimeout;
	}

	return false;
}

//
//
//
void PerProcessEventFilter::reset()
{
	std::scoped_lock lock(m_mtxProcessesInfo);
	m_mapProcessesInfo.clear();
}

} // namespace openEdr
/// @}
