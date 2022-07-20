//
// EDRAv2.libsyswin project
//
// Windows service controller
//
// Author: Yury Podpruzhnikov (04.01.2019)
// Reviewer: 
//
#include "pch.h"
#include <libsysmon/inc/edrdrvapi.hpp>

using namespace cmd;

//
// It is not unit test.
// FIXME: What is it?
//
TEST_CASE("winsvc.simple", "[!hide][!mayfail]")
{
	logging::setRootLogLevel(logging::LogLevel::Detailed);
	try
	{
		LOGINF("isExist: " << execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "isExist"},
			{"params", Dictionary({
				{"name", "GoogleChromeElevationService"},
			})},
		})));

		(void)execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "start"},
			{"params", Dictionary({
				{"name", "GoogleChromeElevationService"},
			})},
		}));

		LOGINF("queryStatus: " << execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "queryStatus"},
			{"params", Dictionary({
				{"name", "GoogleChromeElevationService"},
			})},
		})));

		(void)execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "stop"},
			{"params", Dictionary({
				{"name", "GoogleChromeElevationService"},
			})},
		}));

		//execCommand(Dictionary({
		//	{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
		//	{"command", "waitStatus"},
		//	{"params", Dictionary({
		//		{"name", "GoogleChromeElevationService"},
		//		{"status", SERVICE_RUNNING},
		//		{"timeout", 2000},
		//	})},
		//}));

		(void)execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "waitState"},
			{"params", Dictionary({
				{"name", "GoogleChromeElevationService"},
				{"state", SERVICE_STOPPED},
				{"timeout", 2000},
			})},
		}));

	}
	catch (error::Exception& e)
	{
		e.log(SL);
		throw;
	}

	int i = 0;
}

//
// It is not unit test.
// FIXME: What is it?
//
TEST_CASE("winsvc.install_uninstall_configure", "[!hide][!mayfail]")
{
	logging::setRootLogLevel(logging::LogLevel::Detailed);
	try
	{
		LOGINF("install: " << execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "create"},
			{"params", Dictionary({
				{"name", "xxxxxx"},
				{"path", R"(c:\Users\Desil\source\comodo\edrav2\out\bin\win-Debug-Win32\cmdcon.exe)"},
				{"type", SERVICE_WIN32_OWN_PROCESS},
				{"startMode", SERVICE_DISABLED},
				{"errorControl", SERVICE_ERROR_NORMAL},
				{"displayName", "Display name of ZZZZZZZZZ"},
				{"group", "group_of_ZZZZZZZZZ"},
				//{"dependencies", Sequence({"GoogleChromeElevationService", "CoreMessagingRegistrar"})},
				{"dependencies", Sequence()},
				//{"reinstall", true},
			})},
		})));

		

		LOGINF("configure: " << execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "change"},
			{"params", Dictionary({
				{"name", "xxxxxx"},
				{"description", "Some description"},
				{"start", SERVICE_AUTO_START},
				{"restartIfCrash", Dictionary()},
				{"privileges", Sequence({"Priv1", "Priv2"})},
			})},
		})));

		LOGINF("uninstall: " << execCommand(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
			{"command", "delete"},
			{"params", Dictionary({
				{"name", "xxxxxx"},
			})},
		})));
	}
	catch (error::Exception& e)
	{
		e.log(SL);
		throw;
	}

	int i = 0;

}

