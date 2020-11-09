//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (31.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Processes access filter (self protection)
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "osutils.h"
#include "objmon.h"
#include "fltport.h"

namespace openEdr {
namespace objmon {

//////////////////////////////////////////////////////////////////////////
//
// Sending events
//
//////////////////////////////////////////////////////////////////////////

namespace detail {

//
// Universal file event sending
//
template<typename Fn>
NTSTATUS sendObjectEvent(SysmonEvent eEvent, ULONG_PTR nProcessId, Fn fnWriteAdditionalData)
{
	NonPagedLbvsSerializer<edrdrv::EventField> serializer;

	// +FIXME : Why STATUS_NO_MEMORY?
	// Cause only no memory can lead to error.
	if (!serializer.write(EvFld::RawEventId, uint16_t(eEvent))) return STATUS_NO_MEMORY;
	//if (!serializer.write(EvFld::Time, getSystemTime())) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::TickTime, getTickCount64())) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessPid, (uint32_t)nProcessId)) return STATUS_NO_MEMORY;

	IFERR_RET(fnWriteAdditionalData(&serializer));

	return fltport::sendRawEvent(serializer);
}


} // namespace detail

//
// Universal file event sending
//
template<typename Fn>
void sendObjectEvent(SysmonEvent eEvent, Fn fnWriteAdditionalData)
{
	auto nProcessId = (ULONG_PTR)PsGetCurrentProcessId();

	IFERR_LOG(detail::sendObjectEvent(eEvent, nProcessId, fnWriteAdditionalData),
		"Can't send object event %u pid: %Iu.\r\n", (ULONG)eEvent, nProcessId);
}

//
//
//
GENERIC_MAPPING c_processGenericMapping =
{
	0x21410, // GenericRead
	0x20BEA, // GenericWrite
	0x121001, // GenericExecute
	PROCESS_ALL_ACCESS // GenericAll
};

//
// Expand Generic in desiredAccess for processes
//
ACCESS_MASK expandProcessDesiredAccess(ACCESS_MASK src)
{
	RtlMapGenericMask(&src, &c_processGenericMapping);
	return src;
}

#define PROCESS_TERMINATE                  (0x0001)  
#define PROCESS_CREATE_THREAD              (0x0002)  
#define PROCESS_SET_SESSIONID              (0x0004)  
#define PROCESS_VM_OPERATION               (0x0008)  
#define PROCESS_VM_READ                    (0x0010)  
#define PROCESS_VM_WRITE                   (0x0020)  
#define PROCESS_DUP_HANDLE                 (0x0040)  
#define PROCESS_CREATE_PROCESS             (0x0080)  
#define PROCESS_SET_QUOTA                  (0x0100)  
#define PROCESS_SET_INFORMATION            (0x0200)  
#define PROCESS_QUERY_INFORMATION          (0x0400)  
#define PROCESS_SUSPEND_RESUME             (0x0800)  
#define PROCESS_QUERY_LIMITED_INFORMATION  (0x1000)  
#define PROCESS_SET_LIMITED_INFORMATION    (0x2000)  


//
//
//
struct ProcessCallContext
{
	HANDLE nInitiatorPid = nullptr;
	HANDLE nTargetPid = nullptr;

	ProcessCallContext(HANDLE nPidInitiator_, HANDLE nTargetPid_) :
		nTargetPid(nTargetPid_), nInitiatorPid(nPidInitiator_)
	{
	}
};

//
// Process objects hook
//
OB_PREOP_CALLBACK_STATUS preProcessObjectAccess(PVOID /*RegistrationContext*/, 
	POB_PRE_OPERATION_INFORMATION preOpInfo)
{
	// We should collect data on preAction
	// because we don't have correct all information on postAction

	if (!g_pCommonData->fObjMonStarted)
		return OB_PREOP_SUCCESS;

	// Skip if access is from kernel
	if (preOpInfo->KernelHandle)
		return OB_PREOP_SUCCESS;

	// Check parameters
	if ((PEPROCESS)preOpInfo->Object == nullptr)
		return OB_PREOP_SUCCESS;

	// Extract operation information
	HANDLE nInitiatorPid = PsGetCurrentProcessId(); //< Accessor
	HANDLE nTargetPid = PsGetProcessId((PEPROCESS)preOpInfo->Object); //< target object
	HANDLE nDstPid = nullptr; //< Destination process (which will have new handle)
	ACCESS_MASK* pDesiredAccess = nullptr;
	if (preOpInfo->Operation == OB_OPERATION_HANDLE_CREATE)
	{
		nDstPid = nInitiatorPid;
		pDesiredAccess = &preOpInfo->Parameters->CreateHandleInformation.DesiredAccess;
	}
	else if (preOpInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE)
	{
		auto& pInfo = preOpInfo->Parameters->DuplicateHandleInformation;
		nDstPid = PsGetProcessId((PEPROCESS)pInfo.TargetProcess);
		// TODO: Skip event if SourceProcess == nInitiatorPid
		pDesiredAccess = &pInfo.DesiredAccess;
	}
	else
	{
		return OB_PREOP_SUCCESS;
	}

	ACCESS_MASK eOrigDesiredAccess = *pDesiredAccess;
	ACCESS_MASK eExpandedDesiredAccess = expandProcessDesiredAccess(eOrigDesiredAccess);

	//LOGINFO4("preOpenProcess: act:%u ini: %Iu, trg: %Iu, dst: %Iu, da:0x%08X.\r\n", 
	//	(ULONG)preOpInfo->Operation, (ULONG_PTR)nInitiatorPid, (ULONG_PTR)nTargetPid, 
	//	(ULONG_PTR)nDstPid, (ULONG)eOrigDesiredAccess );

	// Skip access to self
	if (nInitiatorPid == nTargetPid)
		return OB_PREOP_SUCCESS;

	// Get contexts
	procmon::ContextPtr pTargetCtx;
	procmon::ContextPtr pInitiatorCtx;
	IFERR_LOG(procmon::fillContext(nTargetPid, nullptr, pTargetCtx));
	IFERR_LOG(procmon::fillContext(nInitiatorPid, nullptr, pInitiatorCtx));

	// Skip access from parent to child
	bool fHasParentChildRelation = false;
	if (pTargetCtx)
	{
		auto nTargetParent = pTargetCtx->processInfo.nParentPid;
		fHasParentChildRelation = (nTargetParent == nInitiatorPid) || (nTargetParent == nDstPid);
	}

	// Check selfdefense
	do 
	{
		static constexpr ACCESS_MASK c_nDeniedAccessMask = PROCESS_TERMINATE | PROCESS_CREATE_THREAD |
			PROCESS_SET_SESSIONID | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_DUP_HANDLE |
			PROCESS_CREATE_PROCESS | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION |
			PROCESS_SUSPEND_RESUME | /*PROCESS_QUERY_LIMITED_INFORMATION |*/ PROCESS_SET_LIMITED_INFORMATION;

		// If has parent-child relations, allow all
		if(fHasParentChildRelation)
			break;
		// If Target is not protected, allow all
		if (!pTargetCtx || !pTargetCtx->fIsProtected)
			break;

		// If Initiator is trusted, allow all
		if(procmon::isProcessTrusted(pInitiatorCtx))
			break;

		// Skip access from csrss
		if(testFlag(pInitiatorCtx->processInfo.nFlags, (UINT32)ProcessInfoFlags::CsrssProcess))
			break;
		

		// If no denied access
		if((eExpandedDesiredAccess & c_nDeniedAccessMask) == 0)
			break;
		
		// partial restrict access
		*pDesiredAccess = eExpandedDesiredAccess & ~c_nDeniedAccessMask;

		if (*pDesiredAccess == 0)
			*pDesiredAccess = PROCESS_QUERY_LIMITED_INFORMATION;

		LOGINFO1("Selfdefense: Partial restrict access: pid: %Iu, operation: %u, targetPid: %Iu, oldAccess: 0x%08X, newAccess: 0x%08X.\r\n", 
			(ULONG_PTR)nInitiatorPid, (ULONG)preOpInfo->Operation, 
			(ULONG_PTR)nTargetPid, (ULONG)eOrigDesiredAccess, (ULONG)*pDesiredAccess);
	} while (false);
	

	// Filtering events for sending
	bool fSendEvent = false;
	do 
	{
		if (!g_pCommonData->fEnableMonitoring)
			break;
		if (fHasParentChildRelation)
			break;
		// Skip white list processes
		if (procmon::isProcessInWhiteList(pInitiatorCtx))
			break;
		if (!isEventEnabled(SysmonEvent::ProcessOpen))
			break;

		// Skip terminated processes
		if (pTargetCtx && pTargetCtx->processInfo.fIsTerminated)
			break;

		fSendEvent = true;
	} while (false);


	// All check are completed - init postOperation call context
	if (fSendEvent)
		preOpInfo->CallContext = new (NonPagedPool) ProcessCallContext(nInitiatorPid, nTargetPid);

	return OB_PREOP_SUCCESS;
}

//
//
//
VOID postProcessObjectAccess(PVOID /*RegistrationContext*/, POB_POST_OPERATION_INFORMATION postOpInfo)
{
	// Skip action skipped by preAction
	if (postOpInfo->CallContext == nullptr)
		return;

	UniquePtr<ProcessCallContext> pCallContext((ProcessCallContext*)postOpInfo->CallContext);

	// Skip unsuccessful
	if (!NT_SUCCESS(postOpInfo->ReturnStatus))
		return;

	// Check GrantedAccess
	ACCESS_MASK eGrantedAccess = (postOpInfo->Operation == OB_OPERATION_HANDLE_CREATE) ?
		postOpInfo->Parameters->CreateHandleInformation.GrantedAccess :
		postOpInfo->Parameters->DuplicateHandleInformation.GrantedAccess;

	// All but querying information
	static constexpr ACCESS_MASK eDetectedAccessMask = PROCESS_TERMINATE | PROCESS_CREATE_THREAD |
		PROCESS_SET_SESSIONID | PROCESS_VM_OPERATION | /*PROCESS_VM_READ |*/ PROCESS_VM_WRITE | /*PROCESS_DUP_HANDLE |*/
		PROCESS_CREATE_PROCESS | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | /*PROCESS_QUERY_INFORMATION |*/
		PROCESS_SUSPEND_RESUME /*| PROCESS_QUERY_LIMITED_INFORMATION*/ /*| PROCESS_SET_LIMITED_INFORMATION*/;

	if (!FlagOn(eGrantedAccess, eDetectedAccessMask))
		return;

	// Filter repeated openProcess
	bool fSendEvent = true;
	do 
	{
		procmon::ContextPtr pInitiatorCtx;
		IFERR_LOG(procmon::fillContext(pCallContext->nInitiatorPid, nullptr, pInitiatorCtx));
		if (!pInitiatorCtx)
			break;

		uint64_t nCurTime = getTickCount64();

		{
			ScopedLock lock(g_pCommonData->mtxOpenProcessFilter);
			auto pFilter = pInitiatorCtx->pOpenProcessFilter;

			// Check event repeat
			procmon::Context::OpenProcessInfo* pOpenProcessInfo = nullptr;
			IFERR_LOG(pFilter->findOrInsert((uint32_t)(ULONG_PTR) pCallContext->nTargetPid, &pOpenProcessInfo));
			if (pOpenProcessInfo != nullptr)
			{
				if ((nCurTime - pOpenProcessInfo->m_LastOpenSend) < g_pCommonData->nOpenProcessRepeatTimeout)
					fSendEvent = false;
				else
					pOpenProcessInfo->m_LastOpenSend = nCurTime;
			}

			// Garbage collecting
			static constexpr uint64_t c_nGarbageCollectTimeout = 5 /*min*/ * 60 /*sec*/ * 1000 /*ms*/;
			if (fSendEvent && nCurTime - pInitiatorCtx->nLastGarbageCollectTime > c_nGarbageCollectTimeout)
			{
				auto endIter = pFilter->end();
				for (auto iter = pFilter->begin(); iter != endIter;)
				{
					if (nCurTime - iter->second.m_LastOpenSend < c_nGarbageCollectTimeout)
						++iter;
					else
						iter = pFilter->remove(iter);
				}
				pInitiatorCtx->nLastGarbageCollectTime = nCurTime;
			}
		}
	} while (false);

	if (!fSendEvent)
		return;

	LOGINFO2("sendEvent: %u (objectEvent), pid: %Iu, target: %Iu, access: 0x%08X, oper:%u, status: 0x%08X.\r\n", 
		(ULONG)SysmonEvent::ProcessOpen, (ULONG_PTR)pCallContext->nInitiatorPid, 
		(ULONG_PTR)pCallContext->nTargetPid, (ULONG)eGrantedAccess, (ULONG) postOpInfo->Operation, 
		(ULONG) postOpInfo->ReturnStatus);

	// Send event
	sendObjectEvent(SysmonEvent::ProcessOpen, 
		[eGrantedAccess, &pCallContext](auto pSerializer) {
			if (!pSerializer->write(EvFld::TargetProcessPid, (uint32_t) (ULONG_PTR)pCallContext->nTargetPid)) 
				return STATUS_NO_MEMORY;
			if (!pSerializer->write(EvFld::AccessMask, uint32_t(eGrantedAccess)))
				return STATUS_NO_MEMORY;
			return STATUS_SUCCESS;
		}
	);
}

//////////////////////////////////////////////////////////////////////////
//
// Create/Terminate process callback
//
namespace detail {

//
//
//
void notifyOnProcessCreation(procmon::Context* pNewProcessCtx, 
	procmon::Context* pParentProcessCtx)
{
	if (!g_pCommonData->fObjMonStarted)
		return;

	// Protection inheritance
	if (pParentProcessCtx->fIsProtected)
		pNewProcessCtx->fIsProtected;

	return;
}

//
//
//
void notifyOnProcessTermination(procmon::Context* /*pProcessCtx*/)
{
}

} // namespace detail

//
// Initialization
//
NTSTATUS initialize()
{
	if (g_pCommonData->fObjMonStarted)
		return STATUS_SUCCESS;

	g_pCommonData->hProcFltCallbackRegistration = nullptr;

	// Set hooks
	{
		OB_OPERATION_REGISTRATION stObOpReg[2] = {};
		OB_CALLBACK_REGISTRATION stObCbReg = {};

		USHORT OperationRegistrationCount = 0;

		// Processes callbacks
		stObOpReg[OperationRegistrationCount].ObjectType = PsProcessType;
		stObOpReg[OperationRegistrationCount].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
		stObOpReg[OperationRegistrationCount].PreOperation = preProcessObjectAccess;
		stObOpReg[OperationRegistrationCount].PostOperation = postProcessObjectAccess;
		OperationRegistrationCount += 1;

		stObCbReg.Version = OB_FLT_REGISTRATION_VERSION;
		stObCbReg.OperationRegistrationCount = OperationRegistrationCount;
		stObCbReg.OperationRegistration = stObOpReg;
		RtlInitUnicodeString(&stObCbReg.Altitude, edrdrv::c_sAltitudeValue);

		IFERR_RET(ObRegisterCallbacks(&stObCbReg, &g_pCommonData->hProcFltCallbackRegistration));
	}

	g_pCommonData->fObjMonStarted = TRUE;

	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (!g_pCommonData->fObjMonStarted) return;

	// Reset hooks
	if (g_pCommonData->hProcFltCallbackRegistration != nullptr)
	{
		ObUnRegisterCallbacks(g_pCommonData->hProcFltCallbackRegistration);
		g_pCommonData->hProcFltCallbackRegistration = nullptr;
	}

	g_pCommonData->fObjMonStarted = FALSE;
}

} // namespace objmon
} // namespace openEdr
/// @}
