//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _DEVCTRL_H
#define _DEVCTRL_H

#include "hashtable.h"
#include "tcpctx.h"
#include "udpctx.h"
#include "ipctx.h"

#define PEND_LIMIT	(4 * NF_TCP_PACKET_BUF_SIZE)
#define TCP_RECV_INJECT_LIMIT	(8 * 1024 * 1024)
#define TCP_SEND_INJECT_LIMIT	(256 * 1024)
#define UDP_PEND_LIMIT	(100 * NF_TCP_PACKET_BUF_SIZE)

typedef enum _NF_DISABLED_CALLOUTS
{
	NDC_NONE = 0,
	NDC_TCP = 1,
	NDC_UDP = 2,
	NDC_IP = 4,
	NDC_BIND = 8,
} NF_DISABLED_CALLOUTS;

NTSTATUS devctrl_init(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
void	 devctrl_free();
DRIVER_DISPATCH devctrl_dispatch;
NTSTATUS devctrl_dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
BOOLEAN	 devctrl_isProxyAttached();
ULONG	 devctrl_getProxyPid();
NTSTATUS devctrl_pushTcpData(HASH_ID id, int code, PTCPCTX pTcpCtx, PNF_PACKET pPacket);
NTSTATUS devctrl_pushUdpData(HASH_ID id, int code, PUDPCTX pUdpCtx, PNF_UDP_PACKET pPacket);
PDEVICE_OBJECT devctrl_getDeviceObject();
HANDLE	devctrl_getInjectionHandle();
HANDLE	devctrl_getUdpInjectionHandle();
HANDLE	devctrl_getUdpNwV4InjectionHandle();
HANDLE	devctrl_getUdpNwV6InjectionHandle();
HANDLE	devctrl_getNwV4InjectionHandle();
HANDLE	devctrl_getNwV6InjectionHandle();
void	devctrl_setShutdown();
BOOLEAN	devctrl_isShutdown();
NTSTATUS devctrl_pushTcpInject(PTCPCTX pTcpCtx);
NTSTATUS devctrl_pushIPData(int code);
DWORD devctrl_getDisabledCallouts();
void devctrl_sleep(UINT ttw);

#endif