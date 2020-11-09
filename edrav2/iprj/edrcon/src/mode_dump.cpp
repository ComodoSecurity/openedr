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

namespace openEdr {

//
//
//
class AppMode_dump : public IApplicationMode
{
public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		std::cout << "Global catalog dump:" << std::endl;
		std::cout << getCatalogData("");
		return ErrorCode::OK;
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
std::shared_ptr<IApplicationMode> createAppMode_dump()
{
	return std::make_shared<AppMode_dump>();
}

} // namespace openEdr