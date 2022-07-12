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
class AppMode_file : public IApplicationMode
{
private:
	std::string m_sSubMode;
	std::string m_sFileName;
	std::string m_sOutFileName;
	std::string m_sEncryption;
	std::string m_sCompression;

public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		if (m_sSubMode == "info")
		{
			auto pFileDP = queryService("fileDataProvider");
			auto vFile = execCommand(pFileDP, "getFileInfo",
				Dictionary({ {"path", m_sFileName} }));
			std::cout << "File info:" << std::endl;
			std::cout << variant::serializeToJson(vFile, variant::JsonFormat::Pretty) << std::endl;
		}
		else if (m_sSubMode == "sign")
		{
			auto pSignDP = queryService("signatureDataProvider");
			auto vSign = execCommand(pSignDP, "getSignInfo",
				Dictionary({ {"path", m_sFileName} }));
			std::cout << "File signature info:" << std::endl;
			std::cout << vSign.print() << std::endl;
		}
		else if (m_sSubMode == "encrypt")
		{
			auto pInFile = io::createFileStream(m_sFileName, io::FileMode::Read | io::FileMode::ShareRead);
			ObjPtr<io::IWritableStream> pOutFile;
			if (!m_sOutFileName.empty())
				pOutFile = queryInterface<io::IWritableStream>(io::createFileStream(
					m_sOutFileName, io::FileMode::Write | io::FileMode::Truncate | io::FileMode::ShareRead));

			std::cout << "Encrypt file <" << m_sFileName << "> -> <" << m_sOutFileName <<
				">, compression <" << m_sCompression << ">, encryption <" << m_sEncryption << ">" << std::endl;

			auto pStream = queryInterface<io::IRawWritableStream>(
				createObject(CLSID_StreamSerializer, Dictionary({
					{"stream", pOutFile},
					{"compression", m_sCompression},
					{"encryption", m_sEncryption}
				})));
			io::write(pStream, pInFile);
		}
		else if (m_sSubMode == "decrypt")
		{
			auto pInFile = io::createFileStream(m_sFileName, io::FileMode::Read | io::FileMode::ShareRead);
			ObjPtr<io::IWritableStream> pOutFile;
			if (!m_sOutFileName.empty())
				pOutFile = queryInterface<io::IWritableStream>(io::createFileStream(
					m_sOutFileName, io::FileMode::Write | io::FileMode::Truncate | io::FileMode::ShareRead));

			std::cout << "Decrypt file <" << m_sFileName << "> -> <" << m_sOutFileName << ">" << std::endl;

			auto pStream = queryInterface<io::IRawReadableStream>(
				createObject(CLSID_StreamDeserializer, Dictionary({
					{"stream", pInFile},
				})));
			io::write(pOutFile, pStream);
		}
		return ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{
		clp += clara::Arg(m_sSubMode, "info|sign|encrypt|decrypt") ("working mode");
		clp += clara::Opt(m_sEncryption, "[aes|rc4]")["-e"]["--encryption"]("set encryption type");
		clp += clara::Opt(m_sCompression, "[lzma|deflate]")["-c"]["--compression"]("set compression type");
		clp += clara::Opt(m_sFileName, "filename")["-f"]["--file"]("set file name");
		clp += clara::Opt(m_sOutFileName, "filename")["-o"]["--outfile"]("set output file name");
	}
};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_file()
{
	return std::make_shared<AppMode_file>();
}

} // namespace cmd