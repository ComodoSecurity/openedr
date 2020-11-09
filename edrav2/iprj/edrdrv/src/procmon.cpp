//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (31.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Process Context
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "procmon.h"
#include "objmon.h"
#include "procutils.h"
#include "osutils.h"
#include "fltport.h"
#include "dllinj.h"
#include "config.h"

namespace openEdr {
namespace procmon {

//
// Predefinitions
//

enum class ProcessOption
{
	Invalid,
	Trusted,
	Protected,
	EnableInject,
	SendEvent,
};

//
//
//
inline ProcessOption convertToProcessOption(RuleType eRuleType)
{
	switch (eRuleType)
	{
	case RuleType::Trusted:
		return ProcessOption::Trusted;
	case RuleType::Protected:
		return ProcessOption::Protected;
	case RuleType::SendEvent:
		return ProcessOption::SendEvent;
	case RuleType::EnableInject:
		return ProcessOption::EnableInject;
	default:
		return ProcessOption::Invalid;
	}
}

//
//
//
inline RuleType convertToRuleType(ProcessOption eProcessOption)
{
	switch (eProcessOption)
	{
	case ProcessOption::Trusted:
		return RuleType::Trusted;
	case ProcessOption::Protected:
		return RuleType::Protected;
	case ProcessOption::SendEvent:
		return RuleType::SendEvent;
	case ProcessOption::EnableInject:
		return RuleType::EnableInject;
	default:
		return RuleType::Invalid;
	}
}



inline OptionState getDefaultState(ProcessOption eProcessOption);

//////////////////////////////////////////////////////////////////////////
//
// alloc, free, refCnt
//

//
// +FIXME: Why it is not a member of Context? What about RAII?
// Completely agree.
//
LONG incRef(Context* pCtx)
{
	return InterlockedIncrement(&pCtx->_RefCount);
}

//
//
//
LONG decRef(Context* pCtx)
{
	ULONG nNewRefCount = InterlockedDecrement(&pCtx->_RefCount);
	if (nNewRefCount == 0)
	{
		pCtx->free();
		ExFreePoolWithTag(pCtx, c_nAllocTag);
	}
	return nNewRefCount;
}

//
//
//
NTSTATUS allocContext(Context** ppCtx, HANDLE ProcessId)
{
	auto pCtx = (Context*)ExAllocatePoolWithTag(NonPagedPool, 
		sizeof(Context), c_nAllocTag);
	if (pCtx == NULL) return LOGERROR(STATUS_NO_MEMORY);

	NTSTATUS eStatus = pCtx->init();
	if (!NT_SUCCESS(eStatus))
	{
		ExFreePoolWithTag(pCtx, c_nAllocTag);
		return eStatus;
	}

	pCtx->_ProcessId = ProcessId;
	incRef(pCtx);

	*ppCtx = pCtx;
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS Context::init()
{
	RtlZeroMemory(this, sizeof(*this));

	NTSTATUS eResult = STATUS_SUCCESS;

	do 
	{
		processInfo.init();

		fSendEvents = getDefaultState(ProcessOption::SendEvent);
		fEnableInject = getDefaultState(ProcessOption::EnableInject);
		fIsProtected = getDefaultState(ProcessOption::Protected);
		fIsTrusted = getDefaultState(ProcessOption::Trusted);

		pOpenProcessFilter = new (NonPagedPool) OpenProcessFilter;
		if (pOpenProcessFilter == NULL) 
		{
			eResult = STATUS_NO_MEMORY;
			break;
		}

		pRegMonEventFilter = new (NonPagedPool) RegMonEventFilter(
			/*nMaxGCperiod = */ 5 /*min*/ * 60 /*sec*/ * 1000 /*ms*/, /*nMaxGCEventCount = */ 100);
		if (pRegMonEventFilter == NULL)
		{
			eResult = STATUS_NO_MEMORY;
			break;
		}

	} while (false);

	if (!NT_SUCCESS(eResult))
	{
		if (pOpenProcessFilter != nullptr)
			delete pOpenProcessFilter;
		if (pRegMonEventFilter != nullptr)
			delete pRegMonEventFilter;
	}

	return eResult;
}

//
//
//
void Context::free()
{
	processInfo.free();
	delete pOpenProcessFilter;
	delete pRegMonEventFilter;
}

//////////////////////////////////////////////////////////////////////////
//
// Context list (must be called with synchronization)
//

//
// It should be called after locking of g_pCommonData->mtxProcessContextList
//
void freeContextList()
{
	// never initialized
	if (g_pCommonData->processContextList.Blink == NULL) return;

	auto pHead = &g_pCommonData->processContextList;
	while (!IsListEmpty(pHead))
	{
		auto pRemoved = RemoveHeadList(pHead);
		Context* pCtx = CONTAINING_RECORD(pRemoved, Context, _ListEntry);
		decRef(pCtx);
	}
}

//
// +FIXME: Can we combine it with allocContext()
// It is separation of list management and memory management (initialization) functions
//
// It should be synchronized by g_pCommonData->mtxProcessContextList
//
void addContextToList(Context* pCtx)
{
	incRef(pCtx);
	InsertHeadList(&g_pCommonData->processContextList, &pCtx->_ListEntry);
}

//
// Search a process context
// It should be called after locking of g_pCommonData->mtxProcessContextList
// @return Context or nullptr
//
Context* findContextInList(HANDLE ProcessId)
{
	auto pHead = &g_pCommonData->processContextList;
	
	for (auto pCur = pHead->Flink; pCur != pHead; pCur = pCur->Flink)
	{
		Context* pCtx = CONTAINING_RECORD(pCur, Context, _ListEntry);
		if (pCtx->_ProcessId == ProcessId)
		{
			incRef(pCtx);
			return pCtx;
		}
	}

	return nullptr;
}

//
// It should be called after locking of g_pCommonData->mtxProcessContextList
//
size_t getContextCountInList()
{
	size_t nCount = 0;
	auto pHead = &g_pCommonData->processContextList;

	for (auto pCur = pHead->Flink; pCur != pHead; pCur = pCur->Flink)
		nCount++;

	return nCount;
}

//
// It should be called after locking of g_pCommonData->mtxProcessContextList
//
void removeContextFromList(HANDLE ProcessId, Context** ppCtx = nullptr)
{
	auto pHead = &g_pCommonData->processContextList;

	for (auto pCur = pHead->Flink; pCur != pHead; pCur = pCur->Flink)
	{
		Context* pCtx = CONTAINING_RECORD(pCur, Context, _ListEntry);
		if (pCtx->_ProcessId == ProcessId)
		{
			RemoveEntryList(&pCtx->_ListEntry);
			if (ppCtx != nullptr)
				*ppCtx = pCtx;
			else
				decRef(pCtx);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Public function
//

///
/// Get a context by PID.
///
/// If process is present and context is absent and fAllowCreateNew is true,
/// creates a new context and returns it.
///
/// @param ProcessId[in] - PID of a requested process.
/// @param ppCtx[out] - context.
/// @param fAllowCreateNew[in] - create a new context if it is absent.
/// @return The function return a NTSTATUS of the operation.
///
NTSTATUS getContext(HANDLE ProcessId, Context** ppCtx, bool fAllowCreateNew)
{
	if (ppCtx == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);

	if (!g_pCommonData->fProcCtxNotificationIsStarted) 
		return LOGERROR(STATUS_INVALID_PARAMETER);

	*ppCtx = nullptr;

	// Find and return exist context
	{
		SharedLock lock(g_pCommonData->mtxProcessContextList);
		Context* pCtx = findContextInList(ProcessId);
		if (pCtx != nullptr)
		{
			*ppCtx = pCtx;
			return STATUS_SUCCESS;
		}
	}

	if (!fAllowCreateNew)
		return STATUS_NOT_FOUND;

	// Add new context
	{
		UniqueLock lock(g_pCommonData->mtxProcessContextList);

		// Repeate find exist (it can be added)
		Context* pCtx = findContextInList(ProcessId);

		// if not found - create new
		if (pCtx == nullptr)
		{
			IFERR_RET(allocContext(&pCtx, ProcessId));
			addContextToList(pCtx);
		}

		*ppCtx = pCtx;
		return STATUS_SUCCESS;
	}
}

//
//
//
void releaseContext(Context* pCtx)
{
	if (pCtx != nullptr)
		decRef(pCtx);
}


NTSTATUS getContext(HANDLE nProcessId, ContextPtr& pCtx, bool fAllowCreateNew)
{
	Context* pRawCtx = nullptr;
	IFERR_RET_NOLOG(getContext(nProcessId, &pRawCtx, fAllowCreateNew));
	pCtx.reset(pRawCtx);
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS applyForEachProcessCtx(NTSTATUS(*fnProcess)(Context* /*pProcCtx*/, void* /*pCallContext*/), void* pCallContext)
{
	if (!g_pCommonData->fProcCtxNotificationIsStarted)
		return LOGERROR(STATUS_UNSUCCESSFUL);

	// Clone context list
	List<ContextPtr> ctxList;
	{
		SharedLock lock(g_pCommonData->mtxProcessContextList);

		auto pHead = &g_pCommonData->processContextList;

		for (auto pCur = pHead->Flink; pCur != pHead; pCur = pCur->Flink)
		{
			Context* pCtx = CONTAINING_RECORD(pCur, Context, _ListEntry);
			incRef(pCtx);
			ContextPtr ctxPtr(pCtx);
			IFERR_RET(ctxList.pushBack(std::move(ctxPtr)));
		}
	}

	// Apply fnProcess
	for (auto& pCtx : ctxList)
	{
		NTSTATUS ns = fnProcess(pCtx, pCallContext);
		IFERR_RET(ns);
		// Breaking loop if ns != STATUS_SUCCESS
		if (ns != STATUS_SUCCESS)
			return ns;
	}

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Rules
//
//////////////////////////////////////////////////////////////////////////

//
// Select ConfigId by eProcessOption
//
inline cfg::ConfigId getConfigId(ProcessOption eProcessOption)
{
	switch (eProcessOption)
	{
	case ProcessOption::Trusted:
		return cfg::ConfigId::TrustedProcess;
	case ProcessOption::Protected:
		return cfg::ConfigId::ProtectedProcess;
	case ProcessOption::SendEvent:
		return cfg::ConfigId::SendEventProcess;
	case ProcessOption::EnableInject:
		return cfg::ConfigId::InjectProcess;
	default:
		return cfg::ConfigId::Invalid;
	}
}

//
// Select ProcessOptionRules by eProcessOption
//
inline ProcessOptionRules* getRules(ProcessOption eProcessOption)
{
	switch (eProcessOption)
	{
	case ProcessOption::Trusted:
		return &g_pCommonData->processRules.trustedRules;
	case ProcessOption::Protected:
		return &g_pCommonData->processRules.protectedRules;
	case ProcessOption::SendEvent:
		return &g_pCommonData->processRules.sendEventRules;
	case ProcessOption::EnableInject:
		return &g_pCommonData->processRules.enableInjectRules;
	default:
		return nullptr;
	}
}

//
// Returns default state
//
inline OptionState getDefaultState(ProcessOption eProcessOption)
{
	switch (eProcessOption)
	{
	case ProcessOption::Trusted:
	case ProcessOption::Protected:
		return OptionState(false, OptionState::Reason::Default, false);
	case ProcessOption::SendEvent:
	case ProcessOption::EnableInject:
		return OptionState(true, OptionState::Reason::Default, false);
	default:
		return OptionState(false, OptionState::Reason::Default, false);
	}
}

//
//
//
inline OptionState* getProcessOptionState(Context* pCtx, ProcessOption eProcessOption)
{
	if (pCtx == nullptr)
		return nullptr;

	switch (eProcessOption)
	{
	case ProcessOption::Trusted:
		return &pCtx->fIsTrusted;
	case ProcessOption::Protected:
		return &pCtx->fIsProtected;
	case ProcessOption::SendEvent:
		return &pCtx->fSendEvents;
	case ProcessOption::EnableInject:
		return &pCtx->fEnableInject;
	default:
		return nullptr;
	}
}

//
//
//
inline const char* getOptionStateName(ProcessOption eProcessOption)
{
	switch (eProcessOption)
	{
	case ProcessOption::Trusted:
		return "trusted";
	case ProcessOption::Protected:
		return "protected";
	case ProcessOption::SendEvent:
		return "sendEvent";
	case ProcessOption::EnableInject:
		return "enableInject";
	default:
		return "INVALID";
	}
}


//
//
//
NTSTATUS loadRulesFromConfig(ProcessOption eProcessOption)
{
	cfg::ConfigId eCfgId = getConfigId(eProcessOption);
	if (eCfgId == cfg::ConfigId::Invalid)
		return LOGERROR(STATUS_INVALID_PARAMETER);

	Blob data;
	IFERR_RET(cfg::loadConfigBlob(eCfgId, data));
	if (data.isEmpty())
		return STATUS_SUCCESS;

	IFERR_LOG(updateProcessRules(data.getData(), data.getSize()));
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS saveRulesToConfig(ProcessOption eProcessOption)
{
	// Select ConfigId
	cfg::ConfigId eCfgId = getConfigId(eProcessOption);
	if (eCfgId == cfg::ConfigId::Invalid)
		return LOGERROR(STATUS_INVALID_PARAMETER);

	// Select rules list for save
	ProcessOptionRules* pProcessRules = getRules(eProcessOption);
	if(pProcessRules == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);

	// Serialization
	bool fRemoveConfig = false;
	NonPagedLbvsSerializer<UpdateRulesField> serializer;
	do 
	{
		SharedLock rulesLock(g_pCommonData->mtxProcessRules);

		// If list is empty - delete config
		if (pProcessRules->persistRules.isEmpty())
		{
			fRemoveConfig = true;
			break;
		}

		if (!serializer.write(UpdateRulesField::Type, (uint32_t)convertToRuleType(eProcessOption))) return LOGERROR(STATUS_NO_MEMORY);
		if (!serializer.write(UpdateRulesField::Persistent, true)) return LOGERROR(STATUS_NO_MEMORY);
		if (!serializer.write(UpdateRulesField::Mode, (uint32_t)UpdateRulesMode::Replace)) return LOGERROR(STATUS_NO_MEMORY);

		for (auto& rule : pProcessRules->persistRules)
		{
			if (!serializer.write(UpdateRulesField::Rule)) return LOGERROR(STATUS_NO_MEMORY);
			if (!serializer.write(UpdateRulesField::RuleValue, (bool)rule.m_fValue)) return LOGERROR(STATUS_NO_MEMORY);
			if (!rule.usPathMask.isEmpty())
				if (!write(serializer, UpdateRulesField::RuleImagePath, rule.usPathMask)) return LOGERROR(STATUS_NO_MEMORY);
			if (!rule.usTag.isEmpty())
				if (!write(serializer, UpdateRulesField::RuleTag, rule.usTag)) return LOGERROR(STATUS_NO_MEMORY);
			if (rule.fInherit)
				if (!serializer.write(UpdateRulesField::RuleInherite, true)) return LOGERROR(STATUS_NO_MEMORY);
		}
	} while (false);

	// Save
	if (fRemoveConfig)
	{
		IFERR_RET(cfg::removeConfigBlob(eCfgId));
	}
	else
	{
		IFERR_RET(cfg::saveConfigBlob(eCfgId, serializer));
	}
	return STATUS_SUCCESS;
}


//
// Apply optionRulesList
//
OptionState applyOptionRulesList(Context* pCtx, const ProcessOptionRuleList& rules)
{
	for (auto& rule : rules)
	{
		// check rule conditions
		if (!isEndedWith(pCtx->processInfo.pusImageName, (PCUNICODE_STRING)rule.usPathMask))
			continue;

		// apply rule
		return OptionState(rule.m_fValue, OptionState::Reason::ByRule, rule.fInherit);
	}
	return OptionState();
}

//
// Calculates specified option
// It should be called after locking of mtxProcessRules
//
NTSTATUS setProcessOptionState(Context* pCtx, Context* pParentCtx, ProcessOption eProcessOption)
{
	OptionState* pCurState = getProcessOptionState(pCtx, eProcessOption);
	if (pCurState == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);
	ProcessOptionRules* pRules = getRules(eProcessOption);
	if (pRules == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);


	// Option is not change if it was forced set
	if (pCurState->isForced())
		return STATUS_SUCCESS;

	// Try apply rules
	OptionState fStateFromRules;
	if(!fStateFromRules.isInitialized())
		fStateFromRules = applyOptionRulesList(pCtx, pRules->tempRules);
	if (!fStateFromRules.isInitialized())
		fStateFromRules = applyOptionRulesList(pCtx, pRules->persistRules);
	if (fStateFromRules.isInitialized())
	{
		LOGINFO2("setProcessOptionState from rule. pid: %Iu, %s: %u.\r\n",
			(ULONG_PTR)pCtx->processInfo.nPid, 
			getOptionStateName(eProcessOption), (ULONG)(bool)fStateFromRules);
		*pCurState = fStateFromRules;
		return STATUS_SUCCESS;
	}

	// Try apply inheritance
	OptionState* pParentState = getProcessOptionState(pParentCtx, eProcessOption);
	if (pParentState != nullptr && pParentState->isInhereted())
	{
		LOGINFO2("setProcessOptionState from parent. pid: %Iu, %s: %u.\r\n",
			(ULONG_PTR)pCtx->processInfo.nPid,
			getOptionStateName(eProcessOption), (ULONG)(bool)*pParentState);
		*pCurState = pParentState->copyWithReason(OptionState::Reason::Inhereted);
		return STATUS_SUCCESS;
	}

	// Apply default state
	*pCurState = getDefaultState(eProcessOption);
	return STATUS_SUCCESS;
}


//
//
//
NTSTATUS applyRules(Context* pCtx)
{
	if (pCtx == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);

	// Skip if info is not filled
	if (!pCtx->_fIsFilled)
		return STATUS_SUCCESS;

	ContextPtr pParentCtx;
	(void) getContext(pCtx->processInfo.nParentPid, pParentCtx, false);
	if (pParentCtx && !pParentCtx->_fIsFilled)
		pParentCtx.reset();

	// Set options for SYSTEM(4) or similar processes
	if (testFlag(pCtx->processInfo.nFlags, (UINT32)ProcessInfoFlags::SystemLikeProcess))
	{
		if (!pCtx->fEnableInject.isForced())
		{
			pCtx->fEnableInject = OptionState(false, OptionState::Reason::Forced, false);
		}
		if (!pCtx->fSendEvents.isForced())
		{
			pCtx->fSendEvents = OptionState(false, OptionState::Reason::Forced, false);
		}
	}

	// Set options for this product processes
	if (testFlag(pCtx->processInfo.nFlags, (UINT32)ProcessInfoFlags::ThisProductProcess))
	{
		// Always trusted
		if (!pCtx->fIsTrusted.isForced())
		{
			pCtx->fIsTrusted = OptionState(true, OptionState::Reason::Forced, false);
		}
	}

	// Set options for windows protected processes
	if (pCtx->processInfo.eProtectedType != PsProtectedTypeNone)
	{
		bool fOptionsWasUpdated = false;

		if (!pCtx->fEnableInject.isForced())
		{
			pCtx->fEnableInject = OptionState(false, OptionState::Reason::Forced, false);
			fOptionsWasUpdated = true;
		}
		if (!pCtx->fSendEvents.isForced())
		{
			pCtx->fSendEvents = OptionState(false, OptionState::Reason::Forced, false);
			fOptionsWasUpdated = true;
		}

		if(fOptionsWasUpdated)
			LOGINFO2("applyProcessRules for protected process. pid: %Iu.\r\n", (ULONG_PTR)pCtx->processInfo.nPid);
	}

	SharedLock rulesLock(g_pCommonData->mtxProcessRules);

	// calculate OptionState-s
	IFERR_LOG(setProcessOptionState(pCtx, pParentCtx, ProcessOption::EnableInject));
	IFERR_LOG(setProcessOptionState(pCtx, pParentCtx, ProcessOption::Trusted));
	IFERR_LOG(setProcessOptionState(pCtx, pParentCtx, ProcessOption::Protected));
	IFERR_LOG(setProcessOptionState(pCtx, pParentCtx, ProcessOption::SendEvent));
	return STATUS_SUCCESS;
}


//
//
//
NTSTATUS fillContext(HANDLE nPid, HANDLE hProcess, Context** ppCtx)
{
	if (ppCtx == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);

	ContextPtr pCtx;

	// Return only exist context on IRQL > PASSIVE_LEVEL
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		IFERR_RET_NOLOG(getContext(nPid, pCtx, false));
	}
	// Return exist context or try create on IRQL == PASSIVE_LEVEL
	else
	{
		IFERR_RET(getContext(nPid, pCtx, true));

		// Fill context
		bool fContextWasFilled = false;
		do 
		{
			if (pCtx->_fIsFilled)
				break;

			ScopedLock fillLock(g_pCommonData->mtxFillProcessContext);

			// Second check. Possible context was filled
			if (pCtx->_fIsFilled)
				break;

			// Fill CommonProcessInfo
			if (hProcess == nullptr)
				fillProcessInfoByPid(nPid, &pCtx->processInfo, false);
			else
				fillProcessInfoByHandle(nPid, hProcess, &pCtx->processInfo, false);

			pCtx->_fIsFilled = TRUE;
			fContextWasFilled = true;

			// Apply rules
			IFERR_LOG(applyRules(pCtx));

			// Don't add terminated processes
			if (pCtx->processInfo.fIsTerminated)
			{
				UniqueLock listLock(g_pCommonData->mtxProcessContextList);
				removeContextFromList(nPid);
			}
		} while (false);

		if (fContextWasFilled)
		{
			LOGINFO3("fillContext: pid: %Iu, term: %u.\r\n", (ULONG_PTR)pCtx->processInfo.nPid,
				(ULONG)pCtx->processInfo.fIsTerminated);
		}
	}

	// Set ret value
	*ppCtx = pCtx.release();
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS fillContext(HANDLE nPid, HANDLE hProcess, ContextPtr& pCtx)
{
	Context* pRawCtx = nullptr;
	IFERR_RET(fillContext(nPid, hProcess, &pRawCtx));
	pCtx.reset(pRawCtx);
	return STATUS_SUCCESS;
}


//
//
//
NTSTATUS reapplyRulesForAllProcesses()
{
	return applyForEachProcessCtx([](Context* pCtx) {
		IFERR_LOG(applyRules(pCtx));
		return STATUS_SUCCESS;
	});
}

//
//
//
size_t deleteRulesByTag(ProcessOptionRuleList& rules, PCUNICODE_STRING usTag)
{
	return rules.removeIf([usTag](const ProcessOptionRule& rule) -> bool
	{
		return rule.usTag.isEqual(usTag, false);
	});
}

//
//
//
size_t deleteRulesByTag(ProcessOptionRules& rules, PCUNICODE_STRING usTag)
{
	size_t nDeletedItems = 0;
	nDeletedItems += deleteRulesByTag(rules.persistRules, usTag);
	nDeletedItems += deleteRulesByTag(rules.tempRules, usTag);
	return nDeletedItems;
}

//
//
//
size_t deleteRulesByTag(ProcessRules& rules, PCUNICODE_STRING usTag)
{
	size_t nDeletedItems = 0;
	nDeletedItems += deleteRulesByTag(rules.enableInjectRules, usTag);
	nDeletedItems += deleteRulesByTag(rules.protectedRules, usTag);
	nDeletedItems += deleteRulesByTag(rules.sendEventRules, usTag);
	nDeletedItems += deleteRulesByTag(rules.trustedRules, usTag);
	return nDeletedItems;
}

//
// Print rules
//
void printProcessRuleList(const ProcessOptionRuleList& rules)
{
	ULONG nIndex = 0;
	for (auto& rule : rules)
	{
		LOGINFO1("rule#%02u: path: <%wZ>, val: %u, inh: %u, tag: <%wZ>.\r\n",
			nIndex, (PCUNICODE_STRING)rule.usPathMask, (ULONG)rule.m_fValue, (ULONG)rule.fInherit,
			(PCUNICODE_STRING)rule.usTag);
		++nIndex;
	}
}

//
// Print rules
//
void _printCurProcessRules(ProcessOption eProcessOption)
{
	ProcessOptionRules* pRules = getRules(eProcessOption);
	if (pRules == nullptr) return;

	LOGINFO1("rules: <%s>, non-persist: %u, persist: %u.\r\n", getOptionStateName(eProcessOption), 
		(ULONG)pRules->tempRules.getSize(), (ULONG)pRules->persistRules.getSize());
	printProcessRuleList(pRules->tempRules);
	printProcessRuleList(pRules->persistRules);
}


//
// Print rules
//
void printCurProcessRules()
{
	SharedLock rulesLock(g_pCommonData->mtxProcessRules);

	LOGINFO1("Process rules:\r\n");
	_printCurProcessRules(ProcessOption::EnableInject);
	_printCurProcessRules(ProcessOption::Trusted);
	_printCurProcessRules(ProcessOption::Protected);
	_printCurProcessRules(ProcessOption::SendEvent);
}

//
//
//
NTSTATUS updateProcessRules(const void* pData, size_t nDataSize)
{
	ProcessOption eProcessOption = ProcessOption::Invalid;
	UpdateRulesMode updateMode = UpdateRulesMode::Invalid;
	bool fPersistent = false;
	DynUnicodeString usTagForDelete;
	ProcessOptionRuleList rules;

	//////////////////////////////////////////////////////////////////////////
	//
	// Deserialize
	//

	variant::BasicLbvsDeserializer<UpdateRulesField> deserializer;
	if (!deserializer.reset(pData, nDataSize))
		return LOGERROR(STATUS_DATA_ERROR, "Invalid data format.\r\n");

	// Deserialize main data
	for (auto& item : deserializer)
	{
		switch (item.id)
		{
			// Process log file
			case UpdateRulesField::Type:
			{
				RuleType eRuleType = RuleType::Invalid;
				IFERR_RET(getEnumValue(item, eRuleType));
				eProcessOption = convertToProcessOption(eRuleType);
				continue; // goto next item
			}
			case UpdateRulesField::Persistent:
			{
				IFERR_RET(getValue(item, fPersistent));
				continue; // goto next item
			}
			case UpdateRulesField::Mode:
			{
				IFERR_RET(getEnumValue(item, updateMode));
				continue; // goto next item
			}
			case UpdateRulesField::Tag:
			{
				IFERR_RET(getValue(item, usTagForDelete));
				continue; // goto next item
			}
		}
	}


	// Deserialize rules.
	// Should be after main fields, cause it uses this data, but order is not determined
	for (auto& item : deserializer)
	{
		switch (item.id)
		{
			case UpdateRulesField::Rule:
			{
				IFERR_RET(rules.pushBack(ProcessOptionRule()));
				// Set default tag
				if (!usTagForDelete.isEmpty())
					IFERR_RET(rules.getBack().usTag.assign(usTagForDelete), "Can't assign tag");
				continue; // goto next item
			}
			case UpdateRulesField::RulePath:
			case UpdateRulesField::RuleImagePath:
			{
				if (rules.isEmpty())
					return LOGERROR(STATUS_INVALID_PARAMETER, "Unexpected field: %u.\r\n", (ULONG)item.id);
				IFERR_RET(getValue(item, rules.getBack().usPathMask));
				continue; // goto next item
			}
			case UpdateRulesField::RuleTag:
			{
				if (rules.isEmpty())
					return LOGERROR(STATUS_INVALID_PARAMETER, "Unexpected field: %u.\r\n", (ULONG)item.id);
				IFERR_RET(getValue(item, rules.getBack().usTag));
				continue; // goto next item
			}
			case UpdateRulesField::RuleInherite:
			case UpdateRulesField::RuleRecursive:
			{
				if (rules.isEmpty())
					return LOGERROR(STATUS_INVALID_PARAMETER, "Unexpected field: %u.\r\n", (ULONG)item.id);
				IFERR_RET(getValue(item, rules.getBack().fInherit));
				continue; // goto next item
			}
			case UpdateRulesField::RuleValue:
			{
				if (rules.isEmpty())
					return LOGERROR(STATUS_INVALID_PARAMETER, "Unexpected field: %u.\r\n", (ULONG)item.id);
				IFERR_RET(getValue(item, rules.getBack().m_fValue));
				continue; // goto next item
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////
	//
	// Logging
	//

	if (g_pCommonData->nLogLevel >= 2)
	{
		LOGINFO2("updateProcessRules. option: %s, mode: %u, persist: %u, tag: <%wZ>, rulesCount: %u.\r\n",
			getOptionStateName(eProcessOption), (ULONG)updateMode, (ULONG)fPersistent, 
			(PCUNICODE_STRING)usTagForDelete, (ULONG)rules.getSize() );
		printProcessRuleList(rules);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Verification
	//

	if (updateMode <= UpdateRulesMode::Invalid || updateMode >= UpdateRulesMode::_Max)
		return LOGERROR(STATUS_INVALID_PARAMETER, "The %u field is absent or invalid.\r\n", (ULONG)UpdateRulesField::Mode);

	if (updateMode == UpdateRulesMode::DeleteByTag)
	{
		if (usTagForDelete.isEmpty())
			return LOGERROR(STATUS_INVALID_PARAMETER, "The %u field is absent or invalid.\r\n", (ULONG)UpdateRulesField::Tag);
	}

	if (updateMode == UpdateRulesMode::DeleteByTag || updateMode == UpdateRulesMode::Clear)
	{
		if (!rules.isEmpty())
			// This error does not break the function, but should be logged
			LOGERROR(STATUS_INVALID_PARAMETER, "The 'rules' should not be set for mode: %u.\r\n", (ULONG)updateMode);
	}

	if (updateMode != UpdateRulesMode::DeleteByTag)
	{
		if (eProcessOption == ProcessOption::Invalid)
			return LOGERROR(STATUS_INVALID_PARAMETER, "The %u field is absent or invalid.\r\n", (ULONG)UpdateRulesField::Type);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Update rules
	//

	// Delete by tag
	bool fNeedSave = false;
	if (updateMode == UpdateRulesMode::DeleteByTag)
	{
		UniqueLock rulesLock(g_pCommonData->mtxProcessRules);
		auto nDeletedCount = deleteRulesByTag(g_pCommonData->processRules, usTagForDelete);
		fNeedSave = nDeletedCount != 0;
		eProcessOption = ProcessOption::Invalid; // Save All

		LOGINFO2("updateProcessRules. %u rules were deleted.\r\n", (ULONG)nDeletedCount);
	}
	// Update
	else
	{
		UniqueLock rulesLock(g_pCommonData->mtxProcessRules);
		fNeedSave = fPersistent;

		// Select rules list for update
		ProcessOptionRuleList* pRuleList = nullptr;
		{
			ProcessOptionRules* pProcessRules = getRules(eProcessOption);
			// It's impossible, but...
			if (pProcessRules == nullptr)
				return LOGERROR(STATUS_INVALID_PARAMETER);
			pRuleList = fPersistent ? &pProcessRules->persistRules : &pProcessRules->tempRules;
		}

		// update rules list
		updateList(updateMode, *pRuleList, rules);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Logging new state
	//

	if (g_pCommonData->nLogLevel >= 2)
	{
		printCurProcessRules();
	}

	// Save into config
	if (!fNeedSave)
		return STATUS_SUCCESS;
	if (eProcessOption != ProcessOption::Invalid)
	{
		IFERR_LOG(saveRulesToConfig(eProcessOption));
	}
	else
	{
		IFERR_LOG(saveRulesToConfig(ProcessOption::EnableInject));
		IFERR_LOG(saveRulesToConfig(ProcessOption::Protected));
		IFERR_LOG(saveRulesToConfig(ProcessOption::Trusted));
		IFERR_LOG(saveRulesToConfig(ProcessOption::SendEvent));
	}

	return STATUS_SUCCESS;
}

NTSTATUS setProcessInfo(variant::BasicLbvsDeserializer<SetProcessInfoField>& deserializer)
{
	// Deserialize
	uint32_t nPid = 0;
	OptionState fSendEvents;
	OptionState fEnableInject;
	OptionState fIsProtected;
	OptionState fIsTrusted;
	for (auto& item : deserializer)
	{
		switch (item.id)
		{
			case SetProcessInfoField::Pid:
			{
				IFERR_RET(getValue(item, nPid));
				continue; // goto next item
			}
			case SetProcessInfoField::Trusted:
			{
				BOOLEAN fVal;
				IFERR_RET(getValue(item, fVal));
				fIsTrusted = OptionState(fVal, OptionState::Reason::Forced, false);
				continue; // goto next item
			}
			case SetProcessInfoField::Protected:
			{
				BOOLEAN fVal;
				IFERR_RET(getValue(item, fVal));
				fIsProtected = OptionState(fVal, OptionState::Reason::Forced, false);
				continue; // goto next item
			}
			case SetProcessInfoField::SendEvent:
			{
				BOOLEAN fVal;
				IFERR_RET(getValue(item, fVal));
				fSendEvents = OptionState(fVal, OptionState::Reason::Forced, false);
				continue; // goto next item
			}
			case SetProcessInfoField::EnableInject:
			{
				BOOLEAN fVal;
				IFERR_RET(getValue(item, fVal));
				fEnableInject = OptionState(fVal, OptionState::Reason::Forced, false);
				continue; // goto next item
			}
		};
	}

	// Logging
	LOGINFO2("setProcessInfo: pid: %Iu, trust: %s, prot: %s, noInj: %s, sendEvt: %s.\r\n",
		(ULONG_PTR)nPid, fIsTrusted.printState(), fIsProtected.printState(),
		fEnableInject.printState(), fSendEvents.printState());

	// update process info
	if (nPid == 0)
		return LOGERROR(STATUS_INVALID_PARAMETER, "Pid is not specified.\r\n");

	ContextPtr pProcessCtx;
	IFERR_RET(fillContext((HANDLE)(ULONG_PTR)nPid, nullptr, pProcessCtx));

	if (fEnableInject.isInitialized())
		pProcessCtx->fEnableInject = fEnableInject;
	if (fSendEvents.isInitialized())
		pProcessCtx->fSendEvents = fSendEvents;
	if (fIsTrusted.isInitialized())
		pProcessCtx->fIsTrusted = fIsTrusted;
	if (fIsProtected.isInitialized())
		pProcessCtx->fIsProtected = fIsProtected;

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Send events
//
//////////////////////////////////////////////////////////////////////////

//
// Send event SysmonEvent::ProcessCreate
//
NTSTATUS sendProcessCreate(PEPROCESS pProcess, HANDLE nProcessId, 
	PPS_CREATE_NOTIFY_INFO pCreateInfo, procmon::Context* pProcessCtx)
{
	constexpr SysmonEvent eEvent = SysmonEvent::ProcessCreate;

	if (!isEventEnabled(eEvent))
		return STATUS_SUCCESS;

	if (pProcessCtx == nullptr || pProcessCtx->processInfo.pusImageName == nullptr)
	{
		LOGINFO2("sendEvent: %u (ProcessCreate), pid: %Iu, parent: %Iu.\r\n", (ULONG)eEvent,
			(ULONG_PTR)nProcessId, (ULONG_PTR)pCreateInfo->ParentProcessId);
	}
	else
	{
		LOGINFO2("sendEvent: %u (ProcessCreate), pid: %Iu, parent: %Iu, imagePath: <%wZ>.\r\n", (ULONG)eEvent,
			(ULONG_PTR)nProcessId, (ULONG_PTR)pCreateInfo->ParentProcessId, pProcessCtx->processInfo.pusImageName);
	}

	KERNEL_USER_TIMES Times = {};
	IFERR_LOG(getProcessTimes(pProcess, &Times));

	PagedLbvsSerializer<EventField> serializer;
	if (!serializer.write(EvFld::RawEventId, uint16_t(eEvent))) return STATUS_NO_MEMORY;
	//if (!serializer.write(EvFld::Time, getSystemTime())) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::TickTime, getTickCount64())) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessPid, (uint32_t)(ULONG_PTR)nProcessId)) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessCreationTime, (Times.CreateTime.QuadPart - 116444736000000000LL) / 10000)) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessParentPid, (uint64_t)(ULONG_PTR)pCreateInfo->ParentProcessId)) return STATUS_NO_MEMORY;
	if(pCreateInfo->CreatingThreadId.UniqueProcess != pCreateInfo->ParentProcessId)
		if (!serializer.write(EvFld::ProcessCreatorPid, (uint32_t)(ULONG_PTR)pCreateInfo->CreatingThreadId.UniqueProcess)) return STATUS_NO_MEMORY;
	if (pCreateInfo->CommandLine != nullptr)
		if (!write(serializer, EvFld::ProcessCmdLine, pCreateInfo->CommandLine)) return STATUS_NO_MEMORY;
	if (pCreateInfo->FileOpenNameAvailable && pCreateInfo->ImageFileName != nullptr)
		if (!write(serializer, EvFld::ProcessImageFile, pCreateInfo->ImageFileName)) return STATUS_NO_MEMORY;

	HANDLE hToken = nullptr;
	IFERR_LOG(getProcessToken(pProcess, &hToken), "Can't get process token, pid: %Iu", (ULONG_PTR)nProcessId);
	if (hToken != nullptr)
	{
		__try
		{
			// Get user name
			DECLARE_UNICODE_STRING_SIZE(usSID, c_nMaxStringSidSizeCh);
			NTSTATUS ns = getTokenSid(hToken, &usSID);
			IFERR_LOG(ns, "Can't get process user SID, pid: %Iu", (ULONG_PTR)nProcessId);
			if (NT_SUCCESS(ns))
				if (!write(serializer, EvFld::ProcessUserSid, &usSID)) return STATUS_NO_MEMORY;

			// Get TOKEN_ELEVATION
			bool fElevated = false;
			ns = checkTokenElevation(hToken, &fElevated);
			IFERR_LOG(ns, "Can't checkTokenElevation, pid: %Iu", (ULONG_PTR)nProcessId);
			if (NT_SUCCESS(ns))
				if (!serializer.write(EvFld::ProcessIsElevated, fElevated)) return STATUS_NO_MEMORY;

			// Get TOKEN_ELEVATION_TYPE
			TOKEN_ELEVATION_TYPE eElevationType = TokenElevationTypeDefault;
			ns = getTokenElevationType(hToken, &eElevationType);
			IFERR_LOG(ns, "Can't getTokenElevationType, pid: %Iu", (ULONG_PTR)nProcessId);
			if (NT_SUCCESS(ns))
				if (!serializer.write(EvFld::ProcessElevationType, (uint32_t)eElevationType)) return STATUS_NO_MEMORY;
		}
		__finally
		{
			if (hToken != nullptr)
				ZwClose(hToken);
		}
	}

	return fltport::sendRawEvent(serializer);
}

//
// Send event SysmonEvent::ProcessDelete
//
NTSTATUS sendProcessDelete(PEPROCESS pProcess, HANDLE nProcessId)
{
	constexpr SysmonEvent eEvent = SysmonEvent::ProcessDelete;

	if (!isEventEnabled(eEvent))
		return STATUS_SUCCESS;

	LOGINFO2("sendEvent: %u (ProcessDelete), pid: %Iu.\r\n", (ULONG)eEvent, (ULONG_PTR)nProcessId);

	KERNEL_USER_TIMES Times = {};
	IFERR_LOG(getProcessTimes(pProcess, &Times));

	PagedLbvsSerializer<EventField> serializer;
	if (!serializer.write(EvFld::RawEventId, uint16_t(eEvent))) return STATUS_NO_MEMORY;
	//if (!serializer.write(EvFld::Time, getSystemTime())) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::TickTime, getTickCount64())) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessDeletionTime, (Times.ExitTime.QuadPart - 116444736000000000LL) / 10000)) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessPid, (uint32_t)(ULONG_PTR)nProcessId)) return STATUS_NO_MEMORY;
	if (!serializer.write(EvFld::ProcessExitCode, (uint32_t)PsGetProcessExitStatus(pProcess))) return STATUS_NO_MEMORY;

	return fltport::sendRawEvent(serializer);
}


//////////////////////////////////////////////////////////////////////////
//
// Callbacks
//
//////////////////////////////////////////////////////////////////////////

//
// Create/Terminate process callback
//
VOID notifyOnCreateProcess(_Inout_ PEPROCESS pProcess, 
	_In_ HANDLE nProcessId, _In_opt_ PPS_CREATE_NOTIFY_INFO pCreateInfo)
{
	// Process start
	if (pCreateInfo != NULL)
    {
		procmon::Context* pNewProcessCtx = nullptr;
		procmon::Context* pParentProcessCtx = nullptr;
		__try
		{
			IFERR_LOG(procmon::fillContext(nProcessId, nullptr, &pNewProcessCtx));
			IFERR_LOG(procmon::fillContext(pCreateInfo->CreatingThreadId.UniqueProcess, 
				nullptr, &pParentProcessCtx));

			// logging
			PRESET_UNICODE_STRING(usImagePath, L"UNKNOWN");
			PCUNICODE_STRING pusImageName = (pNewProcessCtx != nullptr && pNewProcessCtx->_fIsFilled) ?
				pNewProcessCtx->processInfo.pusImageName : &usImagePath;
			LOGINFO3("Process %Id is created, creator: %Id, image: <%wZ>.\r\n", (ULONG_PTR)nProcessId,
				(ULONG_PTR)pCreateInfo->CreatingThreadId.UniqueProcess, pusImageName);

			// Call callbacks
			if (pNewProcessCtx != nullptr && pParentProcessCtx != nullptr)
			{
				// objmon
				objmon::detail::notifyOnProcessCreation(pNewProcessCtx, pParentProcessCtx);
				// dllinj
				dllinj::detail::notifyOnProcessCreation(pNewProcessCtx, pParentProcessCtx);
			}

			if (g_pCommonData->fEnableMonitoring && !isProcessInWhiteList(pNewProcessCtx))
				IFERR_LOG(sendProcessCreate(pProcess, nProcessId, pCreateInfo, pNewProcessCtx), 
					"Can't send event %u (ProcessCreate). pid: %Id.\r\n", 
					(ULONG)SysmonEvent::ProcessCreate, (ULONG_PTR)nProcessId);
		}
		__finally
		{
			if (pNewProcessCtx != nullptr)
				procmon::releaseContext(pNewProcessCtx);
			if (pParentProcessCtx != nullptr)
				procmon::releaseContext(pParentProcessCtx);
		}
	}

	// Process finish
	else
    {
		Context* pCtx = nullptr;
		__try
		{
			IFERR_LOG(procmon::fillContext(nProcessId, nullptr, &pCtx));

			// Set termination flag
			if (pCtx != nullptr)
			{
				pCtx->processInfo.fIsTerminated = true;
			}

			// Call callbacks
			if (pCtx != nullptr)
			{
				// objmon
				objmon::detail::notifyOnProcessTermination(pCtx);
				// dllinj
				dllinj::detail::notifyOnProcessTermination(pCtx);
			}

			LOGINFO3("Process %Id destroyed\r\n", (ULONG_PTR)nProcessId);

			if (g_pCommonData->fEnableMonitoring && !isProcessInWhiteList(pCtx) )
				IFERR_LOG(sendProcessDelete(pProcess, nProcessId), 
					"Can't send event %u (ProcessDelete). pid: %Id.\r\n",
					(ULONG)SysmonEvent::ProcessDelete, (ULONG_PTR)nProcessId);

			// Free context
			{
				UniqueLock lock(g_pCommonData->mtxProcessContextList);
				removeContextFromList(nProcessId);
			}
		}
		__finally
		{
			if (pCtx != nullptr)
				procmon::releaseContext(pCtx);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Filter registration / deregistration
//
//////////////////////////////////////////////////////////////////////////

//
//
//
NTSTATUS initialize()
{
	LOGINFO2("Enable Process context.\r\n");

	if (g_pCommonData->fProcCtxNotificationIsStarted) 
		return LOGERROR(STATUS_ALREADY_REGISTERED);

	IFERR_RET(PsSetCreateProcessNotifyRoutineEx(&notifyOnCreateProcess, FALSE));

	g_pCommonData->fProcCtxNotificationIsStarted = TRUE;

	// load configs
	IFERR_LOG(loadRulesFromConfig(ProcessOption::Trusted));
	IFERR_LOG(loadRulesFromConfig(ProcessOption::Protected));
	IFERR_LOG(loadRulesFromConfig(ProcessOption::EnableInject));
	IFERR_LOG(loadRulesFromConfig(ProcessOption::SendEvent));

	// Mark spec process
	{
		HANDLE* pProcessArray = nullptr;
		size_t nProcessCount = 0;
		__try
		{
			IFERR_LOG(getProcessList(&pProcessArray, &nProcessCount));

			// Init Interesting processes
			for (size_t i = 0; i < nProcessCount; i++)
			{
				if ((UINT_PTR)pProcessArray[i] == 0) continue;

				procmon::Context* pCtx = nullptr;
				IFERR_LOG(procmon::fillContext(pProcessArray[i], nullptr, &pCtx));
				if (pCtx != nullptr)
					procmon::releaseContext(pCtx);
			}
		}
		__finally
		{
			if (pProcessArray != nullptr)
				ExFreePoolWithTag(pProcessArray, c_nAllocTag);
		}
	}

	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (!g_pCommonData->fProcCtxNotificationIsStarted) return;

	g_pCommonData->fProcCtxNotificationIsStarted = FALSE;
	IFERR_LOG(PsSetCreateProcessNotifyRoutineEx(&notifyOnCreateProcess, TRUE));

	{
		UniqueLock lock(g_pCommonData->mtxProcessContextList);
		freeContextList();
	}
}

} // namespace procmon
} // namespace openEdr


//
// netfilter driver extension
//

///
/// Checks that need to collect events for the process.
///
/// @param nProcessId - process pid.
/// @return The function returns `true` if the process is in the white list.
///
EXTERN_C BOOLEAN cmdedr_isProcessInWhiteList(ULONG_PTR nProcessId)
{
	if (nProcessId < 4)
		return true;
	return openEdr::procmon::isProcessInWhiteList(nProcessId);
}

/// @}
