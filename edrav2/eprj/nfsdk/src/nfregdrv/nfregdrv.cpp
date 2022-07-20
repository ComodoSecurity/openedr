#include <windows.h>
#include "nfapi.h"

#ifndef _C_API
using namespace nfapi;
#endif

LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

void usage()
{
	MessageBox(NULL, 
		"Usage:\r\n"
		"  nfregdrv.exe <driver_name> - register windows\\system32\\drivers\\<driver_name>.sys\r\n"
		"  nfregdrv.exe -u <driver_name> - unregister windows\\system32\\drivers\\<driver_name>.sys\r\n",
		"nfregdvr",
		MB_OK);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    TCHAR szTokens[] = "-/";
	bool bRegister = true;

    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    if (lpszToken != NULL)
    {
        if (*lpszToken == 'u')
            bRegister = false;

		lpszToken = FindOneOf(lpszToken, " ");
    } else
	{
		lpszToken = lpCmdLine;
	}

	if (!lpszToken)
	{
		usage();
		return -1;
	}

	if (!*lpszToken)
	{
		usage();
		return -1;
	}

	if (bRegister)
	{
		nf_registerDriver(lpszToken);
	} else
	{
		nf_unRegisterDriver(lpszToken);
	}

	return 0;
}

