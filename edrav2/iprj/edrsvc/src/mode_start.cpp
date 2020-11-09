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

namespace openEdr {
namespace win {

//
//
//
class AppMode_start : public IApplicationMode
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
				{ "command", "start" },
				{ "processor", "objects.application" }
			}));

			if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
				std::cout << "Service <" << getCatalogData("app.fullName")
				<< "> has already run." << std::endl;
			else
				std::cout << "Service <" << getCatalogData("app.fullName")
					<< "> is started." << std::endl;
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
std::shared_ptr<IApplicationMode> createAppMode_start()
{
	return std::make_shared<win::AppMode_start>();
}

} // namespace openEdr