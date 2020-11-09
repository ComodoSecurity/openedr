//
// edrav2.libcore
// 
// Author: Denis Bogdanov (24.02.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Queue objects declaration
///
/// @weakgroup queue Queue
/// @{
#pragma once
#include "object.hpp"
#include "command.hpp"
#include "common.hpp"

namespace openEdr {

///
/// Queue filter interface.
///
class IQueueFilter : OBJECT_INTERFACE
{

public:

	///
	/// Checks element filtration.
	///
	/// @param vEvent - event descriptor.
	/// @return The function returns `true` if the event is filtered (should be skipped).
	///
	virtual bool filterElem(Variant vElem) = 0;

	///
	/// Resets filter state (cache).
	///
	virtual void reset() = 0;
};

///
/// Interface allows to receive notification from queue object.
///
class IQueueNotificationAcceptor : OBJECT_INTERFACE
{
public:
	
	///
	/// Notifies when the data is added to queue.
	///
	/// @param vTag - tag.
	///
	virtual void notifyAddQueueData(Variant vTag) = 0;
	

	/// 
	/// Notifies when the queue is overflown.
	///
	/// @param vTag - tag.
	///
	virtual void notifyQueueOverflowWarning(Variant vTag) = 0;
};

///
/// Available types of operation.
///
CMD_DEFINE_ENUM_FLAG(QueueMode)
{
	Off = 0x00,
	Put = 0x01,
	Get = 0x02,
	All = 0xFF,
};

///
/// Basic data queue interface.
///
class IQueue : OBJECT_INTERFACE
{
public:
 
	///
	/// Get current state of the queue.
	///
	/// @return The function returns an information data packet with the following fields:
	///   **size** - current queue size.
	///   **maxSize** - maximal available queue size (as it set in the config).
	///   **warnSize** - size limit then overflow warning notification is called.
	///   **count** - total number of added items.
	///
	virtual Variant getInfo() = 0;

	///
	/// Get statistic for data in queue.
	///
	/// @param sIndex - the primary index for gathered data.
	/// @return The function returns an information data packet:
	///
	virtual Variant getDataStatistic(std::string sIndex) = 0;

	///
	/// Dumps queue data into the specified stream.
	///
	/// @param pStream - output stream.
	/// @param fPrint [opt] - use a human-readable output instead of JSON (default: false).
	///
	virtual void dump(ObjPtr<io::IRawWritableStream> pStream, bool fPrint = false) = 0;

	///
	/// Get current queue size.
	///
	/// @return The function returns a current size of the queue.
	///
	virtual Size getSize() = 0;

	///
	/// Set size limits.
	///
	/// Set c_nMaxSize if you don't want to change the parameter.
	///
	/// @param nWarnSize - new limit of warning size.
	/// @param nMaxSize [opt] - new limit of maximal size (default: SIZE_MAX).
	/// @return The function returns an old queue maximal size.
	///
	virtual Size setLimits(Size nWarnSize, Size nMaxSize = c_nMaxSize) = 0;

	///
	/// Set the events acceptor for the queue.
	///
	/// If the acceptor has already set then this acceptod is detached from queue.
	///
	/// @param pEventsAcceptor[in] - pointer to IQueueNotificationAcceptor instance.
	///
	virtual void setNotificationAcceptor(ObjPtr<IQueueNotificationAcceptor> pEventsAcceptor) = 0;

	///
	/// Get the events acceptor for the queue.
	///
	/// @return Pointer to IQueueNotificationAcceptor instance
	///
	virtual ObjPtr<IQueueNotificationAcceptor> getNotificationAcceptor() = 0;

	///
	/// Sets a working mode of queue.
	///
	/// @param eMode - the new mode.
	///
	/// @returns The functios returns an old mode of the queue.
	///
	virtual QueueMode setMode(QueueMode eMode) = 0;

	///
	/// Puts the data to front of the queue.
	///
	/// Ignore queue size limits. Because queue can be already full.
	///
	/// @param vData - the data to be added.
	///
	virtual void rollback(const Variant& vData) = 0;

	///
	/// Loads the data into the queue.
	///
	/// @param vItems - data for loading.
	///
	virtual void loadData(Variant vItems) = 0;

	///
	/// Saves the data of the queue.
	///
	/// @returns The function returns a data packet with all data contained in the queue.
	///
	virtual Variant saveData() = 0;
};

//
//
//
class IQueueManager : OBJECT_INTERFACE
{
public:
	virtual Variant getInfo() = 0;
	virtual void setNotificationAcceptor(ObjPtr<IQueueNotificationAcceptor> pAcceptor) = 0;

	virtual ObjPtr<IQueue> getQueue(std::string_view sName, bool fNoThrow = false) = 0;
	virtual ObjPtr<IQueue> addQueue(std::string_view sName, Variant vCfg, bool fReplace = false) = 0;
	virtual void deleteQueue(std::string_view sName, bool fNoThrow = false) = 0;
};

} // namespace openEdr
/// @}
