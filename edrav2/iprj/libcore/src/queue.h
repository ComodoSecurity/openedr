//
// edrav2.libcore
// 
// Author: Denis Bogdanov (24.02.2019)
// Reviewer: Yury Podpruzhnikov (27.02.2019)
//
///
/// @file Queue object declaration
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

///////////////////////////////////////////////////////////////////////
//
// Queue
//
///////////////////////////////////////////////////////////////////////

///
/// Simple queue class
///
class Queue : public ObjectBase<CLSID_Queue>,
	public IQueue,
	public IDataReceiver,
	public IDataProvider,
	public ICommandProcessor
{

private:
	typedef std::deque<Variant> BaseQueue;
	static inline const Size c_nDefaultMaxSize = 100000;
	static inline const Size c_nWarnMask = 0x1FF;

	Variant m_vTag;

	Variant m_vBatch;
	std::recursive_mutex m_mtxBatch;
	Size m_nBatchSize = 0;
	Size m_nBatchTimeout = 0; // in milliseconds
	ThreadPool::TimerContextPtr m_pBatchTimer;

	BaseQueue m_data;
	std::mutex m_mtxData;
	Time m_nLastLimitExceed = 0;
	const Time c_nLimitExceedTimeout = 5000;
	QueueMode m_eMode = QueueMode::All;
	std::atomic<Size> m_nFilteredCount = 0;
	Size m_nPutTryCount = 0;
	Size m_nGetTryCount = 0;
	Size m_nGetCount = 0;
	Size m_nPutCount = 0;
	Size m_nDropCount = 0;
	Size m_nRollbackCount = 0;
	Size m_nDelayedNotifications = 0;
	Time m_nStartTime = 0;
	Size m_nMaxSize = c_nDefaultMaxSize;
	Size m_nWarnSize = c_nDefaultMaxSize / 2 + c_nDefaultMaxSize / 4;
	Size m_nSaveSize = c_nDefaultMaxSize;
	Size m_nWarnMask = c_nWarnMask;
	bool m_fPersistent = false;
	bool m_fWasOverflown = false;
	bool m_fCircular = false;
	ObjPtr<IQueueNotificationAcceptor> m_pNotifyAcceptor;

	ObjPtr<IQueueFilter> m_pFilter;

	void putIntoData(const Variant& vData, bool fDelaySending);
	bool tryToPut(const Variant& vData);
	bool flushBatch();
	void notifyDelayedData();

public:

	///
	/// Object final construction
	///
	void finalConstruct(Variant vConfig);

	// IQueue

	///
	/// @copydoc IQueue::getInfo()
	///
	virtual Variant getInfo() override;

	///
	/// @copydoc IQueue::getDataStatistic()
	///
	virtual Variant getDataStatistic(std::string sIndex) override;

	///
	/// @copydoc IQueue::dump()
	///
	virtual void dump(ObjPtr<io::IRawWritableStream> pStream, bool fPrint) override;

	///
	/// @copydoc IQueue::getSize()
	///
	virtual Size getSize() override;

	///
	/// @copydoc IQueue::setLimits()
	///
	virtual Size setLimits(Size nWarnSize, Size nMaxSize) override;

	///
	/// @copydoc IQueue::setNotificationAcceptor()
	///
	virtual void setNotificationAcceptor(ObjPtr<IQueueNotificationAcceptor> pAcceptor) override;

	///
	/// @copydoc IQueue::getNotificationAcceptor()
	///
	virtual ObjPtr<IQueueNotificationAcceptor> getNotificationAcceptor() override;

	///
	/// @copydoc IQueue::setMode()
	///
	virtual QueueMode setMode(QueueMode eMode) override;

	///
	/// @copydoc IQueue::putToFront()
	///
	virtual void rollback(const Variant& vData) override;

	///
	/// @copydoc IQueue::loadData()
	///
	virtual void loadData(Variant vItems) override;

	///
	/// @copydoc IQueue::saveData()
	///
	virtual Variant saveData() override;

	// IDataProvider

	///
	/// @copydoc IDataProvider::get()
	///
	virtual std::optional<Variant> get() override;


	// IDataReceiver

	///
	/// @copydoc IDataReceiver::put()
	///
	virtual void put(const Variant& vData) override;

	// ICommandProcessor

	///
	/// @copydoc ICommandProcessor::execute()
	///
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace openEdr
/// @} 
