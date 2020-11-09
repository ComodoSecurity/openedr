//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Communication Port
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace openEdr {
namespace fltport {

///
/// Checks is client connected.
///
/// @returns The function returns `true` is client is connected.
///
bool isClientConnected();

///
/// Sync send buffer with raw event.
///
/// @param pRawEvent - pointer to data.
/// @param nRawEventSize - size of data.
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS sendRawEvent(const void* pRawEvent, size_t nRawEventSize);

template<typename IdType, typename MemAllocator, size_t c_nBufferSize, size_t c_nLimitSize>
NTSTATUS sendRawEvent(variant::BasicLbvsSerializer<IdType, MemAllocator, c_nBufferSize, c_nLimitSize>& serializer)
{
	void* pData = nullptr;
	size_t nDataSize = 0;
	if (!serializer.getData(&pData, &nDataSize)) return STATUS_UNSUCCESSFUL;
	return fltport::sendRawEvent(pData, nDataSize);
}

///
/// Updates the configuration.
///
void reloadConfig();

///
/// Initialization.
///
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS initialize();

///
/// Start.
///
void start();

///
/// Stop.
///
/// @param fClearQueue - TBD.
///
void stop(bool fClearQueue);

///
/// Finalization.
///
void finalize();

} // namespace fltport
} // namespace openEdr
/// @}
