//
// derav2.cmdcon project
//
// Autor: Denis Bogdanov (15.03.2019)
// Reviewer: 
//
///
/// @file Install mode handler for cmdcon
///
#include "pch.h"

namespace openEdr {

static inline bool fCrtlBreakIsUsed = false;

//
//
//
static inline BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
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
class AppMode_run : public IApplicationMode
{
private:
	Size m_nTimeout = c_nMaxSize;
	Size m_nICountParam = c_nMaxSize;
	Size m_nOCountParam = c_nMaxSize;
	std::string m_sParam1;
	std::string m_sParam2;
	std::string m_sParam3;
	std::string m_sInParam;
	std::string m_sOutParam;
	std::vector<std::string> m_scripts;
	std::vector<ObjPtr<IProgressReporter>> m_progPeporters;
	bool m_fShowProress = true;

	//
	//
	//
	Variant loadScript(std::filesystem::path pathScenario)
	{
		if (!std::filesystem::exists(pathScenario) ||
			!std::filesystem::is_regular_file(pathScenario)) return {};

		return variant::deserializeFromJson(queryInterface<io::IRawReadableStream>(
			(io::createFileStream(pathScenario, io::FileMode::Read |
				io::FileMode::ShareRead | io::FileMode::ShareWrite))));
	}

	//
	//
	//
	void runScript(std::string sName)
	{
		std::cout << "Running script <" << sName << ">..." << std::endl;

		if (!boost::algorithm::iends_with(sName, ".sc"))
			sName += ".sc";

		std::filesystem::path pathScenario(sName);
		std::filesystem::path pathFull;

		Variant vScript;
		if (pathScenario.is_absolute())
		{
			pathFull = pathScenario;
			vScript = loadScript(pathFull);
		}
		else do
		{
			// Load from current path
			pathFull = std::filesystem::current_path() / pathScenario;
			vScript = loadScript(pathFull);
			if (!vScript.isEmpty()) break;

			// Load from directory in current path
			pathFull = std::filesystem::current_path() / "scripts" / pathScenario;
			vScript = loadScript(pathFull);
			if (!vScript.isEmpty()) break;

			// Load from environment path
			std::string sScriptsDir(getCatalogData("app.variables.%env_edrrunscripts%", ""));
			if (!sScriptsDir.empty())
			{
				pathFull = sScriptsDir / pathScenario;
				std::cout << "===" << pathFull << std::endl;
				vScript = loadScript(pathFull);
				if (!vScript.isEmpty()) break;
			}

			// Load from profile path
			std::filesystem::path pathData(getCatalogData("app.dataPath", ""));
			pathFull = pathData / "scripts" / pathScenario;
			vScript = loadScript(pathFull);
			if (!vScript.isEmpty()) break;

			// Load from image path
			std::filesystem::path pathImage(getCatalogData("app.imagePath", ""));
			pathFull = pathImage / "scripts" / pathScenario;
			vScript = loadScript(pathFull);
			if (!vScript.isEmpty()) break;

		} while (false);

		if (vScript.isEmpty())
		{
			std::cout << "Scenario file isn't found. You can specify the full path, use "
						 "profile of binary directories\n or specify %edrrunscripts% "
						 "environment variable" << std::endl;
			error::InvalidArgument(SL, FMT("Script <" << sName << "> is not found or empty.")).throwException();
		}
	
		std::cout << "Use scenario file <" << pathFull << ">" << std::endl;

		if (vScript.isDictionaryLike() && vScript.has("description"))
			std::cout << "Description: " << vScript.get("description", "<???>") << std::endl;

		// Run script commands
		Sequence seqCommands = vScript.isSequenceLike() ? Sequence(vScript) :
			Sequence(vScript.get("commands", {}));
		for (auto vCmdInfo : seqCommands)
		{
			bool fOut = vCmdInfo.has("description");
			if (fOut) std::cout << "--> " <<
				vCmdInfo.get("description", "<unnnamed>") << ": " << std::flush;
			auto vRes = execCommand(vCmdInfo["command"]);
			if (fOut)
				if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
					std::cout << "FALSE" << std::endl;
				else
					std::cout << "OK" << std::endl;
		}

		if (!vScript.isDictionaryLike()) return;
		m_fShowProress = vScript.get("showProgress", m_fShowProress);
		if (vScript.has("reporter"))
		{
			auto pRep = queryInterfaceSafe<IProgressReporter>(getCatalogData(vScript["reporter"]));
			if (pRep) 
				m_progPeporters.push_back(pRep);
		}
	}

public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		TRACE_BEGIN;
		using namespace std::chrono_literals;
		using namespace std::chrono;
		if (m_nTimeout == c_nMaxSize)
			m_nTimeout = getCatalogData("app.config.timeout", c_nMaxSize);

		if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
			error::win::WinApiError(SL, "Can't set CTRL-BREAK handler").throwException();

		std::cout << "Script scenarios interpretor v." << CMD_STD_VERSION << std::endl;

		if (m_nICountParam != c_nMaxSize)
			putCatalogData("app.params.script.icount", m_nICountParam);
		if (m_nOCountParam != c_nMaxSize)
			putCatalogData("app.params.script.ocount", m_nOCountParam);
		if (!m_sInParam.empty())
			putCatalogData("app.params.script.in", m_sInParam);
		if (!m_sOutParam.empty())
			putCatalogData("app.params.script.out", m_sOutParam);
		if (!m_sParam1.empty())
			putCatalogData("app.params.script.p1", m_sParam1);
		if (!m_sParam2.empty())
			putCatalogData("app.params.script.p2", m_sParam2);
		if (!m_sParam3.empty())
			putCatalogData("app.params.script.p3", m_sParam3);

		for (auto& sScript : m_scripts)
			runScript(sScript);
		

		if (m_nTimeout != c_nMaxSize)
			std::cout << "Waiting results for " << m_nTimeout << "ms:" << std::endl;
		else if (!m_progPeporters.empty())
			std::cout << "Waiting results from reporters:" << std::endl;
		else
			std::cout << "Run infinitely (use <CRTL-BREAK> for exit):" << std::endl;
	
		// Check timeout
		std::chrono::milliseconds msTimeout(m_nTimeout);
		auto startTime = steady_clock::now();
		bool fWorkIsDone = false;
		while (!pApp->needShutdown() && !fWorkIsDone)
		{
			if (!m_progPeporters.empty())
			{
				fWorkIsDone = true;

				if (m_fShowProress)
					std::cout << "--> Processed:";

				for (auto pRep : m_progPeporters)
				{
					if (m_fShowProress) std::cout << " [";
					auto vInfo = pRep->getProgressInfo();
					Size nCurr = vInfo.get("currPoint", 0);
					Size nTotal = vInfo.get("totalPoints", c_nMaxSize);
					if (nCurr < nTotal) fWorkIsDone = false;
					
					if (!m_fShowProress) continue;
					
					std::cout << std::setfill('0') << std::setw(8) << nCurr << "/";
					if (nTotal == c_nMaxSize) std::cout << "--------]";
					else std::cout << std::setfill('0') << std::setw(8) << nTotal << "]";
				}
				std::cout << std::endl;
			}

			milliseconds dur = duration_cast<milliseconds>(steady_clock::now() - startTime);
			if (m_nTimeout != c_nMaxSize && dur > msTimeout) break;
			std::this_thread::sleep_for(500ms);
		}

		// Stop scripts
		sendMessage("appScriptStop");

		// Stop scenario processors
		auto pScenarioMgr = queryInterfaceSafe<IService>(queryService("scenarioManager"));
		if (pScenarioMgr)
			pScenarioMgr->stop();

		// Output queues state
		auto pQueueMgr = queryInterfaceSafe<IQueueManager>(queryService("queueManager"));
		if (!pQueueMgr) return ErrorCode::OK;
		std::cout << "Queues statistic:" << std::endl;
		std::cout << " | Queue name                 size (maxSize)      putCount (putTry)   getCount (getTry)"
			<< std::endl;
		auto vQueues = pQueueMgr->getInfo().get("queues");
		for (auto v : Dictionary(vQueues))
		{
			auto vInfo = v.second;
			if (vInfo["size"] == 0 && vInfo["putCount"] == 0 && vInfo["putTryCount"] == 0 &&
				vInfo["getCount"] == 0 && vInfo["getTryCount"] == 0) continue;

			std::cout << " | " << std::setfill('.') << std::setw(25) << v.first << ": "
				<< std::setfill('0') << std::setw(8) << vInfo["size"] 
				<< " (" << std::setfill('0') << std::setw(8) << vInfo["maxSize"] << ") "
				<< std::setfill('0') << std::setw(8) << vInfo["putCount"]
				<< " (" << std::setfill('0') << std::setw(8) << vInfo["putTryCount"] << ") "
				<< std::setfill('0') << std::setw(8) << vInfo["getCount"]
				<< " (" << std::setfill('0') << std::setw(8) << vInfo["getTryCount"] << ")"
				<< std::endl;
		}
		
		return (fCrtlBreakIsUsed)? ErrorCode::UserBreak : ErrorCode::OK;
		TRACE_END("Error during script execution");
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{	
		clp += clara::Arg(m_scripts, "scripts") ("Scripts' names");
		clp += clara::Opt(m_nTimeout, "msec")["-t"]["--timeout"]("set work time (msec)");
		clp += clara::Opt(m_nICountParam, "icount")["--icount"]("set icount for script logic");
		clp += clara::Opt(m_nOCountParam, "ocount")["--ocount"]("set ocount for script logic");
		clp += clara::Opt(m_sInParam, "in")["--in"]("set input parameter for script logic");
		clp += clara::Opt(m_sOutParam, "out")["--out"]("set output for script logic");
		clp += clara::Opt(m_sParam1, "param1")["--p1"]("set param1 variable for script logic");
		clp += clara::Opt(m_sParam2, "param2")["--p2"]("set param2 variable for script logic");
		clp += clara::Opt(m_sParam3, "param3")["--p3"]("set param3 variable for script logic");
	}

};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_run()
{
	return std::make_shared<AppMode_run>();
}

} // namespace openEdr