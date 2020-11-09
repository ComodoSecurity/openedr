//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (01.04.2019)
// Reviewer: Denis Bogdanov (18.04.2019)
//
///
/// @file Valkyrie Client declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
///
#pragma once
#include <net.hpp>

namespace cmd {
namespace cloud {
namespace valkyrie {

constexpr char c_sDefaultHost[] = "valkyrie.comodo.com";
constexpr Size c_nDefaultPort = 443;

//
// Valkyrie client class
//
class Client
{
public:
	static constexpr Size c_nFileMaxSize = 85 * 1024 * 1024; // Max file size for submit is 85MB.

	//
	// Constructor
	//
	Client(std::string_view sUrl, std::string_view sApiKey, std::string_view sEndpointId);

	//
	// Constructor
	//
	Client(std::string_view sHost, Size nPort, std::string_view sApiKey,
		std::string_view sEndpointId);

	//
	// Destructor
	//
	virtual ~Client();

	//
	//
	//
	Variant getBasicInfo(std::string_view sHash);

	//
	//
	//
	Variant submitToAutomaticAnalysis(ObjPtr<io::IReadableStream> pStream,
		std::filesystem::path pathFile, std::string_view sSubmitToken);

private:
	net::HttpClient m_http;
	std::string m_sApiKey;
	std::string m_sEndpointId;
};

} // namespace valkyrie
} // namespace cloud
} // namespace cmd

/// @} 
