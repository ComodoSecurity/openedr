//
// EDRAv2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer:
//
///
/// @file Crash handling implementation (via Crashpad)
///
#include "pch.h"

namespace cmd {
namespace error {
namespace dump {
namespace crashpad_ {

// @ermakov FIXME: Disable crashpad compilation

#if defined(_MSC_VER)

//
// Handles a pure virtual call errors.
//
void handlePureCall(void)
{
	CRASHPAD_SIMULATE_CRASH();
}

#endif // _MSC_VER

//
// Initializes Crashpad's client-side handlers.
//
// TODO: Singleton ?
// TODO: Thread-safe ?
bool initCrashHandlers(const std::wstring& sHandlerPath, const std::wstring& sDBPath)
{
	// Cache directory that will store crashpad information and minidumps
	base::FilePath db(sDBPath);
	// Path to the out-of-process handler executable
	base::FilePath handler(sHandlerPath);

	// URL used to submit minidumps to
	//std::string url("https://sentry.io/api/<project>/minidump/?sentry_key=<key>");

	// Optional annotations passed via --annotations to the handler
	std::map<std::string, std::string> annotations;
	annotations["format"] = "minidump";

	// Optional arguments to pass to the handler
	std::vector<std::string> arguments;
	// FIXME: This line should be removed in build for production environment.
	arguments.push_back("--no-rate-limit");

	std::unique_ptr<::crashpad::CrashReportDatabase> database = ::crashpad::CrashReportDatabase::Initialize(db);

	if ((database == nullptr) || (database->GetSettings() == NULL))
		return false;

	// Enable automated uploads
	// database->GetSettings()->SetUploadsEnabled(true);

	::crashpad::CrashpadClient client;
	bool success = client.StartHandler(
		handler,
		db,
		db,
		"",				// url,
		annotations,
		arguments,
		true,			// restartable
		true			// asynchronous_start
	);

	if (success)
	{
		// (Optional) Wait for Crashpad to initialize
		success = client.WaitForHandlerStart(INFINITE);

#if defined(_MSC_VER)
		::_set_purecall_handler(handlePureCall);
#endif // _MSC_VER
	}

	return success;
}

} // namespace crashpad_
} // namespace dump
} // namespace error
} // namespace cmd
