//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (04.01.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Process Data Provider implementation
///
/// @addtogroup syswin System library (Windows)
/// @{
#include "pch.h"
#include "procdataprovider.h"

using namespace std::chrono_literals;

namespace openEdr {
namespace sys {
namespace win {

#undef CMD_COMPONENT
#define CMD_COMPONENT "procdataprv"

//
//
//
ProcessDataProvider::ProcessDataProvider()
{
}

//
//
//
ProcessDataProvider::~ProcessDataProvider()
{
}

//
//
//
void ProcessDataProvider::finalConstruct(Variant vConfig)
{
	m_nPurgeMask = vConfig.get("purgeMask", c_nPurgeMask);
	m_nPurgeTimeout = vConfig.get("purgeTimeout", c_nPurgeTimeout);
	m_sEndpointId = getCatalogData("app.config.license.endpointId", "");
	m_pUserDP = queryInterfaceSafe<IUserInformation>(queryServiceSafe("userDataProvider"));


	// URL: https://support.microsoft.com/en-us/help/131065/how-to-obtain-a-handle-to-any-process-with-sedebugprivilege
	EnableAllPrivileges();
}

//
// TODO: Move to common part (libsys)?
//
Pid getParentProcessId(const HANDLE hProcess)
{
	PROCESS_BASIC_INFORMATION pbi = {};
	NTSTATUS status = ::NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (status)
		error::win::WinApiError(SL, "Fail to call ProcessBasicInformation()").log();
	return (Pid)(ULONG_PTR)pbi.Reserved3;
}

#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

//
// FIXME: I'm not sure that it is a good idea not to pass system exceptions outside
//
bool querySystemInformation(Enum eType, std::vector<uint8_t>& pBuffer)
{
	// FIXME: Please use macros for error statuses
	NTSTATUS status = 0;
	ULONG nBufferSize = 0x1000;
	do
	{
		pBuffer.resize(nBufferSize);
		status = ::NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS(eType), pBuffer.data(), ULONG(pBuffer.size()), &nBufferSize);
	} while (status == STATUS_INFO_LENGTH_MISMATCH);
	if (status)
	{
		LOGWRN("Fail to query system information <status=" << std::to_string(status) << ">");
		return false;
	}

	return true;
}

//
//
//
Time getProcessCreationTime(const HANDLE hProcess)
{
	FILETIME nCreationTime = {};
	FILETIME nExitTime = {};
	FILETIME nKernelTime = {};
	FILETIME nUserTime = {};
	if (!::GetProcessTimes(hProcess, &nCreationTime, &nExitTime, &nKernelTime, &nUserTime))
	{
		error::win::WinApiError(SL, FMT("Fail to query times for process <" << GetProcessId(hProcess) << ">")).log();
		return getCurrentTime();
	}
	
	// TODO: Do a function getTimeFromFileTime()
	ULARGE_INTEGER nTime;
	nTime.LowPart = nCreationTime.dwLowDateTime;
	nTime.HighPart = nCreationTime.dwHighDateTime;
	// Convert to UNIX time
	return ((Time)nTime.QuadPart - 116444736000000000LL) /
		((Time)10 /*100 nano*/ * (Time)1000 /*micro*/);
}

static const Size c_nMaxNtPathLen = 0x8000;

//
//
//
std::wstring getProcessCmdLine(const HANDLE hProcess)
{
	thread_local static std::wstring sBuffer;
	if (sBuffer.empty())
		sBuffer.resize(c_nMaxNtPathLen);
	
	sBuffer[0] = 0;
	// URL: https://wj32.org/wp/2009/01/24/howto-get-the-command-line-of-processes/
	// madCodeHook function, do not add :: before it
	if (!GetProcessCommandLine(hProcess, sBuffer.data(), DWORD(sBuffer.size())))
	{
		LOGLVL(Debug, "Fail to query command line of process <" << ::GetProcessId(hProcess) << ">");
		return {};
	}

	return sBuffer.c_str();
}

//
//
//
ProcId ProcessDataProvider::generateId(const Pid nPid, Time nTime)
{
	crypt::xxh::Hasher calc;
	crypt::updateHash(calc, nPid);
	crypt::updateHash(calc, nTime);
	crypt::updateHash(calc, m_sEndpointId);
	return calc.finalize();
}

//
//
//
bool ProcessDataProvider::hasProcessInfo(ProcId id)
{
	std::shared_lock _lock(m_mtxProc);
	return (m_pProcMap.find(id) != m_pProcMap.end());
}

//
//
//
Variant ProcessDataProvider::getProcessInfoById(ProcId id)
{
	std::shared_lock _lock(m_mtxProc);
	auto itProc = m_pProcMap.find(id);
	if (itProc == m_pProcMap.end())
	{
		error::NotFound(SL, FMT("Can't find process by id <" << id << ">")).log();
		return {};
	}

	// Update process access
	itProc->second.accessTime = getTickCount();
	return itProc->second.data;
}

//
//
//
Variant ProcessDataProvider::getProcessInfoByPid(Pid pid)
{
	ProcId id = 0;
	{
		std::shared_lock _lock(m_mtxProc);
		auto itPid = m_pPidMap.find(pid);
		if (itPid == m_pPidMap.end())
			return {};
		id = itPid->second;
	}
	return getProcessInfoById(id);
}

//
//
//
Variant ProcessDataProvider::createProcessInfo(Pid nPid, Dictionary vProcessBase)
{
	TRACE_BEGIN

	bool fProcessInfo = true;
	bool fProcessQuery = true;
	bool fProcessRead = true;
	sys::win::ScopedHandle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, nPid));
	if (!hProcess)
	{
		fProcessRead = false;
		hProcess.reset(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nPid));
		if (!hProcess)
			hProcess.reset(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, nPid));
	}
	if (!hProcess)
	{
		LOGLVL(Debug, "Can't open process <" << nPid << ">");
		if (vProcessBase.isEmpty() || !vProcessBase.has("creationTime"))
			return {};
		fProcessInfo = false;
		fProcessQuery = false;
	}
	else
	{
		// TODO: Maybe we need to use NtOpenProcessToken here and below
		ScopedHandle hToken;
		fProcessQuery = ::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
	}

	Dictionary vProcess;
	vProcess.put("pid", nPid);

	Time nCreationTime = vProcessBase.has("creationTime") ?
		Time(vProcessBase["creationTime"]) :
		getProcessCreationTime(hProcess);
	vProcess.put("creationTime", nCreationTime);

	vProcess.put("id", generateId(nPid, nCreationTime));

	if (nPid == g_nSystemPid)
		vProcess.put("isKernel", true);

	Variant vToken;
	if (m_pUserDP && (fProcessQuery || vProcessBase.has("userSid")))
	{
		auto sSid = vProcessBase.has("userSid") ?
			std::string(vProcessBase["userSid"]) :
			getProcessSid(hProcess);
		auto fElevated = vProcessBase.has("isElevated") ?
			bool(vProcessBase["isElevated"]) :
			isProcessElevated(hProcess);
		vToken = m_pUserDP->getTokenInfo(hProcess, sSid, fElevated);
	}

	Variant vSecurity;
	if (vToken.isEmpty())
		vToken = Dictionary();
	else
		vSecurity = vToken.get("tokenObj", {});
	vProcess.put("token", vToken);


	if (fProcessInfo || vProcessBase.has("parent"))
	{
		auto vParent = vProcessBase.has("parent") ?
			Dictionary(vProcessBase["parent"]) :
			Dictionary({ {"pid", getParentProcessId(hProcess)} });
		// We can't resolve parent "just now" because of dead loop
		vProcess.put("parent", vParent);
	}

	if (fProcessInfo || vProcessBase.has("imageFile"))
	{
		std::wstring sRawPath = getByPath(vProcessBase, "imageFile.rawPath", "");
		if (sRawPath.empty() && fProcessInfo)
			sRawPath = getProcessImagePath(hProcess);
		if (sRawPath.empty())
			sRawPath = vProcessBase.get("imageName", "");
		
		if (sRawPath.empty())
			LOGLVL(Debug, "Empty image file path for process <" << nPid << ">");
		else
		{
			Variant vFile = Dictionary({
				{"rawPath", sRawPath},
				{"security", vSecurity},
				{"cmdExecute", true}
			});
			vProcess.put("imageFile", variant::createLambdaProxy([vFile]() -> Variant
				{
					auto pFileInformation = queryInterface<sys::win::IFileInformation>(queryService("fileDataProvider"));
					return pFileInformation->getFileInfo(vFile);
				}, true)
			);
		}
	}

	if (fProcessRead || vProcessBase.has("cmdLine"))
	{
		auto sCmdLine = vProcessBase.has("cmdLine") ?
			std::wstring(vProcessBase.get("cmdLine")) :
			getProcessCmdLine(hProcess);
		vProcess.put("cmdLine", sCmdLine);
		vProcess.put("abstractCmdLine", variant::createLambdaProxy([sCmdLine, vSecurity]() -> Variant
		{
			auto pObj = queryInterface<IPathConverter>(queryService("pathConverter"));
			return pObj->getAbstractPath(string::convertToLow(sCmdLine), vSecurity);
		}, true));
	}

	if (fProcessQuery || vProcessBase.has("elevationType"))
	{
		auto eElevationType = vProcessBase.has("elevationType") ?
			Enum(vProcessBase["elevationType"]) :
			getProcessElevationType(hProcess);
		vToken.put("elevationType", eElevationType);
	}

	if (fProcessQuery || vProcessBase.has("integrityLevel"))
	{
		auto nIntegrityLevel = vProcessBase.has("integrityLevel") ?
			IntegrityLevel(vProcessBase["integrityLevel"]) :
			getProcessIntegrityLevel(hProcess);
		vToken.put("integrityLevel", nIntegrityLevel);
	}

	return vProcess;
	TRACE_END(FMT("Fail to get info for process <" << nPid << ">"))
}

//
//
//
Variant ProcessDataProvider::enrichProcessInfo(Variant vParams)
{
	TRACE_BEGIN
	Variant vProcess;
	if (vParams.has("id") && hasProcessInfo(vParams["id"]))
		vProcess = getProcessInfoById(vParams["id"]);
	else if (vParams.has("pid"))
	{
		// Try to find existing process by PID
		Pid nPid = vParams["pid"];
		vProcess = getProcessInfoByPid(nPid);

		// Create new process descriptor
		if (vProcess.isEmpty() ||
			// New process replace another with the existing PID
			(vParams.has("creationTime") && vProcess["id"] != generateId(nPid, vParams["creationTime"])))
		{
			vProcess = createProcessInfo(nPid, vParams);
			if (vProcess.isEmpty())
				return vProcess; // Process not found

			ProcId nId = vProcess["id"];
			{
				std::unique_lock _lock(m_mtxProc);
				if (m_pProcMap.find(nId) == m_pProcMap.end())
				{
					m_pProcMap[nId] = { vProcess, getCurrentTime() };
					m_pPidMap[nPid] = nId;
				}
			}

			if (vProcess.has("parent"))
			{
				// Delayed parent resolve
				auto vParent = vProcess["parent"];
				auto vParentInfo = enrichProcessInfo(vParent);
				if (!vParentInfo.isEmpty() &&
					Time(vProcess["creationTime"]) > Time(vParentInfo["creationTime"]))
					vProcess.put("parent", vParentInfo);
				else
				{
					vProcess.erase("parent");
					LOGLVL(Debug, "Can't find paren for process <" << nPid << "> :" << vParent.print());
				}
			}

			putCatalogData(std::string("os.processes.") + std::to_string(nPid), vProcess);

			LOGLVL(Debug, "New process: pid <" << nPid << ">, id <" << nId <<
				">, path <" << getByPath(vProcess, "imageFile.rawPath", "") << ">");
			LOGLVL(Trace, "Process info: " << vProcess.print());
		}
	}
	else
		error::InvalidArgument(SL, "Not found required parameter <id> or <pid>").throwException();

	// Update process on termination
	if (!vProcess.isEmpty() && vParams.has("deletionTime"))
	{
		vProcess.put("deletionTime", vParams["deletionTime"]);
		vProcess.put("exitCode", vParams.get("exitCode", 0));

		Pid nPid = vProcess["pid"];
		putCatalogData(std::string("os.processes.") + std::to_string(nPid), {});

		// Purge processes every 0x1000 deletions
		if ((++m_nProcCount & m_nPurgeMask) == 0)
			run(&ProcessDataProvider::purgeAllProcessInfo, getPtrFromThis(this));
	}

	return vProcess;
	TRACE_END("Fail to get info for process")
}

//
//
//
void ProcessDataProvider::updateProcessList()
{
	CMD_TRY
	{
		LOGLVL(Debug, "ProcessDataProvider start to update process list");

		std::set<ProcId> vTerminateList;
		{
			std::shared_lock _lock(m_mtxProc);
			for (auto pProc : m_pProcMap)
				vTerminateList.insert(pProc.first);
		}

		std::vector<uint8_t> pBuffer;
		if (!querySystemInformation(SystemProcessInformation, pBuffer) || pBuffer.data() == nullptr)
			return;

		auto pSPI = (SYSTEM_PROCESS_INFORMATION*)pBuffer.data();
		auto pEnd = pBuffer.data() + pBuffer.size();
		while ((uint8_t*)pSPI >= pBuffer.data() && (uint8_t*)pSPI < pEnd)
		{
			// Terminate if service stop before we finish
			if (!m_fInitialized)
				break;

			CMD_TRY
			{
				auto vProcessBase = Dictionary({
					{"pid", (Pid)(UINT_PTR)pSPI->UniqueProcessId},
					{"imageName", std::wstring(pSPI->ImageName.Buffer, pSPI->ImageName.Length / sizeof(wchar_t))} });
				auto vProcInfo = enrichProcessInfo(vProcessBase);
				if (!vProcInfo.isEmpty())
					vTerminateList.erase(vProcInfo["id"]);
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL, FMT("Fail to get info for process <" << pSPI->UniqueProcessId << ">"));
			}

			if (pSPI->NextEntryOffset == 0)
				break;

			pSPI = (SYSTEM_PROCESS_INFORMATION*)((uint8_t*)pSPI + pSPI->NextEntryOffset);
		}

		// Mark processes as terminated 
		{
			for (auto nTermId : vTerminateList)
			{
				{
					std::shared_lock _lock(m_mtxProc);
					auto itProc = m_pProcMap.find(nTermId);
					if (itProc == m_pProcMap.end())
						continue; // Already purged
					if (itProc->second.data.has("deletionTime"))
						continue; // Already terminated
				}
				(void)enrichProcessInfo(Dictionary({
					{"id", nTermId},
					{"deletionTime", getCurrentTime()},
					{"exitCode", 0} })
				);
			}
		}
		LOGLVL(Debug, "ProcessDataProvider finished updating the process list");
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.log(SL, "Fail to update process list");
	}
}

//
//
//
void ProcessDataProvider::purgeAllProcessInfo()
{
	LOGLVL(Debug, "Purge started <ProcessDataProvider>.");

	Size nSizeBefore = 0;
	std::unordered_set<ProcId> vProcList;
	{
		std::shared_lock _lock(m_mtxProc);
		for (auto vProc : m_pProcMap)
			if (vProc.second.data.has("deletionTime"))
				vProcList.insert(vProc.first);
		nSizeBefore = m_pProcMap.size();
	}

	for (auto nProcId : vProcList)
		purgeProcessInfo(nProcId);

	{
		std::shared_lock _lock(m_mtxProc);
		LOGLVL(Debug, "Purge finished <ProcessDataProvider>. Size before <" << nSizeBefore << ">, size after <" << m_pProcMap.size() << ">");
	}
}

//
//
//
void ProcessDataProvider::purgeProcessInfo(ProcId nId)
{
	Variant vParent;
	Pid nPid = 0;
	{
		std::shared_lock _lock(m_mtxProc);
		auto itProc = m_pProcMap.find(nId);
		if (itProc == m_pProcMap.end())
			return;

		auto vProc = itProc->second.data;
		// Check if process is alive
		if (!vProc.has("deletionTime"))
			return;

		Time nCurTime = getTickCount();
		Time nDelTime = itProc->second.accessTime;
		if (nCurTime < nDelTime || nCurTime - nDelTime < m_nPurgeTimeout)
			return;

		// Check other process has our as parent
		for (auto itProc2 : m_pProcMap)
		{
			auto vParent2 = itProc2.second.data.get("parent", {});
			if (!vParent2.isEmpty() && vParent2.has("id") && vParent2["id"] == nId)
				return;
		}

		vParent = itProc->second.data.get("parent", {});
		nPid = vProc.get("pid");
	}

	{
		// Delete process info
		std::unique_lock _lock(m_mtxProc);
		auto itPid = m_pPidMap.find(nPid);
		if (itPid != m_pPidMap.end() && itPid->second == nId)
			m_pPidMap.erase(itPid);
		m_pProcMap.erase(nId);
	}
	LOGLVL(Debug, "Purge process: " << nPid << "(" << nId << ")");

	if (!vParent.isEmpty() && vParent.has("id"))
		purgeProcessInfo(vParent["id"]);
}

//
//
//
Variant ProcessDataProvider::getParentsChain(Variant vProcess, bool fAddProc)
{
	TRACE_BEGIN
	Sequence vChain;
	while (!vProcess.isEmpty() && vProcess.has("id"))
	{
		Variant vParent = vProcess.get("parent", {});
		if (fAddProc)
		{
			Variant vProcCopy = vProcess.clone();
			vProcCopy.erase("parent"); // Unlink parent
			vChain.insert(0, vProcCopy);
		}
		fAddProc = true;
		vProcess = vParent;
	}
	return vChain;
	TRACE_END(FMT("Fail to get process"))
}

//
//
//
void ProcessDataProvider::loadState(Variant vState)
{
}

//
//
//
openEdr::Variant ProcessDataProvider::saveState()
{
	return {};
}

//
//
//
void ProcessDataProvider::start()
{
	TRACE_BEGIN
	LOGLVL(Detailed, "Process Data Provider is being started");

	std::scoped_lock _lock(m_mtxStartStop);
	if (m_fInitialized)
	{
		LOGINF("Process Data Provider already started");
		return;
	}
	
	m_nProcCount = 0;
	
	m_fInitialized = true;
	putCatalogData("os.processes", Dictionary()); // Clear process list
	//m_threadPool.run(&ProcessDataProvider::updateProcessList, getPtrFromThis(this));
	m_pUpdateProcThread = std::thread([this]() { this->updateProcessList(); });
	// TODO: add periodical process list updates without driver

	LOGLVL(Detailed, "Process Data Provider is started");
	TRACE_END("Fail to start Process Data Provider");
}

//
//
//
void ProcessDataProvider::stop()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "Process Data Provider is being stopped");

	std::scoped_lock _lock(m_mtxStartStop);
	if (!m_fInitialized)
		return;
	m_fInitialized = false;

	if (m_pUpdateProcThread.joinable())
		m_pUpdateProcThread.join();

	LOGLVL(Detailed, "Process Data Provider is stopped");
	TRACE_END("Fail to stop Process Data Provider");
}

//
//
//
void ProcessDataProvider::shutdown()
{
	LOGLVL(Detailed, "Process Data Provider is being shutdowned");

	{
		std::unique_lock _lock(m_mtxProc);
		m_pProcMap.clear();
		m_pPidMap.clear();
	}

	LOGLVL(Detailed, "Process Data Provider is shutdowned");
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * 'objects.processDataProvider'
///
/// #### Supported commands
///
Variant ProcessDataProvider::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant ProcessDataProvider::execute()
	///
	/// ##### getProcessInfo()
	/// Adds the process to processes list.
	///   * **id** [int] - process identifier.
	///   * **pid** [int] - process PID.
	/// Returns a process descriptor.
	///
	if (vCommand == "getProcessInfo")
	{
		return enrichProcessInfo(vParams);
	}

	///
	/// @fn Variant ProcessDataProvider::execute()
	///
	/// ##### getParentsChain()
	/// Get the chain of the process parents.
	///   * **data** [variant] - process descriptor.
	/// Returns a sequence of processes (first is root).
	///
	if (vCommand == "getParentsChain")
	{
		Variant vProcess = vParams["data"];
		bool fAddSelf = vParams.get("addSelf", true);
		Variant vParentCain = getParentsChain(vProcess, fAddSelf);
#ifdef _DEBUG
		LOGLVL(Trace, "Parents chain: " << vParentCain.print());
#endif
		return vParentCain;
	}

	///
	/// @fn Variant ProcessDataProvider::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	if (vCommand == "start")
	{
		start();
		return {};
	}

	///
	/// @fn Variant ProcessDataProvider::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	if (vCommand == "stop")
	{
		stop();
		return {};
	}

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("ProcessDataProvider doesn't support a command <"
		<< vCommand << ">")).throwException();
}

} // namespace win
} // namespace sys
} // namespace openEdr 

 /// @}
