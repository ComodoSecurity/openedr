//
// EDRAv2.cmdcon project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (18.01.2019)
//
///
/// @file Default mode handler for cmdcon
///
#include "pch.h"

namespace cmd {

//
//
//
bool checkServiceIsRunning(std::string_view sServiceName)
{
	// Check the service is exist
	Variant vIsExist = execCommand(createObject(CLSID_WinServiceController), "isExist",
		Dictionary({ { "name", sServiceName } }));

	if (!vIsExist)
		return false;

	// Check the service is run
	Variant vServiceStatus = execCommand(createObject(CLSID_WinServiceController), "queryStatus",
		Dictionary({ { "name", sServiceName } }));

	if (vServiceStatus["state"] != 4 /*SERVICE_RUNNING*/)
		return false;
	return true;
}

//
//
//
class AppMode_unprot : public IApplicationMode
{
public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		std::cout << "Stopping self-defense..." << std::endl;

		bool fServiceIsRunning = false;
		bool fDriverIsRunning = false;
		// Check service and driver status
		CMD_TRY
		{
			fServiceIsRunning = checkServiceIsRunning("edrsvc");
			fDriverIsRunning = checkServiceIsRunning("edrdrv");
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& ex)
		{
			std::cout << "Can't check service or driver status. ERROR: " 
				<< ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log(SL, "Can't stop protection via service.");
			return ex.getErrorCode();
		}

		if (!fServiceIsRunning && !fDriverIsRunning)
		{
			std::cout << "Can't stop self-defense because both service and driver are not running (or absent)." << std::endl;
			return ErrorCode::NoAction;
		}

		ErrorCode eReturnedValue = ErrorCode::OK;

		// Try connect to edrsvc and send "unprot" via it
		if (fServiceIsRunning)
		{
			CMD_TRY
			{
				std::cout << "Try to stop via service." << std::endl;
				auto pServiceRpcClient = queryInterface<ICommandProcessor>(getCatalogData("objects.svcControlRpcClient"));

				(void)execCommand(pServiceRpcClient, "allowUnload", Dictionary({ {"value", true} }));

				(void)execCommand(pServiceRpcClient, "execute", Dictionary({
					{"processor", "objects.systemMonitorController"},
					{"command", "setDriverConfig"},
					{"params", Dictionary({ {"disableSelfProtection", true} })}
				}));
				std::cout << "SUCCESS" << std::endl;
				return ErrorCode::OK;
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& ex)
			{
				std::cout << "Can't stop via service. ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
				ex.log(SL, "Can't stop protection via service.");
				eReturnedValue = ex.getErrorCode();
			}
		}

		// Try connect to edrsvc and send "unprot" via it
		if (fDriverIsRunning)
		{
			CMD_TRY
			{
				std::cout << "Try to stop via driver directly." << std::endl;

				// Connect to driver and disable self-defense
				(void)execCommand("objects.systemMonitorController", "setDriverConfig",
					Dictionary({ {"disableSelfProtection", true} }));

				std::cout << "SUCCESS" << std::endl;
				return ErrorCode::OK;
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& ex)
			{
				std::cout << "Can't stop via driver. ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
				ex.log(SL, "Can't stop protection via driver.");
				eReturnedValue = ex.getErrorCode();
			}
		}

		return eReturnedValue;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{}
};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_unprot()
{
	return std::make_shared<AppMode_unprot>();
}

} // namespace cmd