//
// edrav2.libedr project
// 
// Author: Podpruzhnikov Yury (10.09.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
/// @addtogroup edr
/// @{
#pragma once
#include <objects.h>

namespace openEdr {

///
/// Event filter.
///
class PerProcessEventFilter : public ObjectBase<CLSID_PerProcessEventFilter>, 
	public IQueueFilter
{
private:

	// Hash calculator
	typedef crypt::xxh::Hasher Hasher;
	// Hash type
	typedef Hasher::Hash Hash;

	//
	// Hashed field path with default value
	//
	struct HashedField
	{
		std::string sPath;
		std::optional<Variant> vDefault;
	};

	//
	// Info for hash calculation
	//
	struct FilterRule
	{
		Time nTimeout = 0; // Event wait timeout
		std::vector<HashedField> hashedFields; // fields names for hashing
	};

	// Storage of info for hash calculation
	// It is thread safe because it is filled in final construct and read-only later
	std::unordered_map<Event, FilterRule> m_mapEventsRules;

	//
	// Fills m_mapEventsRules
	//
	void parseRules(Variant vCfg);

	//
	// Info for filtration
	//
	struct EventFilterInfo
	{
		Hash nHash;
		Time nTimeout;
	};

	//
	// Creates filterInfo for event
	// @returns std::nullopt if info can't be created
	//
	std::optional<EventFilterInfo> getEventFilterInfo(Variant vEvent);

	//
	// Event filter
	//

	//
	// Event information
	//
	struct EventInfo
	{
		Time nExpireTime = 0;
	};

	//
	// Process information
	//
	struct ProcessEventsStorage
	{
		// Process events
		std::unordered_map<Hash, EventInfo> mapSentEvents;

		// Time for delete the storage.
		// It is set when process has been terminated
		// If it is nullopt, process is run and delete time is not set
		std::optional<std::chrono::steady_clock::time_point> deleteTime;
		
		Variant vProcessInfo;
	};

	ObjPtr<sys::win::IProcessInformation> m_pProcProvider;
	typedef sys::win::ProcId ProcId; // unique Process id
	typedef sys::win::Pid Pid; // unique Process id

	std::unordered_map<ProcId, ProcessEventsStorage> m_mapProcessesInfo; // main storage
	std::mutex m_mtxProcessesInfo; // main storage mutex

	// Purge storage
	static constexpr Time c_nDefaultDelProcTimeout = 10 * 1000 /*10 sec*/;
	Time m_nDelProcTimeout = c_nDefaultDelProcTimeout;

	static constexpr Time c_nDefaultPurgeTimeout = 30 * 1000 /*30 sec*/;
	Time m_nPurgeTimeout = c_nDefaultPurgeTimeout;

	ThreadPool::TimerContextPtr m_purgeTimer;
	bool purge();


protected:
	/// Constructor
	PerProcessEventFilter();
	
	/// Destructor
	virtual ~PerProcessEventFilter() override;

public:

	///
	/// Final construct.
	///
	/// @param vConfig - object's configuration including the following fields:
	///   **purgeTimeout** [int, opt] - purge timeout in ms (default: 30 sec).
	///   **delProcTimeout** [int, opt] - timeout for delete process after its termination in ms (default: 10 sec).
	///   **events** [seq] - sequence of event rules: dictionaries with fields:
	///		**type** [int] - LLE/MLE of event for which rule is applied (supports only one rule for one type).
	///     **timeout** [int] - timeout for event filtration in milliseconds. If 0, timeout is infinite.
	///     **hashedFields** [seq] - list of fields for hash calculation
	///       Element of sequence can be string or dict with fields:
	///         **path** [str] - field path inside event.
	///         **default** [opt] - default value if path is absent.
	///       If sequence element is string, it is analog dictionary with 'path' field only.
	///       If 'path' is absent inside event and default value is not specified, 
	///		  the event is not filtered (passed).
	///
	void finalConstruct(Variant vConfig);

	// IQueueFilter

	/// @copydoc IQueueFilter::filterElem(Variant)
	bool filterElem(Variant vElem) override;
	/// @copydoc IQueueFilter::reset()
	void reset() override;
};

} // namespace openEdr

/// @}
