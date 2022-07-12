#ifndef PCH_H
#define PCH_H

// This file include should be first
#define OS_DEPENDENT_PCH
#include "extheaders.h"

#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <windows.h>
#include <versionhelpers.h>
// #include <atlbase.h>
// #include <atlsecurity.h>
#include <tlhelp32.h>
#include <sddl.h>
#include <winternl.h>
#include <Shlobj.h>
#include <wtsapi32.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include "wintrust_w8.h" // We need this to use in Win8+
#include <mscat.h>
#include <appmodel.h>
#include <appxpackaging.h>
#include <shlwapi.h>

#include <cwctype>
#include <locale>
#include <deque>
#include <unordered_set>

#include "utilities.hpp"
#include "dataproviders.hpp"

// Common EDR Agent external raw API files
#include <cmd/sys.hpp>
#include <cmd/sys/net.hpp>

#endif //PCH_H
