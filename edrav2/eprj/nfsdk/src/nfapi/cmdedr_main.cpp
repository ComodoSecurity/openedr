//
// Comodo EDR extension of NetFilter SDK
//

#include "stdafx.h"
#include "stdinc.h"
#include "nfapi.h"
#include "nfdriver.h"
#include "../include/cmdedr_extapi.h"

namespace nfapi {
namespace cmdedr {

//
// CustomEventHandler storage
//
static CustomEventHandler* g_pCustomEventHandler = nullptr;

//
//
//
void setCustomEventHandler(CustomEventHandler* pCustomEventHandler)
{
	g_pCustomEventHandler = pCustomEventHandler;
}

//
// Process custom data from driver
//
void processCustomData(nfapi::NF_DATA* pData)
{
	if (g_pCustomEventHandler == nullptr)
		return;

	CustomDataType eType = (CustomDataType)(UINT16)(pData->code & 0xFFFF);

	switch (eType)
	{
	case CustomDataType::ListenInfo:
		if (pData->bufferSize != sizeof(ListenInfo))
			return;
		g_pCustomEventHandler->tcpListened(reinterpret_cast<const ListenInfo*>((const void*)pData->buffer));
		break;
	default:
		break;
	}
}

} // namespace cmdedr
} // namespace nfapi
