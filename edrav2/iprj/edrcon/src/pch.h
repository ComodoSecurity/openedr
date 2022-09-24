// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

#ifndef PCH_H
#define PCH_H

#include <libcore/inc/libcore.h>
#include <libprocmon/inc/libprocmon.h>
#include <libsysmon/inc/libsysmon.h>
//#include <libnetmon/inc/libnetmon.h>
#include <libsyswin/inc/libsyswin.h>
#include <libcloud/inc/libcloud.h>
#include <libedr/inc/libedr.h>

// TODO: add headers that you want to pre-compile here
#include <iostream>
#include <conio.h>
#include <cli/clifilesession.h>
#include <cli/cli.h>

#if defined(FEATURE_ENABLE_MADCHOOK)
#pragma comment(lib, "madchookdrv.lib");
#endif

#endif //PCH_H
