//
// edrav2.edrsvc project
//
// Autor: Denis Bogdanov (10.02.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file comtrol mode handler for edrsvc
///
#include "pch.h"
#include "service.h"

namespace cmd {
namespace win {

//
//
//
class AppMode_stop : public IApplicationMode
{

public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		ObjPtr<ICommandProcessor> pProcessor = startElevatedInstance(true);

		try
		{
			auto vRes = execCommand(pProcessor, "execute", Dictionary({
				{ "clsid", CLSID_Command },
				{ "command", "stop" },
				{ "processor", "objects.application" }
			}));

			if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
				std::cout << "Service <" << getCatalogData("app.fullName")
				<< "> is not run." << std::endl;
			else
				std::cout << "Service <" << getCatalogData("app.fullName")
					<< "> is stopped." << std::endl;
		}
		catch (...)
		{
			stopElevatedInstance(pProcessor, true);
			throw;
		}
		stopElevatedInstance(pProcessor, true);
		return ErrorCode::OK;
	}
};

} // namespace win

 //
//
//
std::shared_ptr<IApplicationMode> createAppMode_stop()
{
	return std::make_shared<win::AppMode_stop>();
}

} // namespace cmd