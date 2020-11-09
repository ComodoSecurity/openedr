//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Registry filter
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "osutils.h"
#include "fltport.h"
#include "procmon.h"
#include "regmon.h"
#include "config.h"

namespace openEdr {
namespace regmon {

//
// Default regmon serializer type
//
typedef PagedLbvsSerializer<edrdrv::EventField> DefaultSerializer;

//////////////////////////////////////////////////////////////////////////
//
// Registry utils
//
//////////////////////////////////////////////////////////////////////////

PRESET_STATIC_UNICODE_STRING(c_usInvalidKeyPath, L"INVALID_KEY_PATH");
PRESET_STATIC_UNICODE_STRING(c_usDeletedKeyPath, L"DELETED_KEY_PATH");
PRESET_STATIC_UNICODE_STRING(c_usDisableSelfDefenceValueName, L"" CMD_EDRDRV_DISABLE_SP_VALUE_NAME);

///
/// The structure of a registry key name.
///
struct RegKeyName
{
	// If true, fnCmCallbackGetKeyObjectIDEx is used
	// If false, ObQueryNameString is used
	bool m_fWin8ApiIsUsed = true;

	PCUNICODE_STRING m_pusKeyName = nullptr;

	DynUnicodeString m_usWin7Buffer;

public:
	RegKeyName() = default;
	RegKeyName(const RegKeyName&) = delete;

	~RegKeyName()
	{
		if (m_pusKeyName != nullptr && m_fWin8ApiIsUsed)
			g_pCommonData->fnCmCallbackReleaseKeyObjectIDEx(m_pusKeyName);
	}

	//
	//
	//
	bool isInitialized() const
	{
		return m_pusKeyName != nullptr;
	}

	//
	//
	//
	NTSTATUS init(PVOID pObject)
	{
		if (m_pusKeyName != nullptr)
			return STATUS_ALREADY_INITIALIZED;
		if (pObject == nullptr)
			return STATUS_INVALID_PARAMETER;

		m_fWin8ApiIsUsed = g_pCommonData->fnCmCallbackGetKeyObjectIDEx != nullptr &&
			g_pCommonData->fnCmCallbackReleaseKeyObjectIDEx != nullptr;

		// Win8 API mode
		if (m_fWin8ApiIsUsed)
		{
			PCUNICODE_STRING pName = nullptr;
			IFERR_RET_NOLOG(g_pCommonData->fnCmCallbackGetKeyObjectIDEx(&g_pCommonData->nRegFltCookie,
				pObject, nullptr, &pName, 0));
			if (pName == nullptr)
				return LOGERROR(STATUS_UNSUCCESSFUL, "CmCallbackGetKeyObjectIDEx() failed");
			m_pusKeyName = pName;
		}

		// Win7 API mode
		else
		{
			IFERR_RET_NOLOG(ObQueryNameString(pObject, m_usWin7Buffer));
			m_pusKeyName = m_usWin7Buffer;
		}

		return STATUS_SUCCESS;
	}

	///
	/// Returns printable key path.
	///
	/// @param pObject - a raw pointer to the object.
	/// @return Always return value Pointer to UNICODE_STRING.
	/// The returned pointer is valid until destruction of RegKeyName
	///
	PCUNICODE_STRING initPrintable(PVOID pObject)
	{
		NTSTATUS eResult = init(pObject);
		if (NT_SUCCESS(eResult))
			return m_pusKeyName;
		if (eResult == STATUS_KEY_DELETED)
			return &c_usDeletedKeyPath;
		return &c_usInvalidKeyPath;
	}

	//
	//
	//
	PCUNICODE_STRING getValue()
	{
		return m_pusKeyName;
	}
};

///
/// Captures user mode buffer into kernel memory.
///
/// @param ppDst - TBD.
/// @param pSrc - TBD.
/// @param nSrcSize - TBD.
/// @return The function returns error if exception is occurred during 
/// access to user mode buffer.
///
/// Buffer should be freed with ExFreePoolWithTag
///
/// Some data in registry callback can be not captured. See MSDN
///
NTSTATUS captureBuffer(PVOID* ppDst, const void* pSrc, size_t nSrcSize)
{
	if (ppDst == nullptr)
		return STATUS_INVALID_PARAMETER_1;

    if (nSrcSize == 0) 
	{
        *ppDst = nullptr;
        return STATUS_SUCCESS;
    }

	if (pSrc == nullptr)
		return STATUS_INVALID_PARAMETER_2;

	PVOID pTempBuffer = ExAllocatePoolWithTag(PagedPool, nSrcSize, c_nAllocTag);
	if (pTempBuffer == nullptr)
		return STATUS_NO_MEMORY;

	NTSTATUS eResut = STATUS_SUCCESS;
    __try
	{
        RtlCopyMemory(pTempBuffer, pSrc, nSrcSize);
    } 
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
        ExFreePoolWithTag(pTempBuffer, c_nAllocTag);
        pTempBuffer = nullptr;
		eResut = GetExceptionCode();
		if (NT_SUCCESS(eResut))
			eResut = STATUS_UNSUCCESSFUL;
	}
	IFERR_RET(eResut);

    *ppDst = pTempBuffer;
    return eResut;
}


///
/// Captures UNICODE_STRING into kernel memory.
///
/// @param ppusDst - TBD.
/// @param pusSrc - TBD.
/// @return The function returns a NTSTATUS of the operation.
///
/// Buffer should be free with ExFreePoolWithTag
///
NTSTATUS captureUnicodeString(PCUNICODE_STRING* ppusDst, PCUNICODE_STRING pusSrc)
{
	//
	// Additional sizeof(WCHAR) bytes are added to the buffer size since
	// SourceString->Length does not include the NULL at the end of the string.
	//

	size_t nBufferSize = sizeof(UNICODE_STRING) /*UNICODE_STRING data*/ +
		(size_t)pusSrc->Length /*string data*/ +
		sizeof(WCHAR) /*zero end*/;

	PUINT8 pBuffer = (PUINT8)ExAllocatePoolWithTag(PagedPool, nBufferSize, c_nAllocTag);
	if (pBuffer == nullptr) return STATUS_INSUFFICIENT_RESOURCES;

	// Fill UNICODE_STRING
	PUNICODE_STRING pusDst = (PUNICODE_STRING)pBuffer;
	PUINT8 pStrPlace = pBuffer + sizeof(UNICODE_STRING);
	pusDst->Length = pusSrc->Length;
	pusDst->MaximumLength = pusSrc->Length + sizeof(WCHAR);
	pusDst->Buffer = (PWCH)pStrPlace;
	*((PWCHAR)(pBuffer + nBufferSize - sizeof(WCHAR))) = 0;

	if (pusSrc->Length == 0)
	{
		*ppusDst = pusDst;
		return STATUS_SUCCESS;
	}

	//
	// Only SourceString->Length should be checked. The registry does not
	// validate SourceString->MaximumLength.
	//

	NTSTATUS eResut = STATUS_SUCCESS;
	__try
	{
		RtlCopyMemory(pusDst->Buffer, pusSrc->Buffer, pusSrc->Length);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ExFreePoolWithTag(pusDst, c_nAllocTag);
		pusDst = nullptr;
		eResut = GetExceptionCode();
		if (NT_SUCCESS(eResut))
			eResut = STATUS_UNSUCCESSFUL;
	}

	*ppusDst = pusDst;
	return eResut;
}

//
// Checks that a specified registry value type is enabled to send
//
inline bool isValueTypeEnabled(DWORD eType)
{
	if (eType >= sizeof(uint32_t) * 8)
		return false;
	uint32_t nFlag = 1u << (uint32_t)eType;
	return (g_pCommonData->nSentRegTypesFlags & nFlag) != 0;
}

//
// Normalize reg path
//

UNICODE_STRING g_usWow64Keys[] = { U_STAT(L"\\wow6432node"), U_STAT(L"\\wowaa32node") };
PRESET_STATIC_UNICODE_STRING(g_usHklmPrefix, L"\\registry\\machine\\");
PRESET_STATIC_UNICODE_STRING(g_usHkuPrefix, L"\\registry\\user\\");
PRESET_STATIC_UNICODE_STRING(g_usHklm, L"%hklm%\\");
PRESET_STATIC_UNICODE_STRING(g_usHkcu, L"%hkcu%");
PRESET_STATIC_UNICODE_STRING(g_usHkcuClasses, L"%hkcu%\\software\\classes");
PRESET_STATIC_UNICODE_STRING(g_usControlSetPrefix, L"%hklm%\\system\\controlset");
PRESET_STATIC_UNICODE_STRING(g_usCurrentControlSet, L"%hklm%\\system\\currentcontrolset");
PRESET_STATIC_UNICODE_STRING(g_usClassesPostfix, L"_classes");

///
/// Normalize registry path.
///
/// @param usOrigName - TBD.
/// @param usNormName - TBD.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS normalizeRegKeyName(PCUNICODE_STRING usOrigName, DynUnicodeString& usNormName)
{
	// Copy and convert to lowwer case
	DynUnicodeString usResult;
	IFERR_RET(convertToLowerCase(usOrigName, usResult));

	// Remove wow64 keys
	for (auto& usWow64Key : g_usWow64Keys)
	{
		auto nPos = findSubStrCb(usResult, &usWow64Key);
		if(nPos == SIZE_MAX)
			continue;
		// Check following '\\'
		size_t nEndPos = nPos + usWow64Key.Length;
		if (usResult.getLengthCb() >= nEndPos + sizeof(wchar_t)
			&& usResult.getCharCb(nEndPos) != L'\\')
			continue;

		// cut wow64 key
		cutStrCb((PUNICODE_STRING)usResult, nPos, usWow64Key.Length);
	}

	// Process HKLM
	if (isStartWith(usResult, &g_usHklmPrefix))
	{
		if (!replaceSubStrCb((PUNICODE_STRING)usResult, &g_usHklm, 0, g_usHklmPrefix.Length))
			return LOGERROR(STATUS_UNSUCCESSFUL);

		// Process CurrentControlSet
		size_t nFullControlSetSizeCb = g_usControlSetPrefix.Length + 3 /* 001 */ * sizeof(wchar_t);
		if (usResult.getLengthCb() >= nFullControlSetSizeCb + sizeof(wchar_t) /* \\ */ &&
			isStartWith(usResult, &g_usControlSetPrefix) && 
			usResult.getCharCb(nFullControlSetSizeCb) == L'\\')
		{
			if (!replaceSubStrCb((PUNICODE_STRING)usResult, &g_usCurrentControlSet, 0, nFullControlSetSizeCb))
				return LOGERROR(STATUS_UNSUCCESSFUL);
		}
	}
	// Process HKU
	else if(isStartWith(usResult, &g_usHkuPrefix))
	{
		// Process Classes
		auto nNextSlashPosCb = findChrCb(usResult, L'\\', g_usHkuPrefix.Length);
		if (nNextSlashPosCb != SIZE_MAX)
		{
			auto usBeforeSlash = sliceStrCb(usResult, 0, nNextSlashPosCb);

			if (isEndedWith(&usBeforeSlash, &g_usClassesPostfix))
			{
				if (!replaceSubStrCb((PUNICODE_STRING)usResult, &g_usHkcuClasses, 0, nNextSlashPosCb))
					return LOGERROR(STATUS_UNSUCCESSFUL);
			}
			else
			{
				if (!replaceSubStrCb((PUNICODE_STRING)usResult, &g_usHkcu, 0, nNextSlashPosCb))
					return LOGERROR(STATUS_UNSUCCESSFUL);
			}
		}
	}
	
	usNormName = std::move(usResult);
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Self defense
//
//////////////////////////////////////////////////////////////////////////

struct AllowedAccessInfo
{
	AccessType eAccessType;
	bool fRecurcive;
};

constexpr AllowedAccessInfo c_DefaultAllowedAccessInfo = { AccessType::Full, false };

namespace detail
{

//
// Rules checking for full path
//
AllowedAccessInfo applyRegistryRules(PCUNICODE_STRING pusFullKeyName)
{
	// normalize reg key name
	DynUnicodeString usNormPath;
	IFERR_LOG(normalizeRegKeyName(pusFullKeyName, usNormPath));
	if (usNormPath.isEmpty())
		return c_DefaultAllowedAccessInfo;

	// apply rules
	{
		SharedLock lock(g_pCommonData->mtxRegRules);
		for (auto& rule : g_pCommonData->regRules)
		{
			if (!isStartWith(usNormPath, rule.usPath))
				continue;

			// check applicability for nested keys
			if (!rule.fRecursive && usNormPath.getLengthCb() != rule.usPath.getLengthCb())
				continue;

			// Check following L'\\'
			if (usNormPath.getLengthCb() - rule.usPath.getLengthCb() > sizeof(wchar_t) &&
				usNormPath.getCharCb(rule.usPath.getLengthCb()) != L'\\')
				continue;

			// apply rule
			return { rule.eAccessType, (bool)rule.fRecursive };
		}
	}

	return c_DefaultAllowedAccessInfo;
}

//
//
//
NTSTATUS mergeKeyName(DynUnicodeString& usResult, PCUNICODE_STRING pusRootName, PCUNICODE_STRING pusCompleteName)
{
	PRESET_UNICODE_STRING(usSlash, L"\\");

	IFERR_RET(usResult.alloc(((size_t)pusRootName->Length) + 
		((size_t)usSlash.Length) + ((size_t)pusCompleteName->Length)));
	IFERR_RET(RtlUnicodeStringCopy((PUNICODE_STRING)usResult, pusRootName));
	IFERR_RET(RtlUnicodeStringCat((PUNICODE_STRING)usResult, &usSlash));
	IFERR_RET(RtlUnicodeStringCat((PUNICODE_STRING)usResult, pusCompleteName));

	return STATUS_SUCCESS;
}

//
// Create full path and call rules checking
//
AllowedAccessInfo getAllowedAccessInfo(PCUNICODE_STRING pusRootName, PCUNICODE_STRING pusCompleteName)
{
	// if pusCompleteName is absent
	if (pusCompleteName == nullptr || pusCompleteName->Length < sizeof(L'\\'))
	{
		if (pusRootName == nullptr)
			return c_DefaultAllowedAccessInfo;
		return applyRegistryRules(pusRootName);
	}

	// if pusCompleteName contains fullpath
	if (pusCompleteName->Buffer[0] == L'\\')
	{
		return applyRegistryRules(pusCompleteName);
	}

	// if pusCompleteName contains relative path
	if (pusRootName == nullptr)
		return c_DefaultAllowedAccessInfo;
	DynUnicodeString usFullPath;
	NTSTATUS ns = mergeKeyName(usFullPath, pusRootName, pusCompleteName);
	if (!NT_SUCCESS(ns))
	{
		LOGERROR(ns);
		return c_DefaultAllowedAccessInfo;
	}
	return applyRegistryRules(usFullPath);
}

//
// Create full path and call rules checking
//
AllowedAccessInfo getAllowedAccessInfo(PVOID pRootRegistryObject, PCUNICODE_STRING pusCompleteName)
{
	// Resolve root object name
	PCUNICODE_STRING pusRootName = nullptr;
	RegKeyName keyName;
	if (pRootRegistryObject != nullptr)
	{
		IFERR_LOG(keyName.init(pRootRegistryObject));
		if (keyName.isInitialized())
			pusRootName = keyName.getValue();
	}

	return getAllowedAccessInfo(pusRootName, pusCompleteName);
}

//
//
//
bool checkFullAccessIsApplied()
{
	// Check rules existence
	{
		SharedLock lock(g_pCommonData->mtxRegRules);
		if (g_pCommonData->regRules.isEmpty())
			return true;
	}

	// allow all for trusted
	if (procmon::isCurProcessTrusted())
		return true;

	return false;
}

} // namespace detail


//
// Check self defense for current process and key.
// Returns allowed access type.
//
AllowedAccessInfo getAllowedAccessInfo(PVOID pRootRegistryObject, PCUNICODE_STRING pusCompleteName)
{
	if (detail::checkFullAccessIsApplied())
		return c_DefaultAllowedAccessInfo;

	return detail::getAllowedAccessInfo(pRootRegistryObject, pusCompleteName);
}

//
// Check self defense for current process and key.
// Returns allowed access type.
//
AllowedAccessInfo getAllowedAccessInfo(PCUNICODE_STRING pusRootName, PCUNICODE_STRING pusCompleteName)
{
	if (detail::checkFullAccessIsApplied())
		return c_DefaultAllowedAccessInfo;

	return detail::getAllowedAccessInfo(pusRootName, pusCompleteName);
}

//
// Simple self defense check
// For actions which can't change access.
//
bool isFullAccessAllowed(PVOID pRegistryObject, REG_NOTIFY_CLASS eNotifyClass)
{
	AllowedAccessInfo allowedAccess = getAllowedAccessInfo(pRegistryObject, nullptr);
	if (allowedAccess.eAccessType == AccessType::Full)
		return true;

	// deny access
	LOGINFO1("Selfdefense: Deny access: pid: %Iu, action: %u, regkey: <%wZ>.\r\n",
		(ULONG_PTR)PsGetCurrentProcessId(), (ULONG)eNotifyClass, RegKeyName().initPrintable(pRegistryObject));
	return false;
}

///
/// Convert ACCESS_MASK mask to AccessType
///
inline bool isAccessAllowed(ACCESS_MASK accessMask, AllowedAccessInfo accessInfo)
{
	static constexpr ACCESS_MASK c_eNonRecurciveWriteAccessMask = KEY_SET_VALUE | KEY_CREATE_LINK | 
		DELETE | WRITE_DAC | WRITE_OWNER | GENERIC_WRITE | GENERIC_ALL;
	static constexpr ACCESS_MASK c_eRecurciveWriteAccessMask = c_eNonRecurciveWriteAccessMask | KEY_CREATE_SUB_KEY;

	if (accessMask == 0)
		return true;

	switch (accessInfo.eAccessType)
	{
		case AccessType::Full:
			return true;
		case AccessType::ReadOnly:
		{
			auto eWriteAccessMask = (accessInfo.fRecurcive) ? c_eRecurciveWriteAccessMask : c_eNonRecurciveWriteAccessMask;
			return (accessMask & eWriteAccessMask) == 0;
		}
		case AccessType::NoAccess:
			return false;
		default:
			return true;
	}
}



//
// Print rules
//
void printRegRuleList(const RegRuleList& rules)
{
	ULONG nIndex = 0;
	for (auto& rule : rules)
	{
		LOGINFO1("rule#%02u: path: <%wZ>, val: %u, dir: %u, tag: <%wZ>.\r\n",
			nIndex, (PCUNICODE_STRING)rule.usPath, (ULONG)rule.eAccessType, (ULONG)rule.fRecursive,
			(PCUNICODE_STRING)rule.usTag);
		++nIndex;
	}
}

//
//
//
NTSTATUS loadRulesFromConfig()
{
	Blob data;
	IFERR_RET(cfg::loadConfigBlob(cfg::ConfigId::ProtectedRegKeys, data));
	if (data.isEmpty())
		return STATUS_SUCCESS;

	IFERR_LOG(updateRegRules(data.getData(), data.getSize()));
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS saveRulesToConfig()
{
	// Serialization
	bool fRemoveConfig = false;
	NonPagedLbvsSerializer<UpdateRulesField> serializer;
	do
	{
		SharedLock rulesLock(g_pCommonData->mtxRegRules);

		// If list is empty - delete config
		if (g_pCommonData->regRules.isEmpty())
		{
			fRemoveConfig = true;
			break;
		}

		if (!serializer.write(UpdateRulesField::Mode, (uint32_t)UpdateRulesMode::Replace)) return LOGERROR(STATUS_NO_MEMORY);

		for (auto& rule : g_pCommonData->regRules)
		{
			if (!serializer.write(UpdateRulesField::Rule)) return LOGERROR(STATUS_NO_MEMORY);
			if (!serializer.write(UpdateRulesField::RuleValue, (uint32_t)rule.eAccessType)) return LOGERROR(STATUS_NO_MEMORY);
			if (!rule.usPath.isEmpty())
				if (!write(serializer, UpdateRulesField::RulePath, rule.usPath)) return LOGERROR(STATUS_NO_MEMORY);
			if (!rule.usTag.isEmpty())
				if (!write(serializer, UpdateRulesField::RuleTag, rule.usTag)) return LOGERROR(STATUS_NO_MEMORY);
			if (rule.fRecursive)
				if (!serializer.write(UpdateRulesField::RuleRecursive, true)) return LOGERROR(STATUS_NO_MEMORY);
		}
	} while (false);

	// Save
	if (fRemoveConfig)
	{
		IFERR_RET(cfg::removeConfigBlob(cfg::ConfigId::ProtectedRegKeys));
	}
	else
	{
		IFERR_RET(cfg::saveConfigBlob(cfg::ConfigId::ProtectedRegKeys, serializer));
	}
	return STATUS_SUCCESS;
}


//
//
//
NTSTATUS updateRegRules(const void* pData, size_t nDataSize)
{
	//////////////////////////////////////////////////////////////////////////
	//
	// Deserialize
	//

	variant::BasicLbvsDeserializer<UpdateRulesField> deserializer;
	if (!deserializer.reset(pData, nDataSize))
		return LOGERROR(STATUS_DATA_ERROR, "Invalid data format.\r\n");

	// Deserialize main data
	UpdateRulesMode updateMode = UpdateRulesMode::Invalid;
	DynUnicodeString usTagForDelete;
	for (auto& item : deserializer)
	{
		switch (item.id)
		{
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
	RegRuleList rules;
	for (auto& item : deserializer)
	{
		switch (item.id)
		{
			case UpdateRulesField::Rule:
			{
				IFERR_RET(rules.pushBack(RegRule()));
				// Set default tag
				if (!usTagForDelete.isEmpty())
					IFERR_RET(rules.getBack().usTag.assign(usTagForDelete), "Can't assign tag");
				continue; // goto next item
			}
			case UpdateRulesField::RulePath:
			{
				if (rules.isEmpty())
					return LOGERROR(STATUS_INVALID_PARAMETER, "Unexpected field: %u.\r\n", (ULONG)item.id);

				// to lower case
				DynUnicodeString usPath;
				IFERR_RET(getValue(item, usPath));
				IFERR_RET(convertToLowerCase(usPath, rules.getBack().usPath));
				
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
				IFERR_RET(getValue(item, rules.getBack().fRecursive));
				continue; // goto next item
			}
			case UpdateRulesField::RuleValue:
			{
				if (rules.isEmpty())
					return LOGERROR(STATUS_INVALID_PARAMETER, "Unexpected field: %u.\r\n", (ULONG)item.id);
				IFERR_RET(getEnumValue(item, rules.getBack().eAccessType));
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
		LOGINFO2("updateRegRules. mode: %u, tag: <%wZ>, rulesCount: %u.\r\n",
			(ULONG)updateMode, (PCUNICODE_STRING)usTagForDelete, (ULONG)rules.getSize());
		printRegRuleList(rules);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Verification
	//

	if (updateMode <= UpdateRulesMode::Invalid || updateMode >= UpdateRulesMode::_Max)
		return LOGERROR(STATUS_INVALID_PARAMETER, "The %u field is absent or invalid.\r\n", (ULONG)UpdateRulesField::Mode);

	if (updateMode == edrdrv::UpdateRulesMode::DeleteByTag)
	{
		if (usTagForDelete.isEmpty())
			return LOGERROR(STATUS_INVALID_PARAMETER, "The %u field is absent or invalid.\r\n", (ULONG)UpdateRulesField::Tag);
	}

	if (updateMode == edrdrv::UpdateRulesMode::DeleteByTag || updateMode == edrdrv::UpdateRulesMode::Clear)
	{
		if (!rules.isEmpty())
			// This error does not break the function, but should be logged
			LOGERROR(STATUS_INVALID_PARAMETER, "The 'rules' should not be set for mode: %u.\r\n", (ULONG)updateMode);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Update rules
	//

	// Delete by tag
	bool fNeedSave = false;
	if (updateMode == edrdrv::UpdateRulesMode::DeleteByTag)
	{
		UniqueLock rulesLock(g_pCommonData->mtxRegRules);

		PCUNICODE_STRING usTag = usTagForDelete;
		auto nDeletedCount = g_pCommonData->regRules.removeIf([usTag](const RegRule& rule) -> bool
		{
			return rule.usTag.isEqual(usTag, false);
		});
		fNeedSave = nDeletedCount != 0;
		LOGINFO2("updateRegRules. %u rules were deleted.\r\n", (ULONG)nDeletedCount);
	}
	// Update
	else
	{
		UniqueLock rulesLock(g_pCommonData->mtxRegRules);
		fNeedSave = true;
		updateList(updateMode, g_pCommonData->regRules, rules);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Logging new state
	//

	if (g_pCommonData->nLogLevel >= 2)
	{
		SharedLock rulesLock(g_pCommonData->mtxRegRules);
		LOGINFO1("Registry rules: count:%u.\r\n", (ULONG)g_pCommonData->regRules.getSize());
		printRegRuleList(g_pCommonData->regRules);
	}

	// Save into config
	if (fNeedSave)
	{
		IFERR_LOG(saveRulesToConfig());
	}

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Event Data
//
//////////////////////////////////////////////////////////////////////////

//
// Event Data for post callback
//
#pragma pack(push, 1)
struct EventData
{
	SysmonEvent eEvent; //< Event Type, just for logging
	size_t nDataSize;
	// UINT8 Data[]
};
#pragma pack(pop)

//
// Allocates new EventData from serializer
// If function is successes, it returns new EventData. 
// This EventData should be freed with freeEventData after usage.
//
NTSTATUS allocEventData(EventData** ppEventData, DefaultSerializer& serializer, 
	SysmonEvent eEvent)
{
	if (ppEventData == nullptr)
		return STATUS_INVALID_PARAMETER_1;

	void* pData = nullptr;
	size_t nDataSize = 0;
	if (!serializer.getData(&pData, &nDataSize)) return STATUS_UNSUCCESSFUL;

	auto pEventData = (EventData*)ExAllocatePoolWithTag(PagedPool, 
		sizeof(EventData) + nDataSize, c_nAllocTag);
	if (pEventData == nullptr) return STATUS_NO_MEMORY;

	memcpy(pEventData + 1, pData, nDataSize);
	pEventData->eEvent = eEvent;
	pEventData->nDataSize = nDataSize;
	*ppEventData = pEventData;
	return STATUS_SUCCESS;
}

//
// Send allocated EventData
//
NTSTATUS sendEventData(const EventData* pEventData)
{
	return fltport::sendRawEvent(pEventData + 1, pEventData->nDataSize);
}

///
/// Free Event data
///
void freeEventData(EventData* pEventData)
{
	if (pEventData != nullptr)
		ExFreePoolWithTag(pEventData, c_nAllocTag);
}

//////////////////////////////////////////////////////////////////////////
//
// Events
//
//////////////////////////////////////////////////////////////////////////

namespace detail {

//
// Fills common hashed fields of event.
// @param serializer
// @param eEventId
// @param pRegistryObject - registry object (from callback)
//
NTSTATUS createRegistryBaseEvent_Hashed(DefaultSerializer& serializer,
	SysmonEvent eEventId, PVOID pRegistryObject)
{
	// Get current path
	RegKeyName keyName;
	IFERR_RET(keyName.init(pRegistryObject));

	if (!serializer.write(EvFld::RawEventId, uint16_t(eEventId))) 
		return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RawEventId);
	if (!write(serializer, EvFld::RegistryPath, keyName.getValue()))
		return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RegistryPath);

	return STATUS_SUCCESS;
}

//
// Fills common non-hashed fields of event.
// @param serializer
// @param eEventId
// @param pRegistryObject - registry object (from callback)
//
NTSTATUS createRegistryBaseEvent_NonHashed(DefaultSerializer& serializer,
	SysmonEvent /*eEventId*/, PVOID /*pRegistryObject*/)
{
	//if (!serializer.write(EvFld::Time, getSystemTime()))
	//	return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::Time);
	if (!serializer.write(EvFld::TickTime, getTickCount64()))
		return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::TickTime);
	if (!serializer.write(EvFld::ProcessPid, (uint32_t)(ULONG_PTR)PsGetCurrentProcessId()))
		return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::ProcessPid);

	return STATUS_SUCCESS;
}



//
// Create and send event
// @param eEventId
// @param pRegistryObject - registry object (from callback)
// @param fnWriteAdditionalData - functor to write additional data
//
template<typename Fn>
NTSTATUS sendEventData(SysmonEvent eEvent, PVOID pRegistryObject, procmon::Context* pProcessCtx, Fn fnWriteAdditionalData)
{
	NTSTATUS eResult = STATUS_SUCCESS;
	DefaultSerializer serializer;
	
	// Prepare data for hash

	// access to user data can throw exceptions
	__try
	{
		IFERR_RET(createRegistryBaseEvent_Hashed(serializer, eEvent, pRegistryObject));
		#pragma warning(suppress : 4127)
		IFERR_RET(fnWriteAdditionalData(&serializer));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		eResult = GetExceptionCode();
		if (NT_SUCCESS(eResult)) eResult = STATUS_UNSUCCESSFUL;
	}
	IFERR_RET(eResult);

	// Repeat filtration
	do 
	{
		if (pProcessCtx == nullptr)
			break;

		// Calc hash
		uint64_t nHash;
		{
			void* pData = nullptr;
			size_t nDataSize = 0;
			if (!serializer.getData(&pData, &nDataSize))
				break;

			xxh::hash_state64_t hash;
			auto eErrorCode = hash.update(pData, nDataSize);
			if (eErrorCode != xxh::error_code::ok)
				break;
			nHash = hash.digest();
		}

		bool fSkipEvent = !pProcessCtx->pRegMonEventFilter->checkEventSend(nHash,
			(uint32_t)(uint64_t)g_pCommonData->nRegEventRepeatTimeout);

		if (fSkipEvent)
		{
			LOGINFO2("skipEvent: %u (registry), pid: %Iu, key: <%wZ>.\r\n", (ULONG)eEvent,
				(ULONG_PTR)PsGetCurrentProcessId(), RegKeyName().initPrintable(pRegistryObject));
			return STATUS_SUCCESS;
		}
	} while (false);


	// access to user data can throw exceptions
	__try
	{
		IFERR_RET(createRegistryBaseEvent_NonHashed(serializer, eEvent, pRegistryObject));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		eResult = GetExceptionCode();
		if (NT_SUCCESS(eResult)) eResult = STATUS_UNSUCCESSFUL;
	}
	IFERR_RET(eResult);

	LOGINFO2("sendEvent: %u (registry), pid: %Iu, key: <%wZ>.\r\n", (ULONG)eEvent,
		(ULONG_PTR)PsGetCurrentProcessId(), RegKeyName().initPrintable(pRegistryObject));

	IFERR_RET(fltport::sendRawEvent(serializer));

	return STATUS_SUCCESS;
}

//
//
//
template<typename Fn>
NTSTATUS createEventData(EventData** ppEventData, SysmonEvent eEvent, 
	PVOID pObject, Fn fnWriteAdditionalData)
{
	NTSTATUS eResult = STATUS_SUCCESS;
	DefaultSerializer serializer;

	// access to user data can throw exceptions
	__try
	{
		IFERR_RET(createRegistryBaseEvent_Hashed(serializer, eEvent, pObject));
		IFERR_RET(createRegistryBaseEvent_NonHashed(serializer, eEvent, pObject));
		
		// Warning is appear if fnWriteAdditionalData is trivial (just return STATUS_SUCCESS)
		#pragma warning (suppress : 4127)
		IFERR_RET(fnWriteAdditionalData(&serializer));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		eResult = GetExceptionCode();
		if (NT_SUCCESS(eResult)) eResult = STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(eResult)) return eResult;
	IFERR_RET(allocEventData(ppEventData, serializer, eEvent));

	return STATUS_SUCCESS;
}

} // namespace detail

//
// Create and fill new EventData for sending in post-action
//
template<typename Fn>
EventData* createEventData(SysmonEvent eEvent, PVOID pObject, 
	Fn fnWriteAdditionalData)
{
	if (!g_pCommonData->fEnableMonitoring) 
		return nullptr;
	if (!isEventEnabled(eEvent))
		return nullptr;
	ULONG_PTR nProcessId = (ULONG_PTR)PsGetCurrentProcessId();
	if (procmon::isProcessInWhiteList(nProcessId))
		return nullptr;

	EventData* pEventData = nullptr;
	NTSTATUS eResult = detail::createEventData(&pEventData, eEvent, pObject, 
		fnWriteAdditionalData);

	if (!NT_SUCCESS(eResult))
	{
		LOGERROR(eResult, "Can't create registry event %u, pid: %Iu, key: <%wZ>.\r\n", 
			(ULONG)eEvent, nProcessId, RegKeyName().initPrintable(pObject));
		return nullptr;
	}

	return pEventData;
}

//
//
//
EventData* createEventData(SysmonEvent eEvent, PVOID pObject)
{
	return createEventData(eEvent, pObject, [](auto /*pSerializer*/) {return STATUS_SUCCESS; });
}
//
// Create, fill and send event
//
template<typename Fn>
void sendEventData(SysmonEvent eEvent, PVOID pObject, Fn fnWriteAdditionalData)
{
	if (!g_pCommonData->fEnableMonitoring)
		return;
	if (!isEventEnabled(eEvent))
		return;

	HANDLE nProcessId = PsGetCurrentProcessId();
	procmon::ContextPtr pProcessCtx;
	(void)fillCurProcContext(pProcessCtx);
	if (isProcessInWhiteList(pProcessCtx))
		return;

	NTSTATUS eResult = detail::sendEventData(eEvent, pObject, pProcessCtx, fnWriteAdditionalData);
	if (!NT_SUCCESS(eResult))
	{
		LOGERROR(eResult, "Can't send registry event %u, pid: %Iu, key: <%wZ>.\r\n", 
			(ULONG)eEvent, nProcessId, RegKeyName().initPrintable(pObject));
		return;
	}
}

//
//
//
void sendEventData(SysmonEvent eEvent, PVOID pObject)
{
	sendEventData(eEvent, pObject, [](auto /*pSerializer*/) {return STATUS_SUCCESS; });
}

//
// Writes registry value into serializer
//
// REG_SZ, REG_EXPAND_SZ, REG_LINK -> string
// REG_MULTI_SZ -> string with 0x01 as delimeter
//
// REG_DWORD -> uint32
// REG_QWORD -> uint64
// REG_DWORD_BIG_ENDIAN -> uint32 and reverse bytes
//
NTSTATUS writeRegValue(DefaultSerializer& serializer, ULONG eType, 
	const void* pData, size_t nDataSize)
{
	// write rawData
	switch (eType)
	{
		case REG_SZ:
		case REG_EXPAND_SZ:
		case REG_LINK:
		{
			if (nDataSize > (size_t)USHRT_MAX)
				return STATUS_BUFFER_OVERFLOW;

			UNICODE_STRING usStr = {};
			usStr.Buffer = (PWSTR)pData;
			usStr.Length = (USHORT)nDataSize;
			usStr.MaximumLength = (UINT16)nDataSize;
			if (!write(serializer, EvFld::RegistryRawData, &usStr))
				return STATUS_NO_MEMORY;
			return STATUS_SUCCESS;
		}
		case REG_DWORD:
		{
			if (nDataSize < sizeof(uint32_t))
				return STATUS_INVALID_PARAMETER;

			if (!serializer.write(EvFld::RegistryRawData, *(uint32_t*)pData))
				return STATUS_NO_MEMORY;
			return STATUS_SUCCESS;
		}
		case REG_DWORD_BIG_ENDIAN:
		{
			if (nDataSize < sizeof(uint32_t))
				return STATUS_INVALID_PARAMETER;

			if (!serializer.write(EvFld::RegistryRawData, 
				(uint32_t)RtlUlongByteSwap(*(uint32_t*)pData)))
				return STATUS_NO_MEMORY;
			return STATUS_SUCCESS;
		}
		case REG_QWORD:
		{
			if (nDataSize < sizeof(uint64_t))
				return STATUS_INVALID_PARAMETER;

			if (!serializer.write(EvFld::RegistryRawData, *(uint64_t*)pData))
				return STATUS_NO_MEMORY;
			return STATUS_SUCCESS;
		}
		case REG_MULTI_SZ:
		{
			if (nDataSize < sizeof(wchar_t))
				return STATUS_INVALID_PARAMETER;

			wchar_t* pStart = nullptr;
			IFERR_RET(captureBuffer((PVOID*)&pStart, pData, nDataSize));

			size_t nSizeCh = nDataSize / sizeof(wchar_t) - 1 /*Keep last zero*/;
			size_t nRealSizeCh = nSizeCh;

			// replace 0x00 -> 0x01
			for (size_t i = 0; i < nSizeCh; i++)
			{
				if (pStart[i] != 0) continue;
				pStart[i] = 0x01;

				if (pStart[i + 1] == 0)
				{
					nRealSizeCh = i + 1;
					break;
				}
			}

			bool fWriteResult = serializer.write(EvFld::RegistryRawData, 
				pStart, nRealSizeCh);
			ExFreePoolWithTag(pStart, c_nAllocTag);
			if(!fWriteResult) 
				return STATUS_NO_MEMORY;

			return STATUS_SUCCESS;
		}
		default:
		{
			// TODO: Data type filtration
			if (!serializer.write(EvFld::RegistryRawData, pData, nDataSize))
				return STATUS_NO_MEMORY;

			return STATUS_SUCCESS;
		}
	}

}

//////////////////////////////////////////////////////////////////////////
//
// Registry callback
//
//////////////////////////////////////////////////////////////////////////


//
//  This is the registry callback we'll register to intercept all registry operations. 
//  
//  Arguments:
//  
//      pCallbackContext - The value that the driver passed to the Context parameter
//          of CmRegisterCallbackEx when it registers this callback routine.
//  
//      pArgument1 - A REG_NOTIFY_CLASS typed value that identifies the type of 
//          registry operation that is being performed and whether the callback
//          is being called in the pre or post phase of processing.
//  
//      pArgument2 - A pointer to a structure that contains information specific
//          to the type of the registry operation. The structure type depends
//          on the REG_NOTIFY_CLASS value of pArgument1. Refer to MSDN for the 
//          mapping from REG_NOTIFY_CLASS to REG_XXX_KEY_INFORMATION.
//
NTSTATUS notifyOnRegistryActions(_In_ PVOID /*pCallbackContext*/, 
	_In_opt_ PVOID pArgument1, _In_opt_ PVOID pArgument2)
{
	// This should never happen but the sal annotation on the callback 
	// function marks Argument 2 as opt and is looser than what it actually is.
	if (pArgument2 == NULL) return STATUS_SUCCESS;

	REG_NOTIFY_CLASS eNotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)pArgument1;

	switch (eNotifyClass) 
	{
		case RegNtPreRenameKey:
		{
			SysmonEvent eEvent = SysmonEvent::RegistryKeyNameChange;
			auto pInfo = (PREG_RENAME_KEY_INFORMATION)pArgument2;
			pInfo->CallContext = nullptr;
			if (pInfo->NewName == nullptr) return STATUS_SUCCESS;

			// Self defense
			if (!isFullAccessAllowed(pInfo->Object, eNotifyClass))
				return STATUS_ACCESS_DENIED;

			pInfo->CallContext = createEventData(eEvent, pInfo->Object,
				[pInfo](auto pSerializer)
				{
					// Get new name
					if (pInfo->NewName == nullptr)
						return STATUS_UNSUCCESSFUL;
					if (!write(*pSerializer, EvFld::RegistryKeyNewName, pInfo->NewName)) 
						return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RegistryKeyNewName);
					return STATUS_SUCCESS;
				}
			);

			return STATUS_SUCCESS;
		}
		case RegNtPreDeleteValueKey:
		{
			SysmonEvent eEvent = SysmonEvent::RegistryValueDelete;
			auto pInfo = (PREG_DELETE_VALUE_KEY_INFORMATION)pArgument2;
			pInfo->CallContext = nullptr;

			// Disable self-defense hack
			// access to user data can throw exceptions
			__try
			{
				if (pInfo->ValueName != nullptr &&
					RtlEqualUnicodeString(pInfo->ValueName, &c_usDisableSelfDefenceValueName, FALSE))
				{
					#pragma warning(suppress : 28175)
					g_pCommonData->pDriverObject->DriverUnload = g_pCommonData->pfnSaveDriverUnload;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}

			// Self defense
			if (!isFullAccessAllowed(pInfo->Object, eNotifyClass))
				return STATUS_ACCESS_DENIED;

			pInfo->CallContext = createEventData(eEvent, pInfo->Object, 
				[pInfo](auto pSerializer)
				{
					// Get name
					if (pInfo->ValueName == nullptr)
						return STATUS_UNSUCCESSFUL;
					if (!write(*pSerializer, EvFld::RegistryName, pInfo->ValueName))
						return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RegistryName);
					return STATUS_SUCCESS;
				}
			);

			return STATUS_SUCCESS;
		}
		case RegNtPreDeleteKey:
		{
			SysmonEvent eEvent = SysmonEvent::RegistryKeyDelete;
			auto pInfo = (PREG_DELETE_KEY_INFORMATION)pArgument2;

			// Self defense
			if (!isFullAccessAllowed(pInfo->Object, eNotifyClass))
				return STATUS_ACCESS_DENIED;

			pInfo->CallContext = createEventData(eEvent, pInfo->Object);

			return STATUS_SUCCESS;
		}
		case RegNtPreSetValueKey:
		{
			auto pInfo = (PREG_SET_VALUE_KEY_INFORMATION)pArgument2;

			// Self defense
			if (!isFullAccessAllowed(pInfo->Object, eNotifyClass))
				return STATUS_ACCESS_DENIED;
			return STATUS_SUCCESS;
		}
		case RegNtPreCreateKeyEx:
		{
			auto pPreInfo = (PREG_CREATE_KEY_INFORMATION_V1)pArgument2;

			// Self defense check
			// access to user data can throw exceptions
			__try
			{
				auto eAllowedAccessType = getAllowedAccessInfo(pPreInfo->RootObject, pPreInfo->CompleteName);
				if (eAllowedAccessType.eAccessType != AccessType::Full)
				{
					// deny access
					LOGINFO1("Selfdefense: Deny access: pid: %Iu, action: %u, root: <%wZ>, key: <%wZ>.\r\n",
						(ULONG_PTR)PsGetCurrentProcessId(), (ULONG)eNotifyClass, 
						RegKeyName().initPrintable(pPreInfo->RootObject), pPreInfo->CompleteName);

					return STATUS_ACCESS_DENIED;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}

			return STATUS_SUCCESS;
		}
		case RegNtPreOpenKeyEx:
		{
			auto pPreInfo = (PREG_OPEN_KEY_INFORMATION_V1)pArgument2;

			// Self defense check
			// access to user data can throw exceptions
			__try
			{
				auto eAllowedAccessType = getAllowedAccessInfo(pPreInfo->RootObject, pPreInfo->CompleteName);
				if(!isAccessAllowed(pPreInfo->DesiredAccess, eAllowedAccessType))
				{
					// deny access
					LOGINFO1("Selfdefense: Deny access: pid: %Iu, action: %u, access: 0x%08X, root: <%wZ>, key: <%wZ>.\r\n",
						(ULONG_PTR)PsGetCurrentProcessId(), (ULONG)eNotifyClass, (ULONG)pPreInfo->DesiredAccess,
						RegKeyName().initPrintable(pPreInfo->RootObject), pPreInfo->CompleteName);

					return STATUS_ACCESS_DENIED;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}

			return STATUS_SUCCESS;
		}
		case RegNtPostCreateKeyEx:
		{
			SysmonEvent eEvent = SysmonEvent::RegistryKeyCreate;
			auto pPostInfo = (PREG_POST_OPERATION_INFORMATION)pArgument2;

			auto pPreInfo = (PREG_CREATE_KEY_INFORMATION_V1)pPostInfo->PreInformation;
			
			// Skip opening of exist keys
			bool fActionIsCreation = true;
			__try
			{
				// access to user data can throw exceptions
				if (pPreInfo->Disposition == nullptr)
					__leave;
				ULONG eDisposition = *pPreInfo->Disposition;
				if (eDisposition == REG_OPENED_EXISTING_KEY)
					fActionIsCreation = false;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
			if (!fActionIsCreation)
				return STATUS_SUCCESS;

			if (pPostInfo->Status == STATUS_SUCCESS)
				sendEventData(eEvent, pPostInfo->Object);

			return STATUS_SUCCESS;
		}
		case RegNtPostSetValueKey:
		{
			SysmonEvent eEvent = SysmonEvent::RegistryValueSet;
			auto pPostInfo = (PREG_POST_OPERATION_INFORMATION)pArgument2;
			// Skip processing unsuccesssful operations
			if (pPostInfo->Status != STATUS_SUCCESS) return STATUS_SUCCESS;


			// filter events by value
			auto pInfo = (PREG_SET_VALUE_KEY_INFORMATION) pPostInfo->PreInformation;
			// by type
			if(!isValueTypeEnabled(pInfo->Type))
				return STATUS_SUCCESS;
			// by size
			if (pInfo->DataSize >= g_pCommonData->nMaxRegValueSize)
				return STATUS_SUCCESS;

			// Send event
			sendEventData(eEvent, pPostInfo->Object, 
				[pPostInfo](auto pSerialzer)
				{
					auto pInfo = (PREG_SET_VALUE_KEY_INFORMATION)
						pPostInfo->PreInformation;
					
					// write type
					if (!pSerialzer->write(EvFld::RegistryDataType, (uint32_t)pInfo->Type))
						return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RegistryDataType);
					
					// write value
					IFERR_RET(writeRegValue(*pSerialzer, pInfo->Type, pInfo->Data, pInfo->DataSize),
						"Can't write reg data type: %u, size: %u.\r\n", (ULONG)pInfo->Type, (ULONG)pInfo->DataSize);

					// Get new name
					if (pInfo->ValueName == nullptr)
					{
						if (!pSerialzer->write(EvFld::RegistryName, ""))
							return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RegistryName);
					}
					else
					{
						if (!write(*pSerialzer, EvFld::RegistryName, pInfo->ValueName))
							return LOGERROR(STATUS_NO_MEMORY, "Field: %u.\r\n", (ULONG)EvFld::RegistryName);
					}
					return STATUS_SUCCESS;
				}
			);

			return STATUS_SUCCESS;
		}
		case RegNtPostDeleteValueKey:
		case RegNtPostDeleteKey:
		case RegNtPostRenameKey:
		{
			// Sending event that was created on pre-action
			auto pPostInfo = (PREG_POST_OPERATION_INFORMATION)pArgument2;
			auto pEventData = (EventData*)pPostInfo->CallContext;
			if (pEventData == nullptr) return STATUS_SUCCESS;

			if (pPostInfo->Status == STATUS_SUCCESS)
			{
				ULONG_PTR nProcessId = (ULONG_PTR)PsGetCurrentProcessId();
				LOGINFO2("sendEvent: %u (registry), pid: %Iu, key: <%wZ>.\r\n", 
					(ULONG)pEventData->eEvent, nProcessId, RegKeyName().initPrintable(pPostInfo->Object));
				IFERR_LOG(sendEventData(pEventData),
					"Can't send registry event %u, pid: %Iu, key: <%wZ>.\r\n", 
					(ULONG)pEventData->eEvent, nProcessId, RegKeyName().initPrintable(pPostInfo->Object));
			}

			freeEventData(pEventData);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_SUCCESS;
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
	LOGINFO2("Register RegistryFilter\r\n");

	IFERR_LOG(loadRulesFromConfig());

	UNICODE_STRING usCallbackAltitude = {};
	RtlInitUnicodeString(&usCallbackAltitude, edrdrv::c_sAltitudeValue);

	IFERR_RET(CmRegisterCallbackEx(notifyOnRegistryActions, &usCallbackAltitude,
		g_pCommonData->pDriverObject, NULL, &g_pCommonData->nRegFltCookie, NULL));
	g_pCommonData->fRegFltStarted = TRUE;

	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (!g_pCommonData->fRegFltStarted) return;
	
	IFERR_LOG(CmUnRegisterCallback(g_pCommonData->nRegFltCookie));
	g_pCommonData->fRegFltStarted = FALSE;
}

} // namespace regmon
} // namespace openEdr
/// @}
