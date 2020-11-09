//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: 
//
///
/// @file Driver configuration controller
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "osutils.h"
#include "fltport.h"
#include "config.h"
#include "dllinj.h"


namespace openEdr {



//////////////////////////////////////////////////////////////////////////
//
// Utils
//

//
// 
//
template<typename IdType, typename MemAllocator, size_t c_nBufferSize, size_t c_nLimitSize,
	typename AtomicUnderlying, typename AtomicTypeTraits>
inline bool write(
	variant::BasicLbvsSerializer<IdType, MemAllocator, c_nBufferSize, c_nLimitSize>& serializer, 
	IdType eFieldId, AtomicBase<AtomicUnderlying, AtomicTypeTraits>& storage)
{
	AtomicUnderlying value = storage;
	return serializer.write(eFieldId, value);
}

//////////////////////////////////////////////////////////////////////////
//
// Config loader/saver
//

//
// Configuration data
//

#define GLOBAL_DATA_CONFIGURATION_MAP \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::ConnectionTimeot, nFltPortConnectionTimeot); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::MaxQueueSize, nFltPortMaxQueueSize); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::SendMsgTimeout, nFltPortSendMessageTimeout); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::EventFlags, nSentEventsFlags); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::LogLevel, nLogLevel); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::MaxFullActFileSize, nMaxFullActFileSize); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::MaxRegValueSize, nMaxRegValueSize); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::MinFullActFileSize, nMinFullActFileSize); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::SentRegTypes, nSentRegTypesFlags); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::EnableDllInject, fEnableDllInjection); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::VerifyInjectedDll, fVerifyInjectedDll); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::OpenProcessRepeatTimeout, nOpenProcessRepeatTimeout); \
	PROCESS_GLOBAL_CONFIG_ITEM(ConfigField::RegEventRepeatTimeout, nRegEventRepeatTimeout); \

//
//
//
NTSTATUS loadRulesFromConfig(variant::BasicLbvsDeserializer<ConfigField>& deserializer, bool& fChanged)
{
	ScopedLock lock(g_pCommonData->mtxConfig);

	uint64_t eConfigFields = 0;
	uint64_t eChangedConfigFields = 0;

	List<DynUnicodeString> injectedDllList;

	// Iterate deserializer
	for (auto& item : deserializer)
	{
		setFlag(eConfigFields, convertToFlag(item.id));

		// Process simple items
		#define PROCESS_GLOBAL_CONFIG_ITEM(id, storage) \
			case id: \
				if(NT_SUCCESS(getValue(item, g_pCommonData->storage))) \
					setFlag(eChangedConfigFields, convertToFlag(id)); \
			break;

		switch (item.id)
		{
			GLOBAL_DATA_CONFIGURATION_MAP;
		}
		#undef PROCESS_GLOBAL_CONFIG_ITEM

		switch (item.id)
		{
			// Process log file
			case ConfigField::LogFile:
			{
				DynUnicodeString usLogFile;
				if (!NT_SUCCESS(getValue(item, usLogFile)))
					continue; // goto next item
				g_pCommonData->usLogFilepath = std::move(usLogFile);
				IFERR_LOG(log::setLogFile(g_pCommonData->usLogFilepath), "Can't set Log file: <%wZ>.\r\n", 
					(PCUNICODE_STRING)g_pCommonData->usLogFilepath);
				continue; // goto next item
			}
			// Process filemon mask
			case ConfigField::FileMonNameMask:
			{
				DynUnicodeString usFileMonMask;
				if (!NT_SUCCESS(getValue(item, usFileMonMask)))
					continue; // goto next item

				ScopedLock lock2(g_pCommonData->mtxFilemonMask);
				g_pCommonData->usFilemonMask = std::move(usFileMonMask);
				g_pCommonData->fUseFilemonMask = g_pCommonData->usFilemonMask.getLengthCb() != 0;
				continue; // goto next item
			}
			// Process InjectedDll
			case ConfigField::InjectedDll:
			{
				DynUnicodeString usData;
				if (!NT_SUCCESS(getValue(item, usData)))
					continue; // goto next item
				IFERR_LOG(injectedDllList.pushFront(std::move(usData)));
				continue; // goto next item
			}

			// Process driver self protection
			case ConfigField::DisableSelfProtection:
			{
				bool fDisableSelfProtection;
				if (!NT_SUCCESS(getValue(item, fDisableSelfProtection)))
					continue; // goto next item

				if(g_pCommonData->fDisableSelfProtection == fDisableSelfProtection)
					continue; // goto next item

				g_pCommonData->fDisableSelfProtection = fDisableSelfProtection;
				#pragma warning(suppress : 28175)
				g_pCommonData->pDriverObject->DriverUnload = (fDisableSelfProtection) ?
					g_pCommonData->pfnSaveDriverUnload : nullptr;
				LOGINFO1("Selfdefense: Changes state to: %s.\r\n", (fDisableSelfProtection) ? "disabled" : "enabled");
				continue; // goto next item
			}
		}
	}

	// Process VerifyInjectedDll
	dllinj::enableInjectedDllVerification(g_pCommonData->fVerifyInjectedDll);

	// Process injected DLLs
	if (!injectedDllList.isEmpty())
	{
		IFERR_LOG(dllinj::setInjectedDllList(injectedDllList));
		g_pCommonData->injectedDllList = std::move(injectedDllList);
	}	

	// Process results
	if (testFlag(eChangedConfigFields,
		convertToFlag(ConfigField::ConnectionTimeot) | convertToFlag(ConfigField::MaxQueueSize) |
		convertToFlag(ConfigField::SendMsgTimeout)))
	{
		fltport::reloadConfig();
	}


	if (eChangedConfigFields != 0)
	{
		fChanged = true;
		g_pCommonData->eChangedConfigFields |= eChangedConfigFields;
	}
	else
	{
		fChanged = true;
	}

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS saveConfig(NonPagedLbvsSerializer<ConfigField>& serializer)
{
	ScopedLock lock(g_pCommonData->mtxConfig);
	const uint64_t eChangedConfigFields = g_pCommonData->eChangedConfigFields;

	// Process simple items
	#define PROCESS_GLOBAL_CONFIG_ITEM(id, storage) \
	if (testFlag(eChangedConfigFields, convertToFlag(id))) \
		if(!write(serializer, id, g_pCommonData->storage)) \
			return LOGERROR(STATUS_NO_MEMORY, "Can't serialize config field: %u.\r\n", (ULONG)id);
	GLOBAL_DATA_CONFIGURATION_MAP;
	#undef PROCESS_GLOBAL_CONFIG_ITEM

	// Process filemon mask
	if (g_pCommonData->fUseFilemonMask)
	{
		ScopedLock lock2(g_pCommonData->mtxFilemonMask);
		if (!write(serializer, ConfigField::FileMonNameMask, (PCUNICODE_STRING)g_pCommonData->usFilemonMask))
			return LOGERROR(STATUS_NO_MEMORY, "Can't serialize config field: %u.\r\n", 
			(ULONG)ConfigField::FileMonNameMask);
	}

	// Process injected DLLs
	if (!g_pCommonData->injectedDllList.isEmpty())
	{
		for (auto& usName : g_pCommonData->injectedDllList)
		{
			if (!write(serializer, ConfigField::InjectedDll, (PCUNICODE_STRING)usName))
				return LOGERROR(STATUS_NO_MEMORY, "Can't serialize config field: %u.\r\n",
				(ULONG)ConfigField::InjectedDll);
		}
	}

	return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//
// External API
//

PRESET_STATIC_UNICODE_STRING(c_usConfigRegValueName, CMD_ERDDRV_CFG_REG_VALUE_NAME);

//
//
//
NTSTATUS saveMainConfigToRegistry()
{
	// Serialize changed data
	NonPagedLbvsSerializer<ConfigField> serializer;
	IFERR_RET(saveConfig(serializer));

	// Write data to storage
	IFERR_RET(saveConfigBlob(cfg::ConfigId::Main, serializer));
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS loadMainConfigFromRegistry()
{
	// Read data from storage
	Blob DataBuffer;
	IFERR_RET(loadConfigBlob(cfg::ConfigId::Main, DataBuffer));
	if (DataBuffer.isEmpty())
		return STATUS_SUCCESS;

	// Deserialize and load
	bool fChanged;
	IFERR_RET(loadMainConfigFromBuffer(DataBuffer.getData(), DataBuffer.getSize(), fChanged),
		"Can't deserialize config from registry.\r\n");

	return STATUS_SUCCESS;
}

//
// Load configuration from LBVS format
//
NTSTATUS loadMainConfigFromBuffer(const void* pBuffer, size_t nBufferSize, bool& fChanged)
{
	// Create deserializer
	variant::BasicLbvsDeserializer<ConfigField> deserializer;
	if (!deserializer.reset(pBuffer, nBufferSize))
		return LOGERROR(STATUS_DATA_ERROR, "Invalid config data format");

	return loadRulesFromConfig(deserializer, fChanged);
}


namespace cfg {


///
/// Blob storage
///   Provides saving several config in registry
///
struct ConfigData
{
	struct Element
	{
		ConfigId eId = ConfigId::Invalid;
		Blob<PoolType::NonPaged> data;
	};
	
	typedef List<Element> BlobList;
	BlobList m_blobList;
	KMutex m_mtxBlobList;

};

static ConfigData* g_pConfigData = nullptr;


//
//
//
NTSTATUS moveBlobToList(ConfigData::BlobList& blobList, ConfigId eCfgId, Blob<PoolType::NonPaged>& blob)
{
	ConfigData::Element elem;
	elem.eId = eCfgId;
	elem.data = std::move(blob);

	// Find exist
	for (auto& cfgBlob : blobList)
	{
		if (cfgBlob.eId != elem.eId)
			continue;
		cfgBlob.data = std::move(elem.data);
		return STATUS_SUCCESS;
	}

	// Add new
	IFERR_RET(blobList.pushBack(std::move(elem)));

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS saveConfigsToRegistry(void* pData, size_t nDataSize)
{
	if (g_pCommonData->usRegistryPath.getLengthCb() == 0)
		return LOGERROR(STATUS_SUCCESS, "Registry path is not set.\r\n");

	// Open regkey
	UniqueHandle hDriverRegKey;
	{
		OBJECT_ATTRIBUTES objAttributes = {};
		UNICODE_STRING usRegistryPath = g_pCommonData->usRegistryPath;
		#pragma warning(suppress : 26477)
		InitializeObjectAttributes(&objAttributes, &usRegistryPath,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
		IFERR_RET(ZwOpenKey(hDriverRegKey.getPtr(), KEY_WRITE, &objAttributes),
			"Can't open driver regkey: \"%wZ\"\r\n", &usRegistryPath);
	}

	IFERR_RET(ZwSetValueKey(hDriverRegKey, &c_usConfigRegValueName, 0, REG_BINARY,
		pData, (ULONG) nDataSize), "Can't save config to registry");

	return STATUS_SUCCESS;
}

//
// Save all blobs into registry
//
NTSTATUS saveConfigsToRegistry()
{
	NonPagedLbvsSerializer<ConfigId> serializer;

	for (auto& blob : g_pConfigData->m_blobList)
		if (!serializer.write(blob.eId, static_cast<const void*> (blob.data.getData()), blob.data.getSize()))
			return LOGERROR(STATUS_NO_MEMORY);

	void* pData = nullptr;
	size_t nDataSize = 0;
	if (!serializer.getData(&pData, &nDataSize))
		return LOGERROR(STATUS_UNSUCCESSFUL);
		
	IFERR_RET(saveConfigsToRegistry(pData, nDataSize));
	return STATUS_SUCCESS;
}


//
//
//
NTSTATUS loadConfigsFromRegistry(Blob<PoolType::NonPaged>& value)
{
	if (g_pCommonData->usRegistryPath.getLengthCb() == 0)
		return LOGERROR(STATUS_SUCCESS, "Registry path is not set.\r\n");

	value.clear();

	// Open regkey
	UniqueHandle hDriverRegKey;
	OBJECT_ATTRIBUTES objAttributes = {};
	UNICODE_STRING usRegistryPath = g_pCommonData->usRegistryPath;
	#pragma warning(suppress : 26477)
	InitializeObjectAttributes(&objAttributes, &usRegistryPath,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
	IFERR_RET(ZwOpenKey(hDriverRegKey.getPtr(), KEY_READ, &objAttributes),
		"Can't open driver regkey: \"%wZ\"\r\n", &usRegistryPath);

	// Read data
	ULONG ResultLength = 0;
	const NTSTATUS eStatus = ZwQueryValueKey(hDriverRegKey, &c_usConfigRegValueName,
		KeyValuePartialInformation, NULL, 0, &ResultLength);
	// Config is not found
	if (eStatus == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		LOGINFO1("Config is absent in registry.\r\n");
		return STATUS_SUCCESS;
	}
	if (eStatus != STATUS_BUFFER_TOO_SMALL && eStatus != STATUS_BUFFER_OVERFLOW)
		return LOGERROR(eStatus, "Can't open �onfig regvalue: \"%wZ\".\r\n", &c_usConfigRegValueName);

	
	Blob<PoolType::NonPaged> result;
	IFERR_RET(result.alloc(ResultLength));
	IFERR_RET(ZwQueryValueKey(hDriverRegKey, &c_usConfigRegValueName,
		KeyValuePartialInformation, result.getData(), ResultLength, &ResultLength));

	const PKEY_VALUE_PARTIAL_INFORMATION pValue = reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(result.getData());
	if (pValue->Type != REG_BINARY)
		return LOGERROR(STATUS_INVALID_PARAMETER, "Invalid config regvalue: \"%wZ\".\r\n", &c_usConfigRegValueName);

	IFERR_RET(value.alloc(&pValue->Data, pValue->DataLength));
	return STATUS_SUCCESS;
}


///
/// Save config data
///
NTSTATUS saveConfigBlob(ConfigId eCfgId, const void* pData, size_t nDataSize)
{
	Blob<PoolType::NonPaged> blob;
	IFERR_RET(blob.alloc(pData, nDataSize));

	ScopedLock lock(g_pConfigData->m_mtxBlobList);

	IFERR_RET(moveBlobToList(g_pConfigData->m_blobList, eCfgId, blob));
	IFERR_RET(saveConfigsToRegistry());

	return STATUS_SUCCESS;
}

///
/// Load config data
/// Returns empty blob if config not found
///
NTSTATUS loadConfigBlob(ConfigId eCfgId, Blob<PoolType::NonPaged>& blob)
{
	ScopedLock lock(g_pConfigData->m_mtxBlobList);

	for (const auto& cfgBlob : g_pConfigData->m_blobList)
	{
		if(cfgBlob.eId != eCfgId)
			continue;

		IFERR_RET(blob.alloc(cfgBlob.data));
		return STATUS_SUCCESS;

	}

	blob.clear();
	return STATUS_SUCCESS;
}

///
/// Remove config data
///
NTSTATUS removeConfigBlob(ConfigId eCfgId)
{
	ScopedLock lock(g_pConfigData->m_mtxBlobList);

	const auto nRemovedCount = g_pConfigData->m_blobList.removeIf([eCfgId](auto& data) -> bool
	{
		return data.eId == eCfgId;
	});

	if(nRemovedCount != 0)
		IFERR_RET(saveConfigsToRegistry());
	return STATUS_SUCCESS;
}


///
/// Initialization
///
NTSTATUS initialize()
{
	if (g_pConfigData != nullptr)
		return STATUS_SUCCESS;

	#pragma warning(suppress : 26409)
	g_pConfigData = new(NonPagedPool) ConfigData;
	if (!g_pConfigData)
		return LOGERROR(STATUS_NO_MEMORY);

	// Load data from registry
	Blob<PoolType::NonPaged> regData;
	IFERR_LOG(loadConfigsFromRegistry(regData));
	// If no data in registry
	if (regData.isEmpty())
		return STATUS_SUCCESS;
	
	// Deserialize blob list
	ConfigData::BlobList blobList;

	variant::BasicLbvsDeserializer<ConfigId> deserializer;
	if (!deserializer.reset(regData.getData(), regData.getSize()))
		return LOGERROR(STATUS_UNSUCCESSFUL, "Can't deserialize edrdrv.sys config.\r\n");
	for (auto& item : deserializer)
	{
		// Allow only FieldType::Stream
		if(item.type != variant::lbvs::FieldType::Stream)
			continue;

		Blob<PoolType::NonPaged> blob;
		IFERR_LOG(blob.alloc(item.data, item.size));
		if (blob.isEmpty())
			continue;

		IFERR_LOG(moveBlobToList(blobList, item.id, blob));
	}

	// Replace bloblist
	{
		ScopedLock lock(g_pConfigData->m_mtxBlobList);
		g_pConfigData->m_blobList = std::move(blobList);
	}

	return STATUS_SUCCESS;
}

///
/// Finalization
///
void finalize()
{
	if (g_pConfigData == nullptr)
		return;

	delete g_pConfigData;
	g_pConfigData = nullptr;
}

} // namespace cfg
} // namespace openEdr
/// @}
