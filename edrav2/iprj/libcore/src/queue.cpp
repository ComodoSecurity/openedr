//
// edrav2.libcore
// 
// Author: Denis Bogdanov (24.02.2019)
// Reviewer: Yury Podpruzhnikov (27.02.2019)
//
///
/// @file Queue object implementation
///
/// @addtogroup queue
/// @{
#include "pch.h"
#include "queue.h"
#include "message.hpp"
#include "utilities.hpp"
#include "io.hpp"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "queue"

namespace openEdr {

///
/// Initialize object with following parameters:
///   * notifyAcceptor [obj, opt] - object for receiving queues' notifications.
///   * filter [obj, opt] - object with IQueueFilter interface for filtration before add.
///   * maxSize [int, opt] - maximal queue size.
///   * warnSize [int, opt] - the queue size then warning notifictaion will be send.
///   * warnMask [int, opt] - if the queue size is lager than warnSize then notification
///     is sent then the result (size & mask) != 0.
///   * persistent [bool, opt] - shows that the queue is persistent and saves own state
///     between runs.
///   * tag [var, opt] - the value which is sent to norification function as a context.
///   * mode [int, opt] - default initial mode (put | get | all).
///   * items [seq, opt] - saved items for persistent queue.
///   * batchSize [int, opt] - a branch maximum size (default: 0).
///   * batchTimeout [int, opt] - a timeout (in milliseconds) to finish the current branch 
///		(default: 0 - not used).
///
void Queue::finalConstruct(Variant vConfig)
{
 	if (vConfig.isNull()) return;

	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "finalConstruct() supports only dictionary as a parameter").throwException();
	
	m_nMaxSize = vConfig.get("maxSize", m_nMaxSize);
	m_nWarnSize = vConfig.get("warnSize", m_nWarnSize);
	m_nSaveSize = vConfig.get("saveSize", m_nSaveSize);
	m_nWarnMask = vConfig.get("warnMask", m_nWarnMask);
	m_fPersistent = vConfig.get("persistent", m_fPersistent);
	m_fCircular = vConfig.get("circular", m_fCircular);
	m_nBatchSize = variant::convert<variant::ValueType::Integer>(vConfig.get("batchSize", m_nBatchSize));
	m_nBatchTimeout = vConfig.get("batchTimeout", m_nBatchTimeout);
	m_nStartTime = getCurrentTime();
	m_vBatch = Sequence();
	m_vTag = vConfig.get("tag", 0);
	
	if (vConfig.has("notifyAcceptor"))
		setNotificationAcceptor(queryInterface<IQueueNotificationAcceptor>(vConfig["notifyAcceptor"]));

	auto vFilter = vConfig.getSafe("filter");
	if (vFilter.has_value())
		m_pFilter = queryInterface<IQueueFilter>(vFilter.value());

	m_eMode = vConfig.get("mode", m_eMode);

	// TODO: Add supporting notifyAcceptor as a ICommandProvider using wrapper object
}

//
//
//
void Queue::loadData(Variant vData)
{
	if (!m_fPersistent)
		return;

	{
		std::scoped_lock sync(m_mtxData);
		for (auto& i : vData.get("items", Sequence()))
		{
			if (!tryToPut(i))
			{
				LOGWRN("Some data wasn't loaded to the queue");
				break;
			}
			++m_nDelayedNotifications;
		}
	}
	notifyDelayedData();
}

//
//
//
Variant Queue::saveData()
{
	if (!m_fPersistent)
		return {};

	flushBatch();

	Sequence items;
	{
		std::scoped_lock sync(m_mtxData);
		auto nCurrSize = m_nSaveSize;
		for (auto v : m_data)
		{
			if (nCurrSize-- == 0) break;
			items.push_back(v);
		}
	}

	Dictionary vRoot;
	if (!items.isEmpty())
		vRoot.put("items", items);

	return vRoot;
}

//
//
//
QueueMode Queue::setMode(QueueMode eMode)
{
	bool fNeedNotification = false;
	{
		std::scoped_lock sync(m_mtxData);
		std::swap(m_eMode, eMode);
		fNeedNotification = testFlag(m_eMode, QueueMode::Get);
	}

	if (fNeedNotification)
		notifyDelayedData();
	
	return eMode;
}

//
//
//
void Queue::notifyDelayedData()
{
	ObjPtr<IQueueNotificationAcceptor> pCurrNotifier;
	Size nNotificationsCount = 0;
	Size nCurrSize = 0;

	{
		std::scoped_lock sync(m_mtxData);
		if (m_pNotifyAcceptor == nullptr || m_nDelayedNotifications == 0) return;
		if (!testFlag(m_eMode, QueueMode::Get)) return;

		pCurrNotifier = m_pNotifyAcceptor;
		nNotificationsCount = m_nDelayedNotifications;
		m_nDelayedNotifications = 0;
		nCurrSize = m_data.size();
	}

	if (nCurrSize >= m_nWarnSize)
		pCurrNotifier->notifyQueueOverflowWarning(m_vTag);
	for (Size n = nNotificationsCount; n > 0; --n)
		pCurrNotifier->notifyAddQueueData(m_vTag);
}

//
//
//
std::optional<Variant> Queue::get()
{
	std::scoped_lock sync(m_mtxData);
	++m_nGetTryCount;
	if (m_data.empty()) return std::nullopt;
	if (!testFlag(m_eMode, QueueMode::Get))
	{
		++m_nDelayedNotifications;
		return std::nullopt;
	}
	Variant item = m_data.front();
	m_data.pop_front();
	++m_nGetCount;
	return item;
}

//
//
//
void Queue::rollback(const Variant& vData)
{
	ObjPtr<IQueueNotificationAcceptor> pCurrNotifier;

	{
		std::unique_lock sync(m_mtxData);
		if (!testFlag(m_eMode, QueueMode::Put))
			error::OperationDeclined().throwException();
		pCurrNotifier = m_pNotifyAcceptor;
		m_data.push_front(vData);
		++m_nRollbackCount;
	}

	if (pCurrNotifier)
		pCurrNotifier->notifyAddQueueData(m_vTag);
}

//
// This method should be called under the mutex
//
bool Queue::tryToPut(const Variant& vData)
{
	bool fNoOverflow = true;
	++m_nPutTryCount;
	auto nSize = m_data.size();
	if (nSize >= m_nMaxSize)
	{
		if (!m_fCircular) return false;

		// Process circular mode - remove the most old event
		fNoOverflow = false;
		m_data.pop_front();
		++m_nDropCount;
	}

	m_data.push_back(vData);
	++m_nPutCount;
	return fNoOverflow;
}

//
//
//
void Queue::put(const Variant& vData)
{
	// Apply filter
	if (m_pFilter && m_pFilter->filterElem(vData))
	{
		++m_nFilteredCount;
		return;
	}

	// Simple mode processing
	if (m_nBatchSize == 0)
	{
		putIntoData(vData, true);
		return;
	}

	// Batch mode processing
	{
		std::unique_lock sync(m_mtxData);
		if (!testFlag(m_eMode, QueueMode::Put))
			error::OperationDeclined().throwException();
	}

	std::scoped_lock lock(m_mtxBatch);

	m_vBatch.push_back(vData);
	auto nSize = m_vBatch.getSize();
	if (nSize >= m_nBatchSize)
	{
		if (m_pBatchTimer)
			m_pBatchTimer = nullptr;
		putIntoData(m_vBatch, true);
		m_vBatch = Sequence();
	}
	else if (m_nBatchTimeout != 0)
	{
		if (m_pBatchTimer == nullptr)
		{
			// avoid circular pointing timer <-> this
			ObjWeakPtr<Queue> weakPtrToThis(getPtrFromThis(this));
			m_pBatchTimer = runWithDelay(m_nBatchTimeout, [weakPtrToThis]()
			{
				auto pThis = weakPtrToThis.lock();
				if (pThis == nullptr)
					return false;
				return pThis->flushBatch();
			});
		}
	}
}

//
//
//
void Queue::putIntoData(const Variant& vData, bool fCheckMode)
{
	bool fSendWarning = false;
	bool fSendAddNotification = true;
	bool fSendOverflowMessage = false; // exception should be sent outside mutex
	bool fThrowLimitExceeded = false;
	bool fLimitExceeded = false;
	ObjPtr<IQueueNotificationAcceptor> pCurrNotifier;

	{
		std::unique_lock sync(m_mtxData);

		if (fCheckMode && !testFlag(m_eMode, QueueMode::Put))
			error::OperationDeclined().throwException();

		if (!tryToPut(vData))
		{
			if (!m_fWasOverflown)
				fSendOverflowMessage = m_fWasOverflown = true;
			
			if (!m_fCircular)
			{
				fLimitExceeded = true;
				auto nCurrTime = getCurrentTime();
				if (nCurrTime - m_nLastLimitExceed > c_nLimitExceedTimeout)
					fThrowLimitExceeded = true;
				m_nLastLimitExceed = nCurrTime;
			}
			else 
				fSendAddNotification = false;
		}

		auto nSize = m_data.size();
		if (m_fWasOverflown && nSize < (m_nMaxSize / 3) * 2)
			m_fWasOverflown = false;
		fSendWarning = nSize >= m_nWarnSize && (nSize & m_nWarnMask) == 0;
		pCurrNotifier = m_pNotifyAcceptor;
	}

	// Overflow message is temporary disabled
	//if (fSendOverflowMessage)
	//	sendMessage("QueueOverflow", Dictionary({ {"tag", m_vTag} }));

	if (fThrowLimitExceeded)
		error::LimitExceeded(SL, FMT("Queue limit is exceeded <" << m_vTag << ">, some data is possibly lost")).throwException();

	if (fLimitExceeded)
		return;

	if (!testFlag(m_eMode, QueueMode::Get))
	{
		++m_nDelayedNotifications;
		return;
	}

	if (pCurrNotifier && fSendAddNotification)
		pCurrNotifier->notifyAddQueueData(m_vTag);

	if (pCurrNotifier && fSendWarning)
		pCurrNotifier->notifyQueueOverflowWarning(m_vTag);
}

//
//
//
bool Queue::flushBatch()
{
	std::scoped_lock lock(m_mtxBatch);

	if (m_nBatchSize == 0) 
		return false;

	if (m_pBatchTimer)
		m_pBatchTimer = nullptr;

	if (m_vBatch.isEmpty())
		return false;

	putIntoData(m_vBatch, false);
	m_vBatch = Sequence();
	return false;
}

//
//
//
void Queue::dump(ObjPtr<io::IRawWritableStream> pStream, bool fPrint)
{
	std::scoped_lock sync(m_mtxData);
	for (auto& v : m_data)
	{
		auto sData = fPrint ? v.print() : variant::serializeToJson(v);
		io::write(pStream, sData);
		io::write(pStream, "\n\n");
	}
}

//
//
//
Variant Queue::getDataStatistic(std::string sIndex)
{
	std::map<std::string, Size> stat;
	Size nSkipped = 0;
	{
		std::scoped_lock sync(m_mtxData);
		for (auto& v : m_data)
		{
			auto optData = variant::getByPathSafe(v, sIndex);
			if (optData.has_value())
			{
				try 
				{
					stat[variant::convert<variant::ValueType::String>(optData.value())]++;
				}
				catch (error::TypeError&)
				{
					nSkipped++;
				}
			}
			else
				nSkipped++;
		}
	}

	Dictionary vItems;
	for (auto&[name, val] : stat)
		vItems.put(name, val);

	Dictionary vData;
	vData.put("items", vItems);
	vData.put("skipped", nSkipped);

	return vData;
}

//
//
//
Variant Queue::getInfo()
{
	Time nPeriod = (getCurrentTime() - m_nStartTime) / 1000;
	if (nPeriod == 0) nPeriod = 1;
	std::scoped_lock sync(m_mtxData);
	return Dictionary({
		{"putTryCount", m_nPutTryCount},
		{"filteredCount", (Size)m_nFilteredCount},
		{"putCount", m_nPutCount},
		{"dropCount", m_nDropCount},
		{"rollbackCount", m_nRollbackCount},
		{"getTryCount", m_nGetTryCount},
		{"getCount", m_nGetCount},
		{"saveSize", m_nSaveSize},
		{"warnSize", m_nWarnSize},
		{"warnMask", m_nWarnMask},
		{"mode", m_eMode},
		{"size", m_data.size()},
		{"inRate", m_nPutTryCount / nPeriod},
		{"outRate", m_nGetCount / nPeriod},
		{"maxSize", m_nMaxSize}
	}); 
}

//
//
//
Size Queue::getSize()
{
	std::scoped_lock sync(m_mtxData);
	return m_data.size();
}

//
//
//
Size Queue::setLimits(Size nWarnSize, Size nMaxSize)
{
	std::scoped_lock sync(m_mtxData);
	auto nOldSizeVal = nMaxSize;
	if (nMaxSize != c_nMaxSize)
		m_nMaxSize = nMaxSize;
	if (nWarnSize != c_nMaxSize)
		m_nWarnSize = nWarnSize;
	return nOldSizeVal;
}

//
//
//
ObjPtr<IQueueNotificationAcceptor> Queue::getNotificationAcceptor()
{
	std::scoped_lock sync(m_mtxData);
	return m_pNotifyAcceptor;
}

//
//
//
void Queue::setNotificationAcceptor(ObjPtr<IQueueNotificationAcceptor> pEventsAcceptor)
{
	std::scoped_lock sync(m_mtxData);
	m_pNotifyAcceptor = pEventsAcceptor;
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant Queue::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Trace, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### put()
	/// Puts the data to the queue.
	///   * data [var] - data;
	///
	if (vCommand == "put")
	{
		put(vParams["data"]);
		return {};
	}

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### rollback()
	/// Rolling data back to the head of the queue.
	///   * data [var] - data;
	///
	if (vCommand == "rollback")
	{
		rollback(vParams["data"]);
		return {};
	}


	///
	/// @fn Variant Queue::execute()
	///
	/// ##### getInfo()
	/// Gets the queue information.
	/// 
	else if (vCommand == "getInfo")
	{
		return getInfo();
	}

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### get()
	/// Gets the data from the queue.
	/// 
	else if (vCommand == "get")
	{
		auto v = get();
		return v.has_value() ? v.value() : Variant();
	}

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### dump()
	/// Dumps the queue's data.
	///  * stream - target data stream
	/// 
	else if (vCommand == "dump")
	{
		dump(queryInterface<io::IWritableStream>(vParams["stream"]), vParams.get("print", false));
		return true;
	}

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### getDataStatistic()
	/// Gets the data statistic.
	///  * index - index field name/path
	/// 
	else if (vCommand == "getDataStatistic")
	{
		return getDataStatistic(vParams["index"]);
	}

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### setLimits()
	/// Sets size limits.
	///   * warnSize [var] - warning size;
	///   * maxSize [var] - maximal size;
	/// 
	else if (vCommand == "setLimits")
	{
		return setLimits(vCommand.get("warnSize", c_nMaxSize),
			vCommand.get("maxSize", c_nMaxSize));
	}
 	
	error::OperationNotSupported(SL, 
		FMT("Queue doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace openEdr 
/// @}
