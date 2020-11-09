//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (01.04.2019)
// Reviewer: Denis Bogdanov (18.04.2019)
//
///
/// @file Valkyrie Client objects implementation
///
/// @addtogroup cloud Cloud communication library
/// @{
///
#include "pch.h"
#include "valkyrieclient.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "valkyrie"

namespace cmd {
namespace cloud {
namespace valkyrie {

// FIXME: Telling the truth I don't understand the necessity of this class.
// You only do post requests here and event don't convert results. 
// You can do in in service class.


//
//
//
Client::Client(std::string_view sUrl, std::string_view sApiKey, std::string_view sEndpointId)
	: m_http(std::string(sUrl)), m_sApiKey(sApiKey), m_sEndpointId(sEndpointId)
{
}

//
//
//
Client::Client(std::string_view sHost, Size nPort, std::string_view sApiKey,
	std::string_view sEndpointId)
	: m_http(net::combineUrl(Dictionary({
		{"scheme", "https"},
		{"host", sHost},
		{"port", std::to_string(nPort)}
		}))),
	m_sApiKey(sApiKey), m_sEndpointId(sEndpointId)
{
}

//
//
//
Client::~Client()
{
}

//
//
//
Variant Client::getBasicInfo(std::string_view sHash)
{
	auto vData = Dictionary({
		{"api_key", m_sApiKey},
		{"endpoint_id", m_sEndpointId},
		{"sha1", sHash}
		});
	return m_http.post("/fvs_basic_info", vData, Dictionary({ {"contentType", "multipart/form-data"} }));
}

//
//
//
Variant Client::submitToAutomaticAnalysis(ObjPtr<io::IReadableStream> pStream,
	std::filesystem::path pathFile, std::string_view sSubmitToken)
{
	if (pStream->getSize() > c_nFileMaxSize)
		error::InvalidArgument(SL, FMT("Max file size is 85MB")).throwException();

	auto vData = Dictionary({
		{"api_key", m_sApiKey},
		{"endpoint_id", m_sEndpointId},
		{"file_path", pathFile.native()},
		{"submit_token", sSubmitToken},
		{"file_data", Dictionary({
			{"stream", pStream},
			{"filename", pathFile.filename().c_str()}
			})}
		});
	return m_http.post("/fvs_submit_auto", vData, Dictionary({ {"contentType", "multipart/form-data"} }));
}

} // namespace valkyrie
} // namespace cloud
} // namespace cmd

/// @}
