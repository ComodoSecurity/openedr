//
// edrav2.libcore
// 
// Author: Denis Bogdanov (24.02.2019)
// Reviewer: Yury Podpruzhnikov (27.02.2019)
//
///
/// @file Queue Manager object declaration
///
/// @addtogroup queue Queue
/// @{
#pragma once
#include <objects.h>
#include <command.hpp>
#include <common.hpp>
#include <service.hpp>
#include <queue.hpp>

namespace openEdr {

///
/// Queue Manager service.
///
class QueueManager : public ObjectBase<CLSID_QueueManager>,
	public IService,
	public IQueueManager,
	public ICommandProcessor
{
private:
	typedef std::map<std::string, ObjPtr<IQueue>> QueuesMap;

	QueuesMap m_queues;
	std::mutex m_mtxData;
	std::mutex m_mtxControl;
	bool m_fStopped = true;
	Size m_nWaitOnStop = 500;
	QueueMode m_eQueuesMode = QueueMode::Off;

	ObjPtr<IQueueNotificationAcceptor> m_pNotifyAcceptor;

protected:

	///
	/// Destructor.
	///
	virtual ~QueueManager() override;

	void setQueuesMode(QueueMode eMode);

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **notifyAcceptor** -
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IService

	///
	/// @copydoc IService::loadState()
	///
	virtual void loadState(Variant vState) override;

	///
	/// @copydoc IService::saveState()
	///
	virtual Variant saveState() override;

	///
	/// @copydoc IService::start()
	///
	virtual void start() override;

	///
	/// @copydoc IService::stop()
	///
	virtual void stop() override;

	///
	/// @copydoc IService::shutdown()
	///
	virtual void shutdown() override;


	// IQueueManager

	///
	/// @copydoc IQueueManager::getInfo()
	///
	virtual Variant getInfo() override;

	///
	/// @copydoc IQueueManager::setNotificationAcceptor()
	///
	virtual void setNotificationAcceptor(ObjPtr<IQueueNotificationAcceptor> pAcceptor) override;

	///
	/// @copydoc IQueueManager::getQueue()
	///
	virtual ObjPtr<IQueue> getQueue(std::string_view sName, bool fNoThrow = false) override;

	///
	/// @copydoc IQueueManager::addQueue()
	///
	virtual ObjPtr<IQueue> addQueue(std::string_view sName, Variant vCfg, bool fReplace = false) override;

	///
	/// @copydoc IQueueManager::deleteQueue()
	///
	virtual void deleteQueue(std::string_view sName, bool fNoThrow = false) override;

	// ICommandProcessor

	///
	/// @copydoc ICommandProcessor::execute()
	///
	Variant execute(Variant vCommand, Variant vParams) override;
};

///
/// Queue Manager service.
///
class QueueProxy : public ObjectBase<CLSID_QueueProxy>,
	public IDataReceiver
{
protected:

	std::string m_sQueueName;
	ObjPtr<IDataReceiver> m_pQueueReceiver;
	std::mutex m_mtxData;

	///
	/// Destructor.
	///
	virtual ~QueueProxy() override {}

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **name** - queue name
	///
	void finalConstruct(Variant vConfig)
	{
		m_sQueueName = vConfig["name"];
		// TODO: Add notification support about queues changing
	}

	// IDataReceiver

	///
	/// @copydoc IDataReceiver::put()
	///
	virtual void put(const Variant& vData) override
	{
		{
			std::scoped_lock lock(m_mtxData);
			if (!m_pQueueReceiver)
			{
				auto pQM = queryInterfaceSafe<IQueueManager>(queryService("queueManager"));
				m_pQueueReceiver = queryInterfaceSafe<IDataReceiver>(pQM->getQueue(m_sQueueName));
			}
		}
		m_pQueueReceiver->put(vData);
	}
};

} // namespace openEdr
/// @} 
