#include "pch.h"
#include "service.h"

namespace cmd {
namespace win {
//
//
//
class AppMode_restart : public IApplicationMode
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
			{
				auto vRes = execCommand(pProcessor, "execute", Dictionary({
					{ "command", "stop" },
					{ "processor", "objects.application" }
					}));

				if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
					std::cout << "Service <" << getCatalogData("app.fullName")
					<< "> has already stoped." << std::endl;
				else
					std::cout << "Service <" << getCatalogData("app.fullName")
					<< "> is stoped." << std::endl;
			}

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
std::shared_ptr<IApplicationMode> createAppMode_restart()
{
	return std::make_shared<win::AppMode_restart>();
}


} // namespace cmd