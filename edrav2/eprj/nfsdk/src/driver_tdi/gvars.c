//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "gvars.h"
#include "ctrlio.h"
#include "tcpconn.h"
#include "addr.h"

#define DEFAULT_HASH_SIZE 3019

#define NF_TCP_FILTER_DEVICE	L"\\Device\\Tcp1Flt"
#define NF_TCP6_FILTER_DEVICE	L"\\Device\\Tcp61Flt"
#define NF_UDP_FILTER_DEVICE	L"\\Device\\Udp1Flt"
#define NF_UDP6_FILTER_DEVICE	L"\\Device\\Udp61Flt"

#define NF_CTRL_DEVICE			L"\\Device\\Ctrl"
#define NF_CTRL_DEVICE_LINK		L"\\DosDevices\\Ctrl"
#define NF_CTRL_DEVICE_PF		L"SM"

GVARS g_vars;

void free_gvars()
{
	if (g_vars.phtConnByFileObject)
	{
		hash_table_free(g_vars.phtConnByFileObject);
		g_vars.phtConnByFileObject = NULL;
	}

	if (g_vars.phtConnByContext)
	{
		hash_table_free(g_vars.phtConnByContext);
		g_vars.phtConnByContext = NULL;
	}

	if (g_vars.phtConnById)
	{
		hash_table_free(g_vars.phtConnById);
		g_vars.phtConnById = NULL;
	}

	if (g_vars.phtAddrByFileObject)
	{
		hash_table_free(g_vars.phtAddrByFileObject);
		g_vars.phtAddrByFileObject = NULL;
	}

	if (g_vars.phtAddrById)
	{
		hash_table_free(g_vars.phtAddrById);
		g_vars.phtAddrById = NULL;
	}
}

NTSTATUS init_gvars(PUNICODE_STRING RegistryPath)
{
	wchar_t wszDriverName[MAX_PATH] = { L'\0' };
	int i, driverNameLen = 0;

	// Zero g_vars
	RtlZeroMemory(&g_vars, sizeof(g_vars));

	// Get driver name from registry path
	for (i = RegistryPath->Length / sizeof(wchar_t) - 1; i >= 0; i--)
	{
		if (RegistryPath->Buffer[i] == L'\\' ||
			RegistryPath->Buffer[i] == L'/')
		{
			i++;

			while (RegistryPath->Buffer[i] && 
				(RegistryPath->Buffer[i] != L'.') &&
				(driverNameLen < MAX_PATH))
			{
				wszDriverName[driverNameLen] = RegistryPath->Buffer[i];
				i++;
				driverNameLen++;
			}

			wszDriverName[driverNameLen] = L'\0';

			break;
		}
	}

	if (driverNameLen == 0)
		return STATUS_UNSUCCESSFUL;

	// Initialize TCP filter device name
	g_vars.TCPFilterDeviceName.Buffer = g_vars.wszTCPFilterDeviceName;
	g_vars.TCPFilterDeviceName.Length = 0;
	g_vars.TCPFilterDeviceName.MaximumLength = sizeof(g_vars.wszTCPFilterDeviceName);
	RtlAppendUnicodeToString(&g_vars.TCPFilterDeviceName, NF_TCP_FILTER_DEVICE);
	RtlAppendUnicodeToString(&g_vars.TCPFilterDeviceName, wszDriverName);
	
	// Initialize TCP6 filter device name
	g_vars.TCP6FilterDeviceName.Buffer = g_vars.wszTCP6FilterDeviceName;
	g_vars.TCP6FilterDeviceName.Length = 0;
	g_vars.TCP6FilterDeviceName.MaximumLength = sizeof(g_vars.wszTCP6FilterDeviceName);
	RtlAppendUnicodeToString(&g_vars.TCP6FilterDeviceName, NF_TCP6_FILTER_DEVICE);
	RtlAppendUnicodeToString(&g_vars.TCP6FilterDeviceName, wszDriverName);

	// Initialize UDP filter device name
	g_vars.UDPFilterDeviceName.Buffer = g_vars.wszUDPFilterDeviceName;
	g_vars.UDPFilterDeviceName.Length = 0;
	g_vars.UDPFilterDeviceName.MaximumLength = sizeof(g_vars.wszUDPFilterDeviceName);
	RtlAppendUnicodeToString(&g_vars.UDPFilterDeviceName, NF_UDP_FILTER_DEVICE);
	RtlAppendUnicodeToString(&g_vars.UDPFilterDeviceName, wszDriverName);
	
	// Initialize UDP6 filter device name
	g_vars.UDP6FilterDeviceName.Buffer = g_vars.wszUDP6FilterDeviceName;
	g_vars.UDP6FilterDeviceName.Length = 0;
	g_vars.UDP6FilterDeviceName.MaximumLength = sizeof(g_vars.wszUDP6FilterDeviceName);
	RtlAppendUnicodeToString(&g_vars.UDP6FilterDeviceName, NF_UDP6_FILTER_DEVICE);
	RtlAppendUnicodeToString(&g_vars.UDP6FilterDeviceName, wszDriverName);

	// Initialize control device name
	g_vars.ctrlDeviceName.Buffer = g_vars.wszCtrlDeviceName;
	g_vars.ctrlDeviceName.Length = 0;
	g_vars.ctrlDeviceName.MaximumLength = sizeof(g_vars.wszCtrlDeviceName);
	RtlAppendUnicodeToString(&g_vars.ctrlDeviceName, NF_CTRL_DEVICE);
	RtlAppendUnicodeToString(&g_vars.ctrlDeviceName, NF_CTRL_DEVICE_PF);
	RtlAppendUnicodeToString(&g_vars.ctrlDeviceName, wszDriverName);

	// Initialize control link device name
	g_vars.ctrlDeviceLinkName.Buffer = g_vars.wszCtrlDeviceLinkName;
	g_vars.ctrlDeviceLinkName.Length = 0;
	g_vars.ctrlDeviceLinkName.MaximumLength = sizeof(g_vars.wszCtrlDeviceLinkName);
	RtlAppendUnicodeToString(&g_vars.ctrlDeviceLinkName, NF_CTRL_DEVICE_LINK);
	RtlAppendUnicodeToString(&g_vars.ctrlDeviceLinkName, NF_CTRL_DEVICE_PF);
	RtlAppendUnicodeToString(&g_vars.ctrlDeviceLinkName, wszDriverName);


	g_vars.phtConnByFileObject = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_vars.phtConnByFileObject)
	{
		return STATUS_UNSUCCESSFUL;
	}

	g_vars.phtConnByContext = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_vars.phtConnByContext)
	{
		free_gvars();
		return STATUS_UNSUCCESSFUL;
	}

	g_vars.phtConnById = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_vars.phtConnById)
	{
		free_gvars();
		return STATUS_UNSUCCESSFUL;
	}

	g_vars.phtAddrByFileObject = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_vars.phtAddrByFileObject)
	{
		free_gvars();
		return STATUS_UNSUCCESSFUL;
	}

	g_vars.phtAddrById = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_vars.phtAddrById)
	{
		free_gvars();
		return STATUS_UNSUCCESSFUL;
	}

	KeInitializeSpinLock(&g_vars.slConnections);

	InitializeListHead(&g_vars.lConnections);
	InitializeListHead(&g_vars.lAddresses);

	InitializeListHead(&g_vars.lRules);
	InitializeListHead(&g_vars.lTempRules);

	KeInitializeSpinLock(&g_vars.slRules);

	InitializeListHead(&g_vars.ioQueue);
	InitializeListHead(&g_vars.pendedIoRequests);
	KeInitializeSpinLock(&g_vars.slIoQueue);

	InitializeListHead(&g_vars.pendedIrpsToComplete);
	KeInitializeSpinLock(&g_vars.slPendedIrps);

	KeInitializeDpc(&g_vars.completeIoDpc, ctrl_completeIoDpc, NULL);
	KeInitializeTimer(&g_vars.completeIoDpcTimer);

	InitializeListHead(&g_vars.tcpQueue);
	KeInitializeSpinLock(&g_vars.slTcpQueue);
	KeInitializeDpc(&g_vars.tcpDpc, tcp_procDpc, NULL);

	InitializeListHead(&g_vars.udpQueue);
	KeInitializeSpinLock(&g_vars.slUdpQueue);
	KeInitializeDpc(&g_vars.udpDpc, udp_procDpc, NULL);

	KeInitializeDpc(&g_vars.queueDpc, ctrl_completePendedIoRequestDpc, NULL);

	g_vars.proxyAttached = FALSE;

	return STATUS_SUCCESS;
}