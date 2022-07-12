//
// EDRAv2.cmdcon project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (18.01.2019)
//
///
/// @file Install mode handler for cmdcon
///
#include "pch.h"

namespace cmd {

bool fCrtlBreakIsUsed = false;

//
//
//
BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
{
	auto pApp = queryInterfaceSafe<Application>(getCatalogData("objects.application"));
	if (pApp)
		pApp->shutdown();
	fCrtlBreakIsUsed = true;
	return TRUE;
}

//
//
//
class AppMode_wait : public IApplicationMode
{
private:
	Size m_nTimeout = c_nMaxSize;

public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		using namespace std::chrono_literals;
		using namespace std::chrono;
		if (m_nTimeout == c_nMaxSize)
			m_nTimeout = getCatalogData("app.config.timeout", c_nMaxSize);

		if (m_nTimeout == c_nMaxSize)
			std::cout << "Start application and wait infinitely" << std::endl;
		else
			std::cout << "Start application and wait " << m_nTimeout << "ms" << std::endl;

		std::cout << "Use CRTL_BREAK for stopping" << std::endl;
		if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
			error::win::WinApiError(SL, "Can't set CTRL-BREAK handler").throwException();
		
		std::chrono::milliseconds msTimeout(m_nTimeout);
		auto startTime = steady_clock::now();
		while (!pApp->needShutdown())
		{
			milliseconds dur = duration_cast<milliseconds>(steady_clock::now() - startTime);
			if (m_nTimeout != c_nMaxSize && dur > msTimeout) break;
			std::this_thread::sleep_for(100ms);
		}
		return (fCrtlBreakIsUsed)? ErrorCode::UserBreak : ErrorCode::OK;
	}

	//t
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{	
		clp += clara::Opt(m_nTimeout, "msec")["-t"]["--timeout"]("set work time (msec)");
	}

};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_wait()
{
	return std::make_shared<AppMode_wait>();
}

} // namespace cmd