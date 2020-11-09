//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer:
//
// Saving process dump functional unit tests
//
#include "pch.h"
#include <dbghelp.h>
#include <error/dump/minidumpwriter.hpp>

#pragma comment(lib, "client.lib")
#pragma comment(lib, "util.lib")
#pragma comment(lib, "base.lib")

using namespace openEdr;

//
// CBase and CDerived classes are used when checking handling of pure virtual calls.
//

class CDerived;
class CBase
{
public:
	CBase(CDerived *derived) : m_pDerived(derived) {};
	~CBase();
	virtual void function(void) = 0;

	CDerived * m_pDerived;
};

class CDerived : public CBase
{
public:
	CDerived() : CBase(this) {};   // C4355
	virtual void function(void) {};
};

CBase::~CBase()
{
	m_pDerived->function();
}


#if defined(_WIN32)

//
//
//
std::wstring getCurrentDir()
{
	wchar_t buf[0x1000];
	DWORD dwSize = GetModuleFileNameW(NULL, buf, sizeof(buf) / sizeof(buf[0]));
	if (dwSize == 0)
		error::win::WinApiError(SL).throwException();

	std::wstring sModuleFullName(buf);
	auto const nPos = sModuleFullName.find_last_of(L"\\/");
	return (std::wstring::npos != nPos) ? sModuleFullName.substr(0, nPos) : L".";
}

//
// Test for manual saving of minidumps using MinidumpWriter.
//
// Test perform the following actions:
//  - creates of an instance of MinidumpWriter;
//  - manually calls save() method for saving minidump of current 
//    process on disk.
//
TEST_CASE("MinidumpWriter.saveManual", "[!hide][!throws][!mayfail]")
{
	SECTION("defaultParameters")
	{
		auto fn = [&]()
		{
			error::dump::win::detail::MinidumpWriter::getInstance().save();
		};
		REQUIRE_NOTHROW(fn());
	}

	SECTION("manualParameters")
	{
		auto fn = [&]()
		{
			Variant vConfig = Dictionary({
				{ "prefix", "testdump" },
				{ "dumpDirectory", getCurrentDir() + L"\\manualdump"}
				});
			error::dump::win::detail::MinidumpWriter::getInstance().init(vConfig);
			error::dump::win::detail::MinidumpWriter::getInstance().save();
		};
		REQUIRE_NOTHROW(fn());
	}
}

//
// Test for SEH handling using MinidumpWriter.
//
// Test perform the following actions:
//  - initializes of crash handlers for using MinidumpWriter.
//  - simulates crash via attempt to free unallocated memory.
//
TEST_CASE("MinidumpWriter.handleUnhandledException", "[!hide][!throws][!mayfail]")
{
	error::dump::win::initCrashHandlers();

	// Do crash
#pragma warning(push)
#pragma warning(disable: 4312)
	delete reinterpret_cast<char*>(0xFEE1DEAD);
#pragma warning(pop)
}

//
// Test for pure virtual calls handling using MinidumpWriter.
//
// Test perform the following actions:
//  - initializes of crash handlers for using MinidumpWriter.
//  - simulates crash using two classes (base and derived) to call
//    a pure virtual function in the descructor of base class.
TEST_CASE("MinidumpWriter.handlePureCall", "[!hide][!throws][!mayfail]")
{
	error::dump::win::initCrashHandlers();

	// Do crash
	CDerived myDerived;
}

#endif // _WIN32

const std::wstring g_sCrashpadHandlerPath = L".\\crashpad\\bin\\crashpad_handler.exe";
const std::wstring g_sCrashpadDBPath = L".\\crashpad\\db";

//
// Test for SEH handling using the Crashpad.
//
// Test perform the following actions:
//  - initializes of crash handlers for using the Crashpad.
//  - simulates crash via attempt to free unallocated memory.
//
TEST_CASE("Crashpad.handleUnhandledException", "[!hide][!throws][!mayfail]")
{
	bool fResult = error::dump::crashpad_::initCrashHandlers(g_sCrashpadHandlerPath, g_sCrashpadDBPath);
	if (!fResult)
		FAIL("Can't initialize crash handlers");

	// Do crash
#pragma warning(push)
#pragma warning(disable: 4312)
	delete reinterpret_cast<char*>(0xFEE1DEAD);
#pragma warning(pop)
}

//
//void myPurecallHandler(void)
//{
//	printf("In _purecall_handler.");
//	exit(0);
//}

//
// Test for pure virtual calls handling using the Crashpad.
//
// Test perform the following actions:
//  - initializes of crash handlers for using the Crashpad.
//  - simulates crash using two classes (base and derived) to call
//    a pure virtual function in the descructor of base class.
TEST_CASE("Crashpad.handlePureCall", "[!hide][!throws][!mayfail]")
{
	bool fResult = error::dump::crashpad_::initCrashHandlers(g_sCrashpadHandlerPath, g_sCrashpadDBPath);
	if (!fResult)
		FAIL("Can't initialize crash handlers");

	// Do crash
	//::_set_purecall_handler(myPurecallHandler);
	CDerived myDerived;
}
