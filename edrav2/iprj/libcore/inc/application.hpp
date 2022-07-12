//
// EDRAv2.libcore project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (18.01.2019)
//
///
/// @file Application class declaration
///
/// @addtogroup application Application
/// Application object is intended to be the base of creating new executable files.
/// @{
#pragma once
#include "error.hpp"
#include "command.hpp"

namespace cmd {

// Fix for moxygen generator
namespace application {struct a{};}

class Application;

///
/// The interface for extending Application functional using working modes.
///
/// Any object can implement the methods and this interface and register this
/// interface in Application instance using the string identifier of mode. The 
/// Application will use this interface (and in turn the mode object) in case 
/// of specifying the string identifier for this mode on command line parameters. 
///
class IApplicationMode
{
public:

	///
	/// Main application mode function.
	///
	/// This function is called after application initialization and 
	/// it contains main logic of application in specified mode.
	///
	/// @param[in] "pApp" - the instance of application.
	/// @returns The error code which is used as an application's exit code.
	/// @throw All types of exceptions. These exceptions will be catched and converted into
	///		ErrorCode which is used as application exit code.
	///
	virtual ErrorCode main(Application* pApp) = 0;
	
	///
	/// Executes application commands.
	///
	/// This function is called then application received the unrecognized
	/// command. The working mode handler can extend the supported 
	/// set of application's commands.
	///
	/// @param[in] "pApp" - the instance of application.
	/// @param[in] "vCmd" - the command.
	/// @param[in] "vParams" - the command's parameters.
	/// @returns Command's result.
	/// @throw OperationNotSupported if the command is not supported.
	///
	virtual Variant execute(Application* /*pApp*/, Variant /*vCmd*/, Variant /*vParams*/)
	{
		error::OperationNotSupported(SL, "The object doesn't support any commands").throwException();
		return {};
	}

	///
	/// Function defines the parser for parameters.
	///
	/// The specified parameters parser is only filled by parameters
	/// which are specific for current working mode. For setup the parser please
	/// read this: https://github.com/catchorg/Clara.
	///
	/// @param[in] "paramParser" - the instance of parser for modification.
	/// @throw Different types exceptions. 
	///
	virtual void initParams(clara::Parser& /*paramParser*/)
	{}
};
// CLSID is declared here because Application class is available for inheritance
CMD_DECLARE_LIBRARY_CLSID(CLSID_Application, 0x380579B6);


///
/// Application base class.
///
/// This class provides all functionality for a standard COMODO application.
/// For customization, the end-user application shall use own application 
/// class which is derived from this Application and override common functions.
/// Additionally the working mode handlers (IApplicationMode interface) also 
/// can be used for behavior customization.
///
class Application : public ObjectBase<CLSID_Application>,
	public ICommandProcessor
{
private:
	std::map<std::string, std::shared_ptr<IApplicationMode>> m_appModes;
	
	bool m_fTerminate = false;
	std::string m_sCmdLine;
	std::mutex m_mtxTerminate;
	boost::condition_variable_any m_cvTerminate;

	bool m_fOutExceptionLog = true;

	ErrorCode run2(std::string_view sAppName, std::string_view sFullName, 
		int argc, const char* const argv[]);
	bool parseParameters(int argc, const char* const argv[]);
	bool doBasicStartup(std::string_view sAppName, std::string_view sFullName,
		int argc, const char* const argv[]);
	ErrorCode doBasicShutdown(ErrorCode errCode);
	
	//
	// Sleep specified time.
	//
	// If application is shutting down, throw exception.
	//
	// @param nDurationMs - time to sleep in milliseconds.
	// @throw ShutdownIsStarted if Application termination is started.
	//
	void Sleep(Size nDurationMs);

protected:

	//
	// These are parameters that can be overriden by command line
	// They use only for getting command line data and passing to the
	// config. Don't use it after initEnvironment() call.
	//
	struct
	{
		Size m_nLogLevel{c_nMaxSize};
		std::filesystem::path m_pathLogData;
		std::filesystem::path m_pathConfigFile;
		std::filesystem::path m_pathData;
		bool m_fSilentMode{ false };
	} m_cmdLineParams;

	std::string m_sAppMode;
	std::shared_ptr<IApplicationMode> m_pAppMode;

	///
	/// Initializes primary global catalog data.
	///
	/// Override it if you want to add some additional data on the 
	/// initialization stage.
	///
	/// @param fUseCommonData - (unused) 
	///
	virtual void initEnvironment(bool fUseCommonData);

	///
	/// Initializes log subsystem basing global catalog and command line data  
	///
	/// Override it if you want to customize the logs set and behaviour.
	///
	virtual void initLog();

	///
	/// Outputs header to the main log.
	///
	/// Override it if you want to customize the log header.
	///
	virtual void outputLogHeader();

	///
	/// Loads a configuration from primary config file into catalog.
	///
	/// Override it if you want to load config from other places and/or redefine 
	/// some base parameters of application.
	///
	virtual void loadConfig();
	
	///
	/// Processes configuration file then the object cataloc objects are available.
	///
	/// Override it if you want to initialize data from calculated  fields in config file.
	///
	virtual void processConfig();

	///
	/// Does a selfcheck according to configuration rules.
	///
	/// Override it if you want to chande/do additional selfcheck steps.
	///
	virtual void doSelfcheck();

	///
	/// Calls main() function of working mode handler.
	///
	/// This function can be overloaded for implementing additional behaviour.
	///
	/// @returns Application exit code.
	///
	virtual ErrorCode runMode();

	///
	/// Performs startup operations.
	///
	/// This function can be overloaded for implementing additional
	/// behaviour but overloaded implementation shouls call the original one. 
	///
	/// @returns The function returns false is returned if application shall be finished without following 
	///		initialization.
	///
	virtual bool doStartup();

	///
	/// Performs shutdown operations.
	///
	/// This function can be overloaded for implementing additional behaviour.
	///
	/// @param[in] "errCode" - application error code. 
	/// @return Application error code (can be redefined inside).
	///
	virtual ErrorCode doShutdown(ErrorCode errCode);

	///
	/// Function performs main application operations
	///
	/// This function calls the following sequence of functions:
	///   * doStartup()
	///   * runMode()
	///   * doShutdown()
	///
	/// @return Application error code 
	///
	virtual ErrorCode process();

public:

	//
	// Default mode name. It is used then the mode is not specified in command line
	//
	static inline const char c_sDefModeName[] = "default";

	///
	/// Startup application function.
	///
	/// This function shall be called from the main() C++ program function 
	/// passing command line arguments.
	///
	/// @param[in] "sName" - short identifier of the application. It is used during creation 
	///		or accessing different named objects (logs, configs, ...).
	/// @param[in] "sFullName" - human-readable description of application (phrase).
	/// @param[in] "argc/argv[]" - arguments of C++ main() function.
	/// @return Application exit code.
	///
	int run(std::string_view sName, std::string_view sFullName, int argc, const char* const argv[]);
	int run(std::string_view sName, std::string_view sFullName, int argc, const wchar_t* const argv[]);

	///
	/// Adds a new application mode to the application.
	///
	/// This function registers the intrface of new mode handler for specified mode.
	///
	/// @param[in] "sName" - mode name.
	/// @param[in] "pMode" - smart pointer into the mode handler interface.
	///
	void addMode(std::string sName, std::shared_ptr<IApplicationMode> pMode);

	///
	/// Waits shutdown() call.
	///
	/// Use this function in the working modes when you wait shutdown().
	///
	void waitShutdown();

	///
	/// Performs the application's shutdown.
	///
	/// This function starts shutdown process, sends messages about termination, 
	/// stops and delete services.
	///
	void shutdown();

	///
	/// Checks if application shall shutdown or not.
	///
	/// This function shall be checked in the main() function of every working mode.
	/// The main() function shall return value if this function returns false.
	///
	/// @return The function returns `true` if the application is terminating.
	///
	bool needShutdown();

	// ICommandProcessor
	virtual Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace cmd
/// @}