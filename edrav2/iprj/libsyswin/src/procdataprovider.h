//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (24.02.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Process Data Provider declaration
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once
#include "objects.h"

namespace openEdr {
namespace sys {
namespace win {


///
/// Process Data Provider class.
///
class ProcessDataProvider : public ObjectBase<CLSID_ProcessDataProvider>, 
	public ICommandProcessor, 
	public IProcessInformation,
	public IService
{
private:
	static const Size c_nPurgeMask = 0x00FF;
	static const Size c_nPurgeTimeout = 10 * 60 * 1000; // 10 min

	struct ProcInfo
	{
		Variant data;			// Process descriptor
		Time accessTime = 0;	// Last access time to descriptor
	};

	std::string m_sEndpointId;
	std::atomic_bool m_sShutdown;

	std::shared_mutex m_mtxProc;
	std::unordered_map<ProcId, ProcInfo> m_pProcMap;
	std::map<Pid, ProcId> m_pPidMap;

	std::mutex m_mtxStartStop;
	std::atomic_bool m_fInitialized = false;
	Size m_nProcCount = 0;
	std::thread m_pUpdateProcThread;
	ObjPtr<IUserInformation> m_pUserDP;

	ProcId generateId(const Pid nPid, Time nTime);
	bool hasProcessInfo(ProcId id);
	Variant createProcessInfo(Pid nPid, Dictionary vProcessBase = {});
	void updateProcessList();

	Size m_nPurgeMask = c_nPurgeMask;
	Size m_nPurgeTimeout = c_nPurgeTimeout;
	void purgeProcessInfo(ProcId vProcId);
	void purgeAllProcessInfo();

	// TODO: Deprecated. Remove after drop V1 Agent protocol
	Variant getParentsChain(Variant vProcess, bool fAddSelf);

protected:
	/// Constructor.
	ProcessDataProvider();

	/// Destructor.
	virtual ~ProcessDataProvider() override;

public:

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **purgeMask** - purge process start every (purgeMask + 1) started process (default 0xFF).
	///   **purgeTimeout** - guaranteed time process stay in cache after termination.
	///   **receiver** - object for pushing data.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IProcessInformation

	/// @copydoc IProcessInformation::enrichProcessInfo()
	Variant enrichProcessInfo(Variant vParams) override;
	/// @copydoc IProcessInformation::getProcessInfoById()
	Variant getProcessInfoById(ProcId id) override;
	/// @copydoc IProcessInformation::getProcessInfoByPid()
	Variant getProcessInfoByPid(Pid pid) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IService interface

	/// @copydoc IService::loadState()
	void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	Variant saveState() override;
	/// @copydoc IService::start()
	void start() override;
	/// @copydoc IService::stop()
	void stop() override;
	/// @copydoc IService::shutdown()
	void shutdown() override;
};

} // namespace win
} // namespace sys
} // namespace openEdr 

/// @}
