//
// Comodo EDR extension of NetFilter SDK
//

#include "stdinc.h"
#include "nfdriver.h"

#define CMDEDR_EXTAPI_DRIVER
#include "../include/cmdedr_extapi.h"

//////////////////////////////////////////////////////////////////////////
//
// Tools
//

//
// Print addr
//
void printAddress(const void* pAddrBuffer, const char* sPrefix)
{
	auto eIpFamily = ((sockaddr_in*)pAddrBuffer)->sin_family;

	if (eIpFamily == AF_INET)
	{
		auto pAddr = (sockaddr_in*)pAddrBuffer;

		KdPrint((DPREFIX"%s%u.%u.%u.%u:%d\r\n", sPrefix,
			pAddr->sin_addr.S_un.S_un_b.s_b1,
			pAddr->sin_addr.S_un.S_un_b.s_b2,
			pAddr->sin_addr.S_un.S_un_b.s_b3,
			pAddr->sin_addr.S_un.S_un_b.s_b4,
			htons(pAddr->sin_port)));
	}
	else
	{
		auto pAddr = (sockaddr_in6*)pAddrBuffer;

		KdPrint((DPREFIX"%s[%x:%x:%x:%x:%x:%x:%x:%x]:%d\r\n", sPrefix,
			pAddr->sin6_addr.u.Word[0],
			pAddr->sin6_addr.u.Word[1],
			pAddr->sin6_addr.u.Word[2],
			pAddr->sin6_addr.u.Word[3],
			pAddr->sin6_addr.u.Word[4],
			pAddr->sin6_addr.u.Word[5],
			pAddr->sin6_addr.u.Word[6],
			pAddr->sin6_addr.u.Word[7],
			htons(pAddr->sin6_port)));
	}
}



//////////////////////////////////////////////////////////////////////////
//
// netfilter extension
//

namespace cmd {
namespace netmon {

using namespace nfapi::cmdedr;

struct CalloutDescriptor;

// Function to add context
typedef NTSTATUS (*FnAddFilter) (AddingFilterContext* pContext, CalloutDescriptor* pCalloutDescriptor);

//
//
//
struct CalloutDescriptor
{
	FWPS_CALLOUT_CLASSIFY_FN fnClassify;
	FWPS_CALLOUT_NOTIFY_FN fnNotify;
	FnAddFilter fnAddFilter;
	GUID guidCalloutKey;
	const GUID* pGuidLayer;
	UINT32 nId;
};


//////////////////////////////////////////////////////////////////////////
//
// TcpListen
//

//
// TcpListen callout.classify
//
VOID classifyTcpListen(
	IN const FWPS_INCOMING_VALUES* inFixedValues,
	IN const FWPS_INCOMING_METADATA_VALUES* pMetadata,
	IN VOID* /*packet*/,
	IN const void* /*classifyContext*/,
	IN const FWPS_FILTER* /*filter*/,
	IN UINT64 /*flowContext*/,
	OUT FWPS_CLASSIFY_OUT* pClassifyOut)
{
	// "Listen" is TCP only layer

	// Get pid
	if (!FWPS_IS_METADATA_FIELD_PRESENT(pMetadata, FWPS_METADATA_FIELD_PROCESS_ID))
		return;


	ListenInfo listenInfo;
	RtlZeroMemory(&listenInfo, sizeof(listenInfo));

	listenInfo.nPid = (UINT32)pMetadata->processId;
	if (cmdedr_isProcessInWhiteList(listenInfo.nPid))
		return;

	switch (inFixedValues->layerId)
	{
	case FWPS_LAYER_ALE_AUTH_LISTEN_V4:
	{
		listenInfo.ip_family = AF_INET;
		auto pAddr = (sockaddr_in*)listenInfo.localAddr;
		pAddr->sin_family = AF_INET;

		auto& ipValue = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_ADDRESS].value;
		if (ipValue.type == FWP_UINT32)
			pAddr->sin_addr.S_un.S_addr = htonl(ipValue.uint32);

		auto& portValue = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_PORT].value;
		if (portValue.type == FWP_UINT16)
			pAddr->sin_port = htons(portValue.uint16);

		break;
	}
	case FWPS_LAYER_ALE_AUTH_LISTEN_V6:
	{
		listenInfo.ip_family = AF_INET6;
		auto pAddr = (sockaddr_in6*)listenInfo.localAddr;
		pAddr->sin6_family = AF_INET6;

		auto& ipValue = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V6_IP_LOCAL_ADDRESS].value;
		if (ipValue.type == FWP_BYTE_ARRAY16_TYPE)
			memcpy(&pAddr->sin6_addr, ipValue.byteArray16->byteArray16, NF_MAX_IP_ADDRESS_LENGTH);

		auto& portValue = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V6_IP_LOCAL_PORT].value;
		if (portValue.type == FWP_UINT16)
			pAddr->sin6_port = htons(portValue.uint16);
		break;
	}
	default:
		return;
	}

	KdPrint((DPREFIX"listen. Pid: %I64u\r\n", listenInfo.nPid));
	printAddress(listenInfo.localAddr, "listen. Addr: ");

	void* pData = ExAllocatePoolWithTag(NonPagedPool, sizeof(listenInfo), MEM_TAG);
	if (pData == nullptr)
		return;
	memcpy(pData, &listenInfo, sizeof(listenInfo));

	// Pass collected data to UserMode
	(void)cmdedr_pushCustomData((UINT16)CustomDataType::ListenInfo, pData, 0);
}

//
// TcpListen callout.notify
//
NTSTATUS notifyTcpListen(
	IN  FWPS_CALLOUT_NOTIFY_TYPE  /*notifyType*/,
	IN  const GUID* /*filterKey*/,
	IN  const FWPS_FILTER* /*filter*/)
{
	return STATUS_SUCCESS;
}


//
//
//
static NTSTATUS addTcpListenFilter(AddingFilterContext* pContext, CalloutDescriptor* pCalloutDescriptor)
{
	// Callout description
	FWPM_DISPLAY_DATA displayData;
	wchar_t sDescription[] = L"Comodo EDR Tcp Listen Callout";
	displayData.description = sDescription;
	wchar_t sName[] = L"Tcp Listen Callout";
	displayData.name = sName;

	// Add callout
	FWPM_CALLOUT callout;
	RtlZeroMemory(&callout, sizeof(callout));
	callout.calloutKey = pCalloutDescriptor->guidCalloutKey;
	callout.displayData = displayData;
	callout.applicableLayer = *pCalloutDescriptor->pGuidLayer;
	callout.flags = 0;
	NTSTATUS ns = FwpmCalloutAdd(pContext->hEngine, &callout, nullptr, nullptr);
	if (!NT_SUCCESS(ns)) return ns;

	// Add filter
	FWPM_FILTER filter;
	RtlZeroMemory(&filter, sizeof(FWPM_FILTER));
	filter.layerKey = *pCalloutDescriptor->pGuidLayer;
	filter.displayData = displayData;
	filter.action.type = FWP_ACTION_CALLOUT_INSPECTION; // Readonly filter
	filter.action.calloutKey = pCalloutDescriptor->guidCalloutKey;
	filter.subLayerKey = pContext->pRecvSubLayer->subLayerKey;
	filter.weight.type = FWP_EMPTY; // auto-weight.
	ns = FwpmFilterAdd(pContext->hEngine, &filter, nullptr, nullptr);
	if (!NT_SUCCESS(ns)) return ns;

	return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//
// Common functions
//

//
// Callouts array
//
static CalloutDescriptor g_Callouts[] = {
	{
		(FWPS_CALLOUT_CLASSIFY_FN)classifyTcpListen,
		(FWPS_CALLOUT_NOTIFY_FN)notifyTcpListen,
		addTcpListenFilter,
		// {0E1FB35B-B825-4EEB-87C6-E5052F90263A}
		{ 0xe1fb35b, 0xb825, 0x4eeb, { 0x87, 0xc6, 0xe5, 0x5, 0x2f, 0x90, 0x26, 0x3a } },
		&FWPM_LAYER_ALE_AUTH_LISTEN_V4,
		0
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)classifyTcpListen,
		(FWPS_CALLOUT_NOTIFY_FN)notifyTcpListen,
		addTcpListenFilter,
		// {30FC055C-60E0-491B-8D53-00E1E814B9DA}
		{ 0x30fc055c, 0x60e0, 0x491b, { 0x8d, 0x53, 0x0, 0xe1, 0xe8, 0x14, 0xb9, 0xda } },
		&FWPM_LAYER_ALE_AUTH_LISTEN_V6,
		0
	},
};

//
// Callout indexes in g_Callouts
//
namespace CalloutId
{
	constexpr size_t TcpListenV4 = 0;
	constexpr size_t TcpListenV6 = 1;
};


//
//
//
static NTSTATUS registerCallouts(void* pDeviceObject)
{
	for (size_t i = 0; i < ARRAYSIZE(g_Callouts); ++i)
	{
		FWPS_CALLOUT1 callout = {};
		callout.calloutKey = g_Callouts[i].guidCalloutKey;
		callout.flags = 0;
		callout.classifyFn = g_Callouts[i].fnClassify;
		callout.notifyFn = g_Callouts[i].fnNotify;
		callout.flowDeleteFn = nullptr;

		NTSTATUS ns = FwpsCalloutRegister1(pDeviceObject, &callout, &g_Callouts[i].nId);
		if (!NT_SUCCESS(ns))
			return ns;
	}

	return STATUS_SUCCESS;
}

//
//
//
static NTSTATUS addFilters(AddingFilterContext* pContext)
{
	for (size_t i = 0; i < ARRAYSIZE(g_Callouts); ++i)
	{
		NTSTATUS ns = g_Callouts[i].fnAddFilter(pContext, &g_Callouts[i]);
		if (!NT_SUCCESS(ns))
			return ns;
	}

	return STATUS_SUCCESS;
}

//
//
//
static void unregisterCallouts()
{
	for (size_t i = 0; i < ARRAYSIZE(g_Callouts); ++i)
	{
		const NTSTATUS ns = FwpsCalloutUnregisterByKey(&g_Callouts[i].guidCalloutKey);
		if (!NT_SUCCESS(ns))
		{
			ASSERT(false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Send data to user mode API
//

//
//
//
NTSTATUS popCustomData(CustomDataType nType, void* pData, void* pOutBuffer, UINT64* pnBufferSize)
{
	switch (nType)
	{
	case CustomDataType::ListenInfo:
		{
			if (pData == nullptr)
				return STATUS_UNSUCCESSFUL;
			if (*pnBufferSize < sizeof(ListenInfo))
				return STATUS_NO_MEMORY;
			memcpy(pOutBuffer, pData, sizeof(ListenInfo));
			*pnBufferSize = sizeof(ListenInfo);
			return STATUS_SUCCESS;
		}
	}
	return STATUS_NOT_SUPPORTED;
}

//
//
//
void freeCustomData(CustomDataType nType, void* pData, EndpointId nId)
{
	if (pData == nullptr)
		return;

	switch (nType)
	{
	case CustomDataType::ListenInfo:
		ExFreePool(pData);
	}
}

} // namespace netmon
} // namespace cmd


//////////////////////////////////////////////////////////////////////////
//
// External API functions
//

//
//
//
EXTERN_C NTSTATUS cmdedr_registerCallouts(void* pDeviceObject)
{
	return cmd::netmon::registerCallouts(pDeviceObject);
}

//
//
//
EXTERN_C NTSTATUS cmdedr_addFilters(struct AddingFilterContext* pContext)
{
	return cmd::netmon::addFilters(pContext);
}

//
//
//
EXTERN_C void cmdedr_unregisterCallouts()
{
	return cmd::netmon::unregisterCallouts();
}

//
//
//
EXTERN_C NTSTATUS cmdedr_popCustomData(UINT16 nType, void* pData, void* pOutBuffer, UINT64* pnBufferSize)
{
	return cmd::netmon::popCustomData((cmd::netmon::CustomDataType)nType, pData, pOutBuffer, pnBufferSize);
}

//
//
//
EXTERN_C void cmdedr_freeCustomData(UINT16 nType, void* pData, EndpointId nId)
{
	return cmd::netmon::freeCustomData((cmd::netmon::CustomDataType)nType, pData, nId);
}
