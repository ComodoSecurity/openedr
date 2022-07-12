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
class AppMode_default : public IApplicationMode
{
public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		std::cout << "Default application mode is empty" << std::endl;
		return ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{
		// clp += clara::Arg(m_sTestMode, "mode3|mode4") ("Working mode 2");
	}

};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_default()
{
	return std::make_shared<AppMode_default>();
}

} // namespace cmd