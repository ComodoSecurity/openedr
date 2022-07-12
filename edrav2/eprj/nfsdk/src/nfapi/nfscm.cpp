//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "StdAfx.h"

#define _NF_INTERNALS

#include "nfscm.h"
#include "nfapi.h"
#include <accctrl.h>
#include <aclapi.h>
#include <psapi.h>

using namespace nfapi;

extern DWORD	g_flags;

#define DriverDevicePrefix "\\\\.\\CtrlSM"

static void setDriverLoadingOrder(const char * driverName, bool removeDriverTag);
static bool disableTeredo(bool disable);
static bool disableTCPOffloading(bool disable);
static bool _DriverTag(bool query, const char * driverName, DWORD * pdwTag);

static bool serviceRegPathExists(const char * serviceName)
{
    HKEY	hkey;
    LONG	err;
	char	regPath[MAX_PATH];

	_snprintf(regPath, sizeof(regPath), "SYSTEM\\CurrentControlSet\\Services\\%s", serviceName);

	err = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        regPath,
        0,
        KEY_QUERY_VALUE,
        &hkey
       );

    if ( ERROR_SUCCESS != err )
    {
        return false;
    }

	RegCloseKey(hkey);

	return true;	
}


bool nfapi::nf_applyFlags(bool addChanges)
{
	disableTeredo(addChanges && !(g_flags & NFF_DONT_DISABLE_TEREDO));
	
	disableTCPOffloading(addChanges && !(g_flags & NFF_DONT_DISABLE_TCP_OFFLOADING));

	return true;
}

static NF_STATUS nf_registerDriverInternal(const char * driverName, const char * driverPath = NULL)
{
	bool result = false;
	wchar_t drvName[MAX_PATH];
	wchar_t drvPath[MAX_PATH];
	SC_HANDLE schSCM = NULL;
	SC_HANDLE schService = NULL;
	DWORD tag = 0;
	DWORD dwLastError = 0;

	_snwprintf(drvName, sizeof(drvName)/2, L"%S", driverName);

	if (driverPath)
	{
		_snwprintf(drvPath, sizeof(drvPath)/2, L"%S\\%S.sys", driverPath, driverName);
	} else
	{
		_snwprintf(drvPath, sizeof(drvPath)/2, L"system32\\drivers\\%S.sys", driverName);
	}

	schSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCM)
	{
		return NF_STATUS_FAIL;
	}

	schService = CreateServiceW(schSCM,
				drvName,
				drvName,
				SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER,
				SERVICE_SYSTEM_START,
				SERVICE_ERROR_NORMAL,
				drvPath,
				L"PNP_TDI",
				&tag,
				NULL,
				NULL,
				NULL);

	if (g_flags & NFF_DISABLE_AUTO_START)
	{
		if (schService != NULL)
		{
			CloseServiceHandle(schService);
			result = true;
		} else
		{
			dwLastError = GetLastError();
			if (dwLastError == ERROR_SERVICE_EXISTS)
			{
				result = true;
			}
		}
	} else
	{
		if (schService != NULL)
		{
			if (StartService(schService, 0, NULL))
			{
				result = true;
			}

			dwLastError = GetLastError();

			CloseServiceHandle(schService);
		} else
		{
			dwLastError = GetLastError();

			if (dwLastError == ERROR_SERVICE_EXISTS)
			{
				schService = OpenServiceA(schSCM, driverName, SERVICE_START | SERVICE_QUERY_STATUS);
				if (schService != NULL)
				{
					SERVICE_STATUS ss;

					if (QueryServiceStatus(schService, &ss))
					{
						if (ss.dwCurrentState == SERVICE_RUNNING)
						{
							result = true;
						}
					}

					if (!result)
					if (StartService(schService, 0, NULL))
					{
						result = true;
					}

					dwLastError = GetLastError();

					CloseServiceHandle(schService);
				} else
				{
					dwLastError = GetLastError();
				}
			}
		}
	} 

	CloseServiceHandle(schSCM);

//	if (result)
	{
		setDriverLoadingOrder(driverName, false);
	}

	SetLastError(dwLastError);

	return result? NF_STATUS_SUCCESS : NF_STATUS_FAIL;
}

static bool setDriverGroup(const char * driverName, bool isWFP)
{
    HKEY	hkey;
    LONG	err;
	char	regPath[MAX_PATH];
	const char szWFPGroup[] = "NDIS";
	const char mszWFPDepend[] = "tcpip\0";
	const char szTDIGroup[] = "PNP_TDI";

	_snprintf(regPath, sizeof(regPath), "SYSTEM\\CurrentControlSet\\Services\\%s", driverName);

	err = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        regPath,
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hkey
       );

    if ( ERROR_SUCCESS != err )
    {
        return false;
    }

	if (isWFP)
	{
		err = RegSetValueExA(
					hkey,
					"Group",
					NULL,
					REG_SZ,
					(LPBYTE)szWFPGroup,
					(DWORD)sizeof(szWFPGroup)
				   );

		err = RegSetValueExA(
					hkey,
					"DependOnService",
					NULL,
					REG_MULTI_SZ,
					(LPBYTE)mszWFPDepend,
					(DWORD)sizeof(mszWFPDepend)
				   );
	} else
	{
		err = RegSetValueExA(
					hkey,
					"Group",
					NULL,
					REG_SZ,
					(LPBYTE)szTDIGroup,
					(DWORD)sizeof(szTDIGroup)
				   );
	}

	RegCloseKey(hkey);

	return ERROR_SUCCESS == err;
}


HANDLE nfapi::nf_openDevice(const char * driverName)
{
	char	deviceName[MAX_PATH];
	HANDLE	hDevice = NULL;
	int		step = 0;
	DWORD	dwLastError = 0;

	_snprintf(deviceName, sizeof(deviceName), "%s%s", DriverDevicePrefix, driverName);

	for (;;)
	{
		hDevice = CreateFileA(deviceName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);

		dwLastError = GetLastError();

		if (g_flags & NFF_DISABLE_AUTO_REGISTER)
			break;

		if (hDevice == INVALID_HANDLE_VALUE)
		{
			step++;

			switch (step)
			{
			case 1:
				nf_registerDriverInternal(driverName);
				continue;
			}
		}

		break;
	}	

	// EDR_FIX: Remove not compiled code for TDI
	/*
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		OSVERSIONINFOEXA osvi;
		ZeroMemory(&osvi, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionExA((LPOSVERSIONINFOA)&osvi);
		
		bool isWFP = false;

		if (hDevice != INVALID_HANDLE_VALUE)
		{
			DWORD	dwBytesReturned;
			ULONG	driverType = 0;

			if (DeviceIoControl(hDevice,
				NF_REQ_GET_DRIVER_TYPE,
				(LPVOID)NULL, 0,
				(LPVOID)&driverType, sizeof(driverType),
				&dwBytesReturned, NULL) && (dwBytesReturned > 0))
			{
				isWFP = true;
			}
		}

		if (isWFP)
		{
			// Set NDIS driver group and dependency from TCPIP for WFP driver 
			// when Avast is installed, for correct loading order
			if (serviceRegPathExists("aswstm"))
			{
				setDriverGroup(driverName, TRUE);
			}
		} else
		// Disable default loading order for TDI driver on Windows 7 and lower 
		if ((osvi.dwMajorVersion < 6) ||
			(osvi.dwMajorVersion == 6 && osvi.dwMinorVersion <= 1))
		{
//			if (!isWFP)
			{
				DWORD dwTag = 0;
	
				if (!serviceRegPathExists("nisdrv") &&
					!serviceRegPathExists("360netmon") &&
					!serviceRegPathExists("symnets"))
				{
					_DriverTag(false, driverName, &dwTag);
				} else
				{
					if (_DriverTag(true, driverName, &dwTag) && (dwTag == 0))
					{
						setDriverLoadingOrder(driverName, false);
					}
				}

				if (serviceRegPathExists("aswstm"))
				{
					setDriverGroup(driverName, FALSE);
				}
			}
		}
	}
	*/
	SetLastError(dwLastError);

	return hDevice;
}

NFAPI_API NF_STATUS NFAPI_NS nf_registerDriver(const char * driverName)
{
	NF_STATUS status = nf_registerDriverInternal(driverName);

	if (status == NF_STATUS_SUCCESS)
	{
		if (!nf_applyFlags(true))
		{
			return NF_STATUS_REBOOT_REQUIRED;
		}
	}

	return status;
}

NFAPI_API NF_STATUS NFAPI_NS nf_registerDriverEx(const char * driverName, const char * driverPath)
{
	NF_STATUS status = nf_registerDriverInternal(driverName, driverPath);

	if (status == NF_STATUS_SUCCESS)
	{
		if (!nf_applyFlags(true))
		{
			return NF_STATUS_REBOOT_REQUIRED;
		}
	}

	return status;
}

NFAPI_API NF_STATUS NFAPI_NS nf_unRegisterDriver(const char * driverName)
{
	bool bResult = false;
	SC_HANDLE schSCM = NULL;
	DWORD	dwLastError = 0;
	
	setDriverLoadingOrder(driverName, true);

	schSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCM)
	{
		SC_HANDLE schService = OpenServiceA(schSCM, driverName, DELETE);
		if (schService != NULL)
		{
			DeleteService(schService);
	
			dwLastError = GetLastError();

			CloseServiceHandle(schService);

			bResult = true;
		} else
		{
			dwLastError = GetLastError();
		}

		CloseServiceHandle(schSCM);
	} else
	{
		dwLastError = GetLastError();
	}

	SetLastError(dwLastError);

	if (!nf_applyFlags(false))
	{
		return NF_STATUS_REBOOT_REQUIRED;
	}

	return bResult? NF_STATUS_SUCCESS : NF_STATUS_FAIL;
}

static bool _DriverTag(bool query, const char * driverName, DWORD * pdwTag)
{
    HKEY	hkey;
    LONG	err;
	char	regPath[MAX_PATH];
    DWORD	dwType;
    DWORD	dwSize;

	_snprintf(regPath, sizeof(regPath), "SYSTEM\\CurrentControlSet\\Services\\%s", driverName);

	err = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        regPath,
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hkey
       );

    if ( ERROR_SUCCESS != err )
    {
        return false;
    }

	if (query)
	{
		dwSize = sizeof(DWORD);

		err = RegQueryValueExA(
				hkey,
				"Tag",
				NULL,
				&dwType,
				(LPBYTE)pdwTag,
				&dwSize
			   );
	} else
	{
		err = RegSetValueExA(
					hkey,
					"Tag",
					NULL,
					REG_DWORD,
					(LPBYTE)pdwTag,
					(DWORD)sizeof(DWORD)
				   );
	}

	RegCloseKey(hkey);

	return ERROR_SUCCESS == err;
}

static bool _GroupOrderList(bool query, DWORD ** ppdwList, DWORD * pdwSize)
{
    HKEY	hkey;
    LONG	err;
    DWORD	dwType;

	err = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\GroupOrderList",
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hkey
       );

    if ( ERROR_SUCCESS != err )
    {
        return false;
    }

	if (query)
	{
		err = RegQueryValueExA(
				hkey,
				"PNP_TDI",
				NULL,
				&dwType,
				NULL,
				pdwSize
			   );

		if (ERROR_SUCCESS != err)
		{
			RegCloseKey(hkey);
			return false;
		}

		*ppdwList = (DWORD*)malloc(*pdwSize + sizeof(DWORD));
		if (!*ppdwList)
		{
			RegCloseKey(hkey);
			return false;
		}

		err = RegQueryValueExA(
				hkey,
				"PNP_TDI",
				NULL,
				&dwType,
				(LPBYTE)*ppdwList,
				pdwSize
			   );

		if (ERROR_SUCCESS != err)
		{
			RegCloseKey(hkey);
			free(*ppdwList);
			*ppdwList = NULL;
			return false;
		}
	} else
	{
		DWORD newSize = (*pdwSize > sizeof(DWORD))? (*pdwSize / sizeof(DWORD))-1 : 0;
		(*ppdwList)[0] = newSize;

		err = RegSetValueExA(
					hkey,
					"PNP_TDI",
					NULL,
					REG_BINARY,
					(LPBYTE)*ppdwList,
					*pdwSize
				   );
	}

	RegCloseKey(hkey);

	return ERROR_SUCCESS == err;
}

/**
*	Update the loading order in PNP_TDI group to allow the driver 
*	start immediately after TDI provider, before other TDI filters.
**/
static void setDriverLoadingOrder(const char * driverName, bool removeDriverTag)
{
	DWORD	*pdwList = NULL;
    DWORD	dwSize;
	DWORD	dwListSize;
	bool	updateGOL = true;
	DWORD	i;
	DWORD	dwTag = 0;
	DWORD	tagIndex = -1;
	DWORD	dwTdiTag = 0;
	DWORD	tdiTagIndex = -1;
	
	// Get Tag value from driver regkey
	if (!_DriverTag(true, driverName, &dwTag))
		return;

	// Load GroupOrderList
	if (!_GroupOrderList(true, &pdwList, &dwSize))
		return;

	dwListSize = dwSize / sizeof(DWORD);

	if (removeDriverTag)
	{
		// Delete tag from GroupOrderList
		if (dwTag != 0)
		{
			// Find tag value in GroupOrderList
			for (i=1; i<dwListSize; i++)
			{
				if (pdwList[i] == dwTag)
				{
					tagIndex = i;
					break;
				}
			}

			if (tagIndex != -1)
			{
				memmove(pdwList+tagIndex, pdwList+tagIndex+1, (dwListSize - tagIndex - 1) * sizeof(DWORD));
				dwSize -= sizeof(DWORD);
				_GroupOrderList(false, &pdwList, &dwSize);
			}
		}

		if (pdwList)
		{
			free(pdwList);
		}
		
		return;
	}

	if (dwTag == 0)
	{
		// Assign a unique tag to driver 
		for (i=1; i<dwListSize; i++)
		{
			if (pdwList[i] >= dwTag)
				dwTag = pdwList[i] + 1;
		}
		
		// Set Tag value
		_DriverTag(false, driverName, &dwTag);
	}

	// Find tag value in GroupOrderList
	for (i=1; i<dwListSize; i++)
	{
		if (pdwList[i] == dwTag)
		{
			tagIndex = i;
			break;
		}
	}

	//
	// Update GroupOrderList
	//

	// Get TDI provider driver tag

	// Windows Vista/7
	if (!_DriverTag(true, "Tdx", &dwTdiTag))
	{
		// Windows NT/2000/XP
		_DriverTag(true, "Tcpip", &dwTdiTag);
	}

	// Find TDI provider driver tag value in GroupOrderList
	if (dwTdiTag > 0)
	{
		for (i=1; i<dwListSize; i++)
		{
			if (pdwList[i] == dwTdiTag)
			{
				tdiTagIndex = i;
				break;
			}
		}
	}

	if (tagIndex != -1)
	{
		// Tag already exists in GroupOrderList
		if (tdiTagIndex != -1)
		{
			// Move tag to a position next to tdiTagIndex if it is not there
			if ((tdiTagIndex < tagIndex) && (tagIndex - tdiTagIndex) > 1)
			{
				memmove(pdwList+tdiTagIndex+2, pdwList+tdiTagIndex+1, (tagIndex - tdiTagIndex - 1) * sizeof(DWORD));
				pdwList[tdiTagIndex+1] = dwTag;
			} else
			{
				updateGOL = false;
			}
		}
	} else
	{
		// Insert tag to a position next to tdiTagIndex
		if (tdiTagIndex != -1)
		{
			memmove(pdwList+tdiTagIndex+2, pdwList+tdiTagIndex+1, (dwListSize - tdiTagIndex - 1) * sizeof(DWORD));
			pdwList[tdiTagIndex+1] = dwTag;
		} else
		{
			pdwList[dwListSize] = dwTag;
		}

		dwSize += sizeof(DWORD);
	}

	if (updateGOL)
	{
		_GroupOrderList(false, &pdwList, &dwSize);
	}

	if (pdwList)
	{
		free(pdwList);
	}
}

static bool disableTeredo(bool disable)
{
    HKEY	hkey;
    LONG	err;
	DWORD	dwType, dwSize;
    DWORD	dwValue;

	err = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters",
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hkey
       );

    if ( ERROR_SUCCESS != err )
    {
        return false;
    }

	dwSize = sizeof(DWORD);

	err = RegQueryValueExA(
			hkey,
			"DisabledComponents",
			NULL,
			&dwType,
			(LPBYTE)&dwValue,
			&dwSize
		   );

	if (ERROR_SUCCESS != err || dwSize == 0)
	{
		dwValue = 0;
	}

	dwValue = disable? (dwValue | 0x8) : (dwValue & (~0x8));

	err = RegSetValueExA(
					hkey,
					"DisabledComponents",
					NULL,
					REG_DWORD,
					(LPBYTE)&dwValue,
					sizeof(DWORD)
				   );

	RegCloseKey(hkey);

	return ERROR_SUCCESS == err;
}

static bool disableTCPOffloading(bool disable)
{
    HKEY	hkey;
    LONG	err;
    DWORD	dwValue;

	err = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
        0,
        KEY_SET_VALUE,
        &hkey
       );

    if ( ERROR_SUCCESS != err )
    {
        return false;
    }

	dwValue = disable? 1 : 0;

	err = RegSetValueExA(
					hkey,
					"DisableTaskOffload",
					NULL,
					REG_DWORD,
					(LPBYTE)&dwValue,
					sizeof(DWORD)
				   );

	RegCloseKey(hkey);

	return ERROR_SUCCESS == err;
}
