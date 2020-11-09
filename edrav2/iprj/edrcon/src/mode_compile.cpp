//
// edrav2.edrcon
//
// Autor: Yury Ermakov (09.09.2019)
// Reviewer:
//
///
/// @file Compile mode handler for edrcon
///
#include "pch.h"

namespace openEdr {

//
//
//
class AppMode_compile : public IApplicationMode
{
private:
	std::vector<std::string> m_srcFiles;
	std::string m_sCfgFileName;
	std::string m_sOutFileName;
	bool m_fVerbose = false;

	//
	//
	//
	void showParameters() const
	{
		std::cout << "---------- Parameters -----------" << std::endl;
		std::cout << "Config file: " << m_sCfgFileName << std::endl;
		std::cout << "Output file: " << m_sOutFileName << std::endl;
		std::cout << "Source files: " << std::endl;
		for (const auto& sName : m_srcFiles)
			std::cout << "  " << sName << std::endl;
		std::cout << "---------------------------------" << std::endl;
	}

	//
	//
	//
	std::string getSourceName(std::string_view sName) const
	{
		std::vector<std::string> nameParts;
		boost::split(nameParts, sName, boost::is_any_of("."));

		auto it = nameParts.begin();
		std::string sResult = *it;
		for (++it; it != nameParts.end(); ++it)
		{
			std::string sPart = *it;
			sPart[0] = std::toupper(sPart[0]);
			sResult += sPart;
		}
		return sResult;
	}

	//
	//
	//
	Variant loadSingleSource(std::string_view sName) const
	{
		std::cout << sName << std::endl;

		std::filesystem::path pathSource(sName);
		if (!pathSource.is_absolute())
			pathSource = std::filesystem::current_path() / pathSource;

		if (!std::filesystem::exists(pathSource) ||
			!std::filesystem::is_regular_file(pathSource))
			error::InvalidArgument(SL, FMT("Source file <" << sName << "> is not found")).throwException();

		auto vSource = variant::deserializeFromJson(queryInterface<io::IRawReadableStream>(
			(io::createFileStream(pathSource, io::FileMode::Read |
				io::FileMode::ShareRead | io::FileMode::ShareWrite))));

		std::string sRawName = (pathSource.has_stem() ?
			pathSource.stem().string() : pathSource.filename().string());

		return Dictionary({ {getSourceName(sRawName), vSource} });
	}

	//
	//
	//
	Variant loadSources() const
	{
		std::cout << "Loading source files..." << std::endl;
		Variant vResult = Dictionary();
		for (const auto& sName : m_srcFiles)
		{
			auto vSourceDescr = loadSingleSource(sName);
			if (vSourceDescr.isEmpty())
			{
				std::cout << "WARNING: File <" << sName << "> is empty" << std::endl;
				continue;
			}
			vResult.merge(vSourceDescr, variant::MergeMode::All |
				variant::MergeMode::CheckType | variant::MergeMode::MergeNestedDict |
				variant::MergeMode::MergeNestedSeq);
		}
		return vResult;
	}

	//
	//
	//
	void saveResult(Variant vData) const
	{
		std::cout << "Saving the result policy to <" << m_sOutFileName << ">..." << std::endl;
		auto pOutFile = queryInterface<io::IWritableStream>(io::createFileStream(
			m_sOutFileName, io::FileMode::Write | io::FileMode::Truncate | io::FileMode::ShareRead));
		variant::serialize(pOutFile, Dictionary({ {"scenario", vData} }));
	}

	//
	//
	//
	Variant loadCompilerConfig() const
	{
		std::cout << "Loading compiler config..." << std::endl;

		std::string sFileName = (!m_sCfgFileName.empty() ? m_sCfgFileName : "compiler.cfg");
		std::filesystem::path pathConfig(sFileName);
		if (!pathConfig.is_absolute())
			pathConfig = std::filesystem::current_path() / pathConfig;

		if (!std::filesystem::exists(pathConfig) ||
			!std::filesystem::is_regular_file(pathConfig))
			error::InvalidArgument(SL, FMT("File <" << sFileName << "> is not found")).throwException();

		return variant::deserializeFromJson(queryInterface<io::IRawReadableStream>(
			(io::createFileStream(pathConfig, io::FileMode::Read |
				io::FileMode::ShareRead | io::FileMode::ShareWrite))));
	}

	//
	//
	//
	Variant compile(Variant vSources) const
	{
		std::cout << "Compiling..." << std::endl;
		auto vConfig = Dictionary({
			{"config", loadCompilerConfig()}
			});
		auto pCompiler = queryInterface<edr::policy::IPolicyCompiler>(
			createObject(CLSID_PolicyCompiler, vConfig));
		return pCompiler->compile(edr::policy::PolicyGroup::AutoDetect, vSources, false /* fActivate */);
	}

	//
	//
	//
	void build()
	{
		if (m_fVerbose)
			showParameters();

		// Check the required parameters
		if (m_srcFiles.empty())
			error::InvalidArgument(SL, "Missing source files").throwException();
		if (m_sOutFileName.empty())
			error::InvalidArgument(SL, "Missing output file").throwException();

		saveResult(compile(loadSources()));
	}

public:
	static bool g_fCrtlBreakIsUsed;

	//
	//
	//
	static BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
	{
		auto pApp = queryInterfaceSafe<Application>(getCatalogData("objects.application"));
		g_fCrtlBreakIsUsed = true;
		if (pApp)
			pApp->shutdown();
		return TRUE;
	}

	//
	//
	//
	ErrorCode main(Application* pApp) override
	{
		if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
			error::win::WinApiError(SL, "Failed to set CTRL+BREAK handler").throwException();

		CMD_TRY
		{
			std::cout << "========= Build started =========" << std::endl;
			build();
			std::cout << "======== Build succeeded ========" << std::endl;
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& e)
		{
			if (m_fVerbose)
				e.print();
			else
				std::cout << "ERROR: " << e.what() << std::endl;
			std::cout << "========= BUILD FAILED ==========" << std::endl;
		}
		return ErrorCode::OK;
	}

	//
	//
	//
	void initParams(clara::Parser& clp) override
	{
		clp += clara::Opt(m_sOutFileName, "filename")["-o"]["--out-file"].required()("set output file");
		clp += clara::Arg(m_srcFiles, "srcfile1 [srcfile2] ...").required()("list of source files");
		clp += clara::Opt(m_sCfgFileName, "filename")["-C"]["--cfg-file"]("set compiler config file");
		clp += clara::Opt(m_fVerbose)["-V"]["--verbose"]("set verbose on");
	}
};

bool AppMode_compile::g_fCrtlBreakIsUsed = false;

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_compile()
{
	return std::make_shared<AppMode_compile>();
}

} // namespace openEdr
