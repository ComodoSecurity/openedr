//
// edrav2.libcore
// 
// Author: Denis Bogdanov (24.02.2019)
// Reviewer: Yury Podpruzhnikov (27.02.2019)
//
///
/// @file Queue Manager object implementation
///
/// @addtogroup queue
/// @{
#include "pch.h"
#include "queuemgr.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "queuemgr"

namespace cmd {

//
//
//
QueueManager::~QueueManager()
{
	shutdown();
}

///
/// Initialize object with following parameters:
///   * notifyAcceptor [obj, opt] - Object for receiving queues' notificeations. It is used
///     if an queue doesn't have own notifyAcceptor;
///   * waitOnStopTime [int, opt] - Timeout in milliseconds which is used then the manager
///     stops operations;
///   * queues [seq, opt] - The sequence of queues' settings;
///   * autoStart [bool, opt] - If true then the all queues are ready to process events
///     right after creation (without start() call);
///
void QueueManager::finalConstruct(Variant vConfig)
{
	if (vConfig.isNull()) return;
	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "finalConstruct() supports only dictionary as a parameter")
			.throwException();

	if (vConfig.has("notifyAcceptor"))
		setNotificationAcceptor(queryInterface<IQueueNotificationAcceptor>
		(vConfig["notifyAcceptor"]));

	m_nWaitOnStop = vConfig.get("waitOnStopTime", m_nWaitOnStop);

	Dictionary queues = vConfig.get("queues", Dictionary());
	for (auto qDesc : queues)
	{
		auto q = addQueue(qDesc.first, qDesc.second);
		if (q->getNotificationAcceptor() == nullptr)
			q->setNotificationAcceptor(m_pNotifyAcceptor);
	}

	if (vConfig.get("autoStart", false))
		start();
}

//
//
//
void QueueManager::loadState(Variant vState)
{
	LOGLVL(Detailed, "QueueManager state is being loaded");
	if (vState.isEmpty()) return;
	for (auto qDesc : Dictionary(vState))
	{
		auto currQueue = getQueue(qDesc.first, true);
		if (currQueue == nullptr) continue;
		currQueue->loadData(qDesc.second);
	}
	LOGLVL(Detailed, "QueueManager state is loaded");
}

//
//
//
Variant QueueManager::saveState()
{
	LOGLVL(Detailed, "QueueManager state is being saved");
	std::scoped_lock sync(m_mtxData);
	Dictionary vState;
	for (auto q : m_queues)
	{
		auto vData = q.second->saveData();
		if (vData.isEmpty()) continue;
		vState.put(q.first, vData);
	}
	LOGLVL(Detailed, "QueueManager state is saved");
	return vState;
}

//
//
//
void QueueManager::start()
{
	std::scoped_lock sync(m_mtxControl);
	if (!m_fStopped) return;
	LOGLVL(Detailed, "QueueManager is being started");
	m_fStopped = false;
	setQueuesMode(QueueMode::All);
	LOGLVL(Detailed, "QueueManager is started");
}

//
//
//
void QueueManager::stop()
{
	std::scoped_lock sync(m_mtxControl);
	if (m_fStopped) return;
	LOGLVL(Detailed, "QueueManager is being stopped");
	m_fStopped = true;
	setQueuesMode(QueueMode::Get);
	std::this_thread::sleep_for(std::chrono::milliseconds(m_nWaitOnStop));
	setQueuesMode(QueueMode::Off);
	LOGLVL(Detailed, "QueueManager is stopped");
}

//
//
//
void QueueManager::shutdown()
{
	std::scoped_lock sync(m_mtxControl);
	LOGLVL(Detailed, "QueueManager is being shutdowned");
	setQueuesMode(QueueMode::Off);
	std::scoped_lock sync2(m_mtxData);
	m_queues.clear();
	m_pNotifyAcceptor.reset();
	LOGLVL(Detailed, "QueueManager is shutdowned");
}

//
//
//
void QueueManager::setQueuesMode(QueueMode eMode)
{
	std::scoped_lock sync(m_mtxData);
	m_eQueuesMode = eMode;
	for (auto q : m_queues)
		q.second->setMode(m_eQueuesMode);
}

//
//
//
Variant QueueManager::getInfo()
{
	std::scoped_lock sync(m_mtxData);
	Dictionary queues;
	for (auto q : m_queues)
		queues.put(q.first, q.second->getInfo());

	return Dictionary({
			{"queues", queues},
			{"size", m_queues.size()}
		});
}

//
//
//
void QueueManager::setNotificationAcceptor(ObjPtr<IQueueNotificationAcceptor> pEventsAcceptor)
{
	std::scoped_lock sync(m_mtxData);
	auto pOldAcceptor = m_pNotifyAcceptor;
	m_pNotifyAcceptor = pEventsAcceptor;
	for (auto q : m_queues)
		// Set acceptor only for externally overridden acceptors 
		if (q.second->getNotificationAcceptor() == pOldAcceptor)
			q.second->setNotificationAcceptor(m_pNotifyAcceptor);
}

//
//
//
ObjPtr<IQueue> QueueManager::getQueue(std::string_view svName, bool fNoThrow)
{
	std::string sName(svName);
	std::scoped_lock sync(m_mtxData);
	if (m_queues.find(sName) == m_queues.end())
	{
		if (fNoThrow) return nullptr;
		error::InvalidArgument(SL, FMT("Queue <" << svName << "> is not found")).throwException();
	}
	return m_queues[sName];
}

//
//
//
ObjPtr<IQueue> QueueManager::addQueue(std::string_view svName, Variant vCfg, bool fReplace)
{
	if (!fReplace && getQueue(svName, true) != nullptr)
		error::AlreadyExists(SL, FMT("Queue <" << svName << "> already exists")).throwException();
	LOGLVL(Detailed, "Create queue <" << svName << ">");
	auto queue = queryInterface<IQueue>(createObject(CLSID_Queue, vCfg));
	std::scoped_lock sync(m_mtxData);
	queue->setMode(m_eQueuesMode);
	m_queues[std::string(svName)] = queue;
	return queue;
}

//
//
//
void QueueManager::deleteQueue(std::string_view svName, bool fNoThrow)
{
	// Destruction of Queue is out of sync
	// Because it can be long and lead to deadlock
	LOGLVL(Detailed, "Delete queue <" << svName << ">");
	ObjPtr<IQueue> pQueue;
	{
		std::scoped_lock sync(m_mtxData);
		auto iter = m_queues.find(std::string(svName));
		if (iter == m_queues.end())
		{
			if (fNoThrow) return;
			error::InvalidArgument(SL, FMT("Queue <" << svName << "> is not found")).throwException();
		}
		pQueue = iter->second;
		m_queues.erase(iter);
	}
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant QueueManager::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### getQueue()
	/// Get queue by name. Parameter can be a string or Dictionary with
	/// the following data:
	///   * name [str] - queue name;
	///
	if (vCommand == "getQueue")
	{
		if (vParams.getType() == variant::ValueType::String)
			return getQueue(vParams);
		return getQueue(vParams["name"]);
	}

	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### addQueue()
	/// Add new queue.
	///   * name [str] - queue name;
	///   * recreate [bool,opt] - recreate existing queue (true by default)
	///   * ... - other fieds of queue descriptor (see Queue object)
	///
	else if (vCommand == "addQueue")
	{
		return addQueue(vParams["name"], vParams);
	}

	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### addOrGetQueue()
	/// Add new queue or return existing with specified name.
	///   * name [str] - queue name;
	///   * recreate [bool,opt] - recreate existing queue (true by default)
	///   * ... - other fieds of queue descriptor (see Queue object)
	///
	else if (vCommand == "addOrGetQueue")
	{
		auto pQueue = getQueue(vParams["name"], true);
		if (pQueue) return pQueue;
		return addQueue(vParams["name"], vParams);
	}

	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### deleteQueue()
	/// Get queue by name. Parameter can be a string or Dictionary with
	/// the following data:
	///   * name [str] - queue name;
	///
	if (vCommand == "deleteQueue")
	{
		if (vParams.getType() == variant::ValueType::String)
			deleteQueue(vParams);
		else
			deleteQueue(vParams["name"]);
	}

	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### getInfo()
	/// Gets the queue information.
	/// 
	else if (vCommand == "getInfo")
	{
		return getInfo();
	}
	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	else if (vCommand == "start")
	{
		start();
		return {};
	}
	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### stop()
	/// Stops scenarios processing.
	///
	else if (vCommand == "stop")
	{
		stop();
		return {};
	}
	///
	/// @fn Variant QueueManager::execute()
	///
	/// ##### dump()
	/// Dumps queues status to log.
	///
	else if (vCommand == "dump")
	{
		LOGLVL(Critical, "\n" << getInfo() << "\n");
		return {};
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));

	error::OperationNotSupported(SL,
		FMT("QueueManager doesn't support command <" << vCommand << ">")).throwException();
}

} // namespace cmd 
/// @}
