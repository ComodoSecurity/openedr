//
// EDRAv2.cmdcon project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file Uninstall mode handler for cmdcon
///
#include "pch.h"

namespace openEdr {

//
//
//
class AppMode_empty : public IApplicationMode
{
public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
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
std::shared_ptr<IApplicationMode> createAppMode_empty()
{
	return std::make_shared<AppMode_empty>();
}

} // namespace openEdr