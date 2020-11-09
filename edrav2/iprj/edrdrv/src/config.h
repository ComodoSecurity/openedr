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
#pragma once

namespace openEdr {

///
/// Load configuration from the registry.
///
NTSTATUS loadMainConfigFromRegistry();

///
/// Save configuration to the registry.
///
NTSTATUS saveMainConfigToRegistry();

///
/// Load configuration from buffer (LBVS format).
///
NTSTATUS loadMainConfigFromBuffer(const void* pBuffer, size_t nBufferSize, bool& fChanged);

namespace cfg {

//
//
//
enum class ConfigId
{
	Invalid = 0,
	Main = 1,
	TrustedProcess = 2,
	ProtectedProcess = 3,
	InjectProcess = 4,
	SendEventProcess = 5,
	ProtectedFiles = 6,
	ProtectedRegKeys = 7,
};

///
/// Save config data.
///
NTSTATUS saveConfigBlob(ConfigId eCfgId, const void* pData, size_t nDataSize);

///
/// Save config data.
///
template<typename TypeId>
NTSTATUS saveConfigBlob(ConfigId eCfgId, NonPagedLbvsSerializer<TypeId>& serializer)
{
	void* pData = nullptr;
	size_t nDataSize = 0;
	if (!serializer.getData(&pData, &nDataSize)) return STATUS_UNSUCCESSFUL;
	return saveConfigBlob(eCfgId, pData, nDataSize);
}

///
/// Load config data.
///
/// Returns empty blob if config not found.
///
NTSTATUS loadConfigBlob(ConfigId eCfgId, Blob<PoolType::NonPaged> & blob);

///
/// Remove config data.
///
NTSTATUS removeConfigBlob(ConfigId eCfgId);

///
/// Initialization.
///
NTSTATUS initialize();

///
/// Finalization.
///
void finalize();

} // namespace cfg
} // namespace openEdr
/// @}
