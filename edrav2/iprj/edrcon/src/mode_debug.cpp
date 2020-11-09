//
// EDRAv2.cmdcon project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: ()
//
///
/// @file Debug mode handler for cmdcon
///
#include "pch.h"

namespace openEdr {

static inline bool c_fCrtlBreakIsUsed = false;
static inline bool c_fStopOutput = false;
static inline bool c_fOutputIsActive = false;

//
//
//
static inline BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
{
	if (c_fOutputIsActive)
	{
		c_fStopOutput = true;
		return TRUE;
	}
	auto pApp = queryInterfaceSafe<Application>(getCatalogData("objects.application"));
	if (pApp)
		pApp->shutdown();
	c_fCrtlBreakIsUsed = true;
	return TRUE;
}

//
//
//
class AppMode_debug : public IApplicationMode
{
private:
	ObjPtr<ICommandProcessor> m_pCurrProcessor;
	bool m_fNoColor = false;

	//
	//
	// 
	void resetProcessor()
	{
		m_pCurrProcessor = queryInterface<ICommandProcessor>(queryService("application"));
	}

	//
	//
	// 
	bool isLocalProcessor()
	{
		return m_pCurrProcessor->getClassId() == CLSID_Application;
	}

	//
	// 
	// 
	std::string cmd_connect(Application* pApp, std::ostream& out, std::string sAddress)
	{
		if (!isLocalProcessor())
		{
			out << "Disconnecting from the current remote application..." << std::endl;
			resetProcessor();
		}

		std::string sHost(getCatalogData("app.config.remote.host", "127.0.0.1"));
		Size nPort = getCatalogData("app.config.remote.port", 9999);
		std::string sChannelMode(getCatalogData("app.config.remote.channelMode", ""));
		std::string sEncryption(getCatalogData("app.config.remote.encryption", ""));
		std::string sResAddress;

		try
		{

			std::vector<std::string> vecData;
			boost::split(vecData, sAddress, boost::is_any_of(":-"), boost::token_compress_on);
			if (!sAddress.empty())
			{
				sHost = vecData[0];
				if (vecData.size() > 1)
					nPort = std::stoi(vecData[1]);
			}

			out << "Connecting to " << sHost << ":" << nPort << "..." << std::endl;
			Variant vConfig = Dictionary({
				{"host", sHost},
				{"port", nPort},
				{"encryption", sEncryption}
				});
			if (!sChannelMode.empty())
				vConfig.put("channelMode", sChannelMode);
			m_pCurrProcessor = queryInterface<ICommandProcessor>(
				createObject(CLSID_JsonRpcClient, vConfig));

			if (m_pCurrProcessor->execute("delay", Dictionary({ {"msec", 100 } })))
				out << "Connection is established." << std::endl;

			sResAddress = sHost + ":" + std::to_string(nPort);
		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "CONNECTION ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
			resetProcessor();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC CONNECTION ERROR" << std::endl;
			error::RuntimeError("Generic error during connection").log();
			resetProcessor();
		}
		out << rang::style::reset;
		return sResAddress;
	}

	//
	// 
	// 
	bool cmd_disconnect(Application* pApp, std::ostream& out)
	{
		if (!isLocalProcessor())
		{
			out << "Disconnecting from current remote application..." << std::endl;
			resetProcessor();
			return true;
		}
		out << "Debugger has no remote connection." << std::endl;
		return false;
	}

	//
	//
	//
	std::string filterString(std::string_view sData)
	{
		std::string sResult;
		sResult.reserve(sData.size());
		bool fUS = false;
		for (auto it = sData.begin(); it != sData.end(); ++it)
		{
			if (*it != '_')
			{
				if (fUS) sResult += ' ';
				sResult += *it;
				fUS = false;
			}
			else if (!fUS)
			{
				fUS = true;
			}
			else
			{
				sResult += '_';
				fUS = false;
			}
		}
		return sResult;
	}

	//
	//
	//
	Variant cmd_exec(Application* pApp, std::ostream& out, std::string_view sCmd, std::string_view sParams)
	{
		Variant vRes;
		try
		{
			std::vector<std::string> vecData;
			auto nPos = sCmd.find_last_of(":.");
			std::string sProcessor("objects.application");
			std::string sCommand(sCmd);
			if (nPos != std::string::npos)
			{
				sProcessor = sCmd.substr(0, nPos);
				sCommand = sCmd.substr(nPos + 1);
			}

			if (sProcessor.empty() || sCommand.empty())
				error::InvalidArgument("Bad command is specified").throwException();
			
			Variant vProcessor = sProcessor;
			if (sProcessor[0] == '{')
			{
				//std::replace(sProcessor.begin(), sProcessor.end(), '_', ' ');
				vProcessor = variant::deserializeFromJson(filterString(sProcessor));
			}

			auto vParams = Dictionary();
			if (!sParams.empty())
				vParams = variant::deserializeFromJson(filterString(sParams));

			vRes = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"processor", vProcessor},
				{"command", sCommand},
				{"params", vParams}
			}));
			out << rang::fg::cyan << vRes << std::endl;

		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
		return vRes;
	}

	//
	//
	//
	void cmd_get(Application* pApp, std::ostream& out, std::string_view sPath, bool fResolve = false)
	{
		std::string sParam("{\"path\":\"");
		sParam += std::string(sPath) + "\"";
		if (fResolve) sParam += ",\"resolve\":true";
		sParam += "}";
		(void)cmd_exec(pApp, out, "getCatalogData", sParam);
	}

	//
	//
	//
	void cmd_put(Application* pApp, std::ostream& out, std::string& sPath, std::string& sData)
	{
		std::string sParam("{\"path\":\"");
		sParam += sPath + "\",\"data\":";
		if (sData.empty())
			sParam += "null";
		else if (sData[0] == '{')
			sParam += sData;
		else if (sData[0] >= '0' && sData[0] <= '9')
			sParam += sData;
		else if (sData == "null" || sData == "true" || sData == "false")
			sParam += sData;
		else
			sParam += "\"" + sData + "\"";
		sParam += "}";
		(void)cmd_exec(pApp, out, "putCatalogData", sParam);
	}

	//
	//
	//
	void setCursorPosition(Size x, Size y)
	{
		//COORD position = { x,y };
		//sys::win::ScopedHandle hCon(GetStdHandle(STD_OUTPUT_HANDLE));
		//SetConsoleCursorPosition(hCon, position);
	}

	//
	//
	//
	void cmd_qhealth(Application* pApp, std::ostream& out, Size nRepeatCount = 1)
	{
		try
		{
			c_fStopOutput = false;
			c_fOutputIsActive = true;
			while (nRepeatCount-- > 0 && !c_fStopOutput)
			{
				// Output queues state
				auto vQueuesRes = execCommand(m_pCurrProcessor, "execute", Dictionary({
					{"processor", "objects.queueManager"},
					{"command", "getInfo"}
				}));
				auto vQueues = vQueuesRes.get("queues");

				std::cout << "== Queues statistic ============================================"
					"=========================================================================================" << std::endl;
				std::cout << "| progress               | size     | maxSize  | putCount | putTry   |"
					" getCount | getTry   | rollback | inRate   | outRate  | name                      |" << std::endl;
				for (auto v : Dictionary(vQueues))
				{
					auto vInfo = v.second;

					// Show progress
					std::cout << "| [";
					const Size c_nBarSize = 20;
					Size nCurrSize = vInfo["size"];
					Size nMaxSize = vInfo["maxSize"];
					Size nProgSize = nCurrSize / (nMaxSize / c_nBarSize);
					for (Size i = 0; i < nProgSize; ++i)
						std::cout << '#';
					for (Size i = 0; i < c_nBarSize - nProgSize; ++i)
						std::cout << '.';

					// Show data
					std::cout << "]"
						<< " | " << std::setfill(' ') << std::setw(8) << nCurrSize
						<< " | " << std::setfill(' ') << std::setw(8) << nMaxSize
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["putCount"]
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["putTryCount"]
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["getCount"]
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["getTryCount"]
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["rollbackCount"]
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["inRate"]
						<< " | " << std::setfill(' ') << std::setw(8) << vInfo["outRate"]
						<< " | " << std::setfill(' ') << std::setw(25) << v.first << " |" << std::endl;
				}

				// Script statistic
				auto vScenarioRes = execCommand(m_pCurrProcessor, "execute", Dictionary({
					{"processor", "objects.scenarioManager"},
					{"command", "getStatistic"}
					}));

				if (!vScenarioRes.has("reports"))
				{
					std::cout << "No scenarios-related data is available" << std::endl;
					return;
				}

				auto vScenarioRep = vScenarioRes["reports"];

				auto vScenariosByGlobalTime = vScenarioRep["scenariosByGlobalTime"];
				auto vScenariosByThreadTime = vScenarioRep["scenariosByThreadTime"];
				Size nScenariosCount = std::min(vScenariosByGlobalTime.getSize(), vScenariosByThreadTime.getSize());

				std::cout << std::endl <<  "== Hot scenarios =========================================="
					"=========================" << std::endl << "== By global time ======================    "
					<< "== By thread time ======================" << std::endl 
					<<	"| scenario name             | time(ms) |    | scenario name             | time(ms) |" << std::endl;
				for (Size i = 0; i < nScenariosCount; ++i)
				{
					std::cout
						<< "| " << std::setfill(' ') << std::setw(25) << vScenariosByGlobalTime.get(i)["name"]
						<< " | " << std::setfill(' ') << std::setw(8) << vScenariosByGlobalTime.get(i)["time"]
						<< " |    | " << std::setfill(' ') << std::setw(25) << vScenariosByThreadTime.get(i)["name"]
						<< " | " << std::setfill(' ') << std::setw(8) << vScenariosByThreadTime.get(i)["time"]
						<< " |" << std::endl;
				}

				auto vLinesByGlobalTime = vScenarioRep["linesByGlobalTime"];
				auto vLinesByThreadTime = vScenarioRep["linesByThreadTime"];
				Size nLinesCount = std::min(vLinesByGlobalTime.getSize(), vLinesByThreadTime.getSize());
				std::cout << std::endl << "== Hot lines ==============================================="
					"==========================================================================" << std::endl 
					<< "== By global time ===============================================    "
					<< "== By thread time ===============================================" << std::endl
					<< "| scenario point                                     | time(ms) |    "
					<< "| scenario point                                     | time(ms) |" << std::endl;
				for (Size i = 0; i < nLinesCount; ++i)
				{
					std::cout
						<< "| " << std::setfill(' ') << std::setw(50) << vLinesByGlobalTime.get(i)["name"]
						<< " | " << std::setfill(' ') << std::setw(8) << vLinesByGlobalTime.get(i)["time"]
						<< " |    | " << std::setfill(' ') << std::setw(50) << vLinesByThreadTime.get(i)["name"]
						<< " | " << std::setfill(' ') << std::setw(8) << vLinesByThreadTime.get(i)["time"]
						<< " |" << std::endl;
				}

				// std::cout << vScenarioRes << std::endl;
				if (nRepeatCount != 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

	//
	//
	//
	void cmd_strace(Application* pApp, std::ostream& out, std::string& sName)
	{
		try
		{
			// Output queues state
			auto vScenarioRes = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"processor", "objects.scenarioManager"},
				{"command", "getStatistic"},
				{"params", Dictionary({{"showLineInfo", true}}) }
			}));
			auto vStatInfo = vScenarioRes["full"];
			
			if (!vStatInfo.has(sName))
			{
				std::cout << "No data is found for scenario: " << sName << std::endl;
				return;
			}

			Dictionary vLineInfo = vStatInfo[sName]["lines"];
	
			std::cout << "== Trace for scenario: " << sName  << std::endl
				<< "| scenario name             | count    | gtime    | ttime    |" << std::endl;
			for (auto&[sLine, vTime] : vLineInfo)
			{
				std::cout
					<< "| " << std::setfill(' ') << std::setw(25) << sLine
					<< " | " << std::setfill(' ') << std::setw(8) << vTime["count"]
					<< " | " << std::setfill(' ') << std::setw(8) << vTime["globalTime"]
					<< " | " << std::setfill(' ') << std::setw(8) << vTime["threadTime"]
					<< " |" << std::endl;
			}

			auto vTotalInfo = vStatInfo[sName]["total"];
			std::cout << "|                    Total: | "
				<< std::setfill(' ') << std::setw(8) << vTotalInfo["count"]
				<< " | " << std::setfill(' ') << std::setw(8) << vTotalInfo["globalTime"]
				<< " | " << std::setfill(' ') << std::setw(8) << vTotalInfo["threadTime"] << " |" << std::endl;
		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

	//
	//
	//
	void cmd_qdump(Application* pApp, std::ostream& out, std::string& sName, std::string& sPath, bool fPrint)
	{
		try
		{
			std::filesystem::path pathToDump(getCatalogData("app.workPath").convertToPath());
			pathToDump /= sPath;

			// Output queues state
			auto vData = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"processor", Dictionary({
					{"$$proxy", "cachedCmd"},
					{"processor", "objects.queueManager"},
					{"command", "getQueue"},
					{"params", Dictionary({{ "name", sName }})}
				})},
				{"command", "dump"},
				{"params", Dictionary({
					{"print", fPrint},
					{"stream", Dictionary({
						{"$$proxy", "cachedObj"},
						{"clsid", CLSID_File},
						{"mode", "wtRW"},
						{"path", pathToDump.string()}
					})}
				})}
			}));
								
			std::cout << "File <" << sPath << "> is saved" << std::endl;

		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

	//
	//
	//
	void cmd_qstat(Application* pApp, std::ostream& out, std::string& sName, std::string& sIndex)
	{
		try
		{
			// Output queues state
			auto vStat = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"processor", Dictionary({
					{"$$proxy", "cachedCmd"},
					{"processor", "objects.queueManager"},
					{"command", "getQueue"},
					{"params", Dictionary({{ "name", sName }})}
				})},
				{"command", "getDataStatistic"},
				{"params", Dictionary({
					{"index", sIndex},
				})}
			}));

			std::cout << "== Statistic for queue <" << sName << "> by index <" << sIndex << ">" << std::endl
				<< "| count    | Index Data                            " << std::endl;

			Dictionary vItems = vStat.get("items", Dictionary());
			for (auto&[sName, vItem] : vItems)
			{
				std::cout
					<< "| " << std::setfill(' ') << std::setw(8) << vItem
						<< " | " << sName << std::endl;
			}

			std::cout
				<< "| " << std::setfill(' ') << std::setw(8) << vStat.get("skipped", 0)
				<< " | <skipped>" << std::endl;

		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

	//
	//
	//
	void cmd_qget(Application* pApp, std::ostream& out, std::string& sName)
	{
		try
		{
			// Output queues state
			auto vItem = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"processor", Dictionary({
					{"$$proxy", "cachedCmd"},
					{"processor", "objects.queueManager"},
					{"command", "getQueue"},
					{"params", Dictionary({{ "name", sName }})}
				})},
				{"command", "get"}
			}));

			std::cout << vItem.print() << std::endl;
		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

	//
	//
	//
	void cmd_mstat(Application* pApp, std::ostream& out)
	{
		try
		{
			// Output queues state
			auto vMemInfo = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"processor", "objects.application"},
				{"command", "getMemoryInfo"},
			}));

			std::cout << "== Memory information" << std::endl;
			std::cout << "Allocated memory: " << vMemInfo["size"] << " in " << 
				vMemInfo["count"] << " blocks" << std::endl;
			std::cout << "Total allocated memory: " << vMemInfo["totalSize"] << " in " << 
				vMemInfo["totalCount"] << " blocks" << std::endl;
		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

	//
	//
	//
	void cmd_lanestat(Application* pApp, std::ostream& out)
	{
		try
		{
			// Enable lane statistic
			execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"clsid", CLSID_LaneOperation},
				{"operation", "enableStatistic"},
			}));

			// Output lanes counters
			Variant vLaneInfo = execCommand(m_pCurrProcessor, "execute", Dictionary({
				{"clsid", CLSID_LaneOperation},
				{"operation", "getStatistic"},
			}));

			out << "== Lanes statistic ============================================" << std::endl;
			if (vLaneInfo.isNull())
			{
				out << "Statistic is disabled" << std::endl;
				return;
			}

			Sequence vTagsByCount (vLaneInfo.get("tagsByCount", Sequence()));

			out << std::endl << "== Tags by count =======================" << std::endl
				<< "| tag                       | count    |" << std::endl;
			for (auto item : vTagsByCount)
			{
				out << "| " << std::setfill(' ') << std::setw(25) << item["tag"]
					<< " | " << std::setfill(' ') << std::setw(8) << item["count"] 
					<< " |" << std::endl;
			}

		}
		catch (error::Exception & ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}


	//
	//
	//
	void cmd_mstat2(Application* pApp, std::ostream& out)
	{
		try
		{
			// Output queues state
			auto memInfo = getMemoryInfo();
			std::cout << "== Memory information" << std::endl;
			std::cout << "Allocated memory: " << memInfo.nCurrSize << " in " <<
				memInfo.nCurrCount << " blocks" << std::endl;
			std::cout << "Total allocated memory: " << memInfo.nTotalSize << " in " <<
				memInfo.nTotalCount << " blocks" << std::endl;
		}
		catch (error::Exception& ex)
		{
			out << rang::fg::red << rang::style::bold
				<< "ERROR: " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			ex.log();
		}
		catch (...)
		{
			out << rang::fg::red << rang::style::bold
				<< "GENERIC ERROR" << std::endl;
			error::RuntimeError("Generic error during command processing").log();
		}
		out << rang::style::reset;
	}

public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		std::cout << "EDR Application debugger v." << CMD_STD_VERSION << std::endl;
		std::cout << "Use CRTL-BREAK or 'application.shutdown()' command for stopping." << std::endl
			<< "For entring commands and parameters the json format is allowed but all"
			" spaces in description shall be replaced by '_'" << std::endl;

		resetProcessor();

		// Reset m_pCurrProcessor on scope exit
		// For cutting circular pointing AppMode_debug <-> Application
		std::shared_ptr<int> processorDeleter(nullptr, [this](int*) {
			this->m_pCurrProcessor.reset();
		});


		if (m_fNoColor)
			cli::SetNoColor();
		else
			cli::SetColor();
		//rang::setControlMode(rang::control::Auto);
		//rang::setWinTermMode(rang::winTerm::Auto)

		if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
			error::win::WinApiError(SL, "Can't set CTRL-BREAK handler").throwException();

		cli::Menu* pMenu = nullptr;

		// Prepare 'main' menu
		auto rootMenu = std::make_unique<cli::Menu>("local");
		rootMenu->Add("exec",
			[this, pApp](std::string sCmd, std::ostream& out)
			{ (void)this->cmd_exec(pApp, out, sCmd, ""); },
			"Execute the specified command of a service (default: application)");
		rootMenu->Add("exec",
			[this, pApp](std::string sCmd, std::string sParams, std::ostream& out)
			{ (void)this->cmd_exec(pApp, out, sCmd, sParams); },
			"Execute the specified command of a service with parameters (in json format)");
		rootMenu->Add("get",
			[this, pApp](std::ostream& out)
			{ (void)this->cmd_get(pApp, out, ""); },
			"Get the data from the entire catalog");
		rootMenu->Add("get",
			[this, pApp](std::string sPath, std::ostream& out)
			{ (void)this->cmd_get(pApp, out, sPath, false); },
			"Get data from a catalog using a specified path");
		rootMenu->Add("getr",
			[this, pApp](std::string sPath, std::ostream& out)
			{ (void)this->cmd_get(pApp, out, sPath, true); },
			"Get data from a catalog using a specified path and resolving proxy values");
		rootMenu->Add("put",
			[this, pApp](std::string sPath, std::string sData, std::ostream& out)
			{ (void)this->cmd_put(pApp, out, sPath, sData); },
			"Put the data to catalog using a specified path (parameter is in json format)");
		rootMenu->Add("strace",
			[this, pApp](std::string sScName, std::ostream& out)
			{ this->cmd_strace(pApp, out, sScName); },
			"Output scenario trace");
		rootMenu->Add("qdump",
			[this, pApp](std::string sScName, std::string sPath, std::ostream& out)
			{ this->cmd_qdump(pApp, out, sScName, sPath, false); },
			"Dump the specified queue into a the specified file");
		rootMenu->Add("qprint",
			[this, pApp](std::string sScName, std::string sPath, std::ostream& out)
			{ this->cmd_qdump(pApp, out, sScName, sPath, true); },
			"Print the specified queue into a the specified file");
		rootMenu->Add("qstat",
			[this, pApp](std::string sScName, std::string sIndex, std::ostream& out)
			{ this->cmd_qstat(pApp, out, sScName, sIndex); },
			"[queueName, fieldName] - output queue statistic by the specified field (index)");
		rootMenu->Add("qget",
			[this, pApp](std::string sScName, std::ostream& out)
			{ this->cmd_qget(pApp, out, sScName); },
			"[queueName] - output one item from the specified queue (item is deleted from queue)");
		rootMenu->Add("mstat",
			[this, pApp](std::ostream& out)
			{ this->cmd_mstat(pApp, out); },
			"Output memory statistic");
		rootMenu->Add("mstat2",
			[this, pApp](std::ostream& out)
			{ this->cmd_mstat2(pApp, out); },
			"Output memory statistic using local raw functions");
		rootMenu->Add("lanestat",
			[this, pApp](std::ostream& out)
			{ this->cmd_lanestat(pApp, out); },
			"Output lane statistic");
		rootMenu->Add("qhealth",
			[this, pApp](std::ostream& out)
			{ this->cmd_qhealth(pApp, out, 1); },
			"Show current health status related to the queues and scenarios");
		rootMenu->Add("qhealth*",
			[this, pApp](std::ostream& out)
			{ this->cmd_qhealth(pApp, out, 10000); },
			"Runs <qhealth> command repeatedly until CTRL-BREAK signal");
		rootMenu->Add("connect",
			[this, pApp, &pMenu](std::string sAddress, std::ostream& out)
			{auto s = this->cmd_connect(pApp, out, sAddress); if (!s.empty()) pMenu->SetPrompt(s); },
			"Connect to a remote application instance (address:port)");
		rootMenu->Add("connect",
			[this, pApp, &pMenu](std::ostream& out)
			{auto s = this->cmd_connect(pApp, out, ""); if (!s.empty()) pMenu->SetPrompt(s); },
			"Connect to a remote application instance (default address)");
		rootMenu->Add("disconnect",
			[this, pApp, &pMenu](std::ostream& out)
			{ if (this->cmd_disconnect(pApp, out)) pMenu->SetPrompt("local"); },
			"Disconnect from the current remote application instance");

		// Init and start input loop
		cli::Cli currCli(std::move(rootMenu));
		pMenu = currCli.RootMenu();
		currCli.ExitAction([](auto& out)
		{ out << "Debug session is finished. Goodbye...\n"; });
		cli::CliFileSession input(currCli);

		//std::filesystem::path pathHistory(getCatalogData("app.dataPath", ""));
		//pathHistory /= ".debug.history";
		//std::ifstream istr(pathHistory.c_str());
		//input.GetHistory().Load(istr);
		//input.GetHistory().Save(std::cout);
		//istr.close();
		input.Start();
		//std::ofstream ostr(pathHistory.c_str());
		//input.GetHistory().Save(ostr);

		//while (!pApp->needShutdown())

		return (c_fCrtlBreakIsUsed)? ErrorCode::UserBreak : ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{	
		clp += clara::Opt(m_fNoColor)["--noColor"]("switch off color output");
	}

};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_debug()
{
	return std::make_shared<AppMode_debug>();
}

} // namespace openEdr