//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file Http parser
///
/// @addtogroup netmon Network Monitor library
/// @{
#include "pch_win.h"
#include "connection_win.h"
#include "parser_http_win.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "netmon"

namespace openEdr {
namespace netmon {
namespace win {

//////////////////////////////////////////////////////////////////////////

using namespace nfapi;
using namespace ProtocolFilters;

//////////////////////////////////////////////////////////////////////////
//
// Utils
//

//
//
//
enum class HttpVersion
{
	INVALID = 0, // message has error, but possibly HTTP
	V09 = 9,
	V10 = 10,
	V11 = 11,
};

//
//
//
enum class HttpMethod
{
	Get = 0,
	Post = 1,
	Head = 2,
	Options = 3,
	Put = 4,
	Delete = 5,
	Trace = 6,
	Connect = 7,
	Unknown = 1000,
};

//
//
//
struct HttpRequestInfo
{
	HttpMethod eMethod;  // HTTP method
	std::string sMethod; // Origin method string

	std::string sPath; // Path from Status line
	std::string sUrl; // Full URL
};

//
//
//
bool parseHttpRequestInfo(PFObject* pObject, HttpRequestInfo& result)
{
	if (pObject->getType() != OT_HTTP_REQUEST)
		return false;

	// get status string
	auto sStatusBuffers = convertStreamToString(pObject, HS_STATUS, true);
	if(!sStatusBuffers.has_value())
		return false;
	std::string_view sStatus = sStatusBuffers.value();

	// Parse method
	auto nMethodStart = sStatus.find_first_not_of(' ');
	if(nMethodStart == std::string_view::npos)
		return false;
	auto nMethodEnd = sStatus.find_first_of(' ', nMethodStart);
	if (nMethodEnd == std::string_view::npos)
		return false;
	std::string_view sMethod = sStatus.substr(nMethodStart, nMethodEnd-nMethodStart);

	// Parse path
	auto nPathStart = sStatus.find_first_not_of(' ', nMethodEnd);
	if (nPathStart == std::string_view::npos)
		return false;
	auto nPathEnd = sStatus.find_first_of(' ', nPathStart);
	if (nPathEnd == std::string_view::npos)
		return false;
	std::string_view sPath = sStatus.substr(nPathStart, nPathEnd - nPathStart);

	// Convert method
	HttpMethod eMethod = HttpMethod::Unknown;
	if (sMethod == "GET") eMethod = HttpMethod::Get;
	else if (sMethod == "POST") eMethod = HttpMethod::Post;
	else if (sMethod == "HEAD") eMethod = HttpMethod::Head;
	else if (sMethod == "OPTIONS") eMethod = HttpMethod::Options;
	else if (sMethod == "PUT") eMethod = HttpMethod::Put;
	else if (sMethod == "DELETE") eMethod = HttpMethod::Delete;
	else if (sMethod == "TRACE") eMethod = HttpMethod::Trace;
	else if (sMethod == "CONNECT") eMethod = HttpMethod::Connect;

	result.eMethod = eMethod;
	result.sMethod = sMethod;
	result.sPath = sPath;

	result.sUrl = sPath;
	auto sPathPrefix = string::convertToLower(sPath.substr(0, 4));
	do
	{
		if (string::startsWith(sPathPrefix, "http") || string::startsWith(sPathPrefix, "ftp"))
			break;

		PFHeader header;
		if (!pf_readHeader(pObject->getStream(HS_HEADER), &header))
			break;
		PFHeaderField* pField = header.findFirstField("Host");
		if (!pField)
			break;

		result.sUrl = "http://";
		result.sUrl += string::convertToLower(pField->value());
		result.sUrl += sPath;
	} while (false);

	return true;
}

//////////////////////////////////////////////////////////////////////////
//
// HttpParser
//

//
//
//
template<typename FnAddData>
void HttpParser::sendEvent(RawEvent eRawEvent, std::shared_ptr<const ConnectionInfo> pConInfo, FnAddData fnAddData)
{
	if (!m_pNetMonController->isEventEnabled(eRawEvent))
		return;

	if(eRawEvent != RawEvent::NETMON_REQUEST_DATA_HTTP)
		error::InvalidArgument("Invalid raw event type").throwException();

	Dictionary vEvent {
		{"baseType", Event::LLE_NETWORK_REQUEST_DATA},
		{"rawEventId", createRaw(CLSID_NetworkMonitorController, (uint32_t)eRawEvent)},
		{"tickTime", ::GetTickCount64()},
		{"process", Dictionary {{"pid", pConInfo->nPid}} },
		{"connection", pConInfo->vConnection },
		{"protocol", AppProtocol::HTTP},
	};

	bool fSendEvent = fnAddData(vEvent);
	if (fSendEvent)
		m_pNetMonController->sendEvent(vEvent);
}


//
//
//
DataStatus HttpParser::dataAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject)
{
	auto eObjectType = pObject->getType();
	if (!checkFilterType(FT_HTTP, eObjectType))
		return DataStatus::Incompatible;

	if (eObjectType != OT_HTTP_REQUEST)
		return DataStatus::Processed;

	HttpRequestInfo requestInfo;
	if(!parseHttpRequestInfo(pObject, requestInfo))
		return DataStatus::Processed;

	// Send GET and POST only
	if(requestInfo.eMethod != HttpMethod::Get && requestInfo.eMethod != HttpMethod::Post)
		return DataStatus::Processed;

	// Analize object
	sendEvent(RawEvent::NETMON_REQUEST_DATA_HTTP, pConInfo, [&requestInfo](Dictionary& vEvent) -> bool
	{
		vEvent.put("url", requestInfo.sUrl);
		vEvent.put("secured", false);
		vEvent.put("command", requestInfo.sMethod);
		return true;
	});

	return DataStatus::Processed;
}

//
//
//
DataPartStatus HttpParser::dataPartAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject)
{
	auto eObjectType = pObject->getType();
	if (!checkFilterType(FT_HTTP, eObjectType))
		return DataPartStatus::Incompatible;

	if (eObjectType == OT_HTTP_REQUEST)
		return DataPartStatus::NeedFullObject;

	return DataPartStatus::BypassObject;
}

ObjPtr<HttpParser> HttpParser::create(ObjPtr<INetworkMonitorController> pNetMonController)
{
	auto pThis = createObject<HttpParser>();
	pThis->m_pNetMonController = pNetMonController;
	return pThis;
}


//////////////////////////////////////////////////////////////////////////
//
// HttpParserFactory
//

void HttpParserFactory::finalConstruct(Variant vConfig)
{
	m_pNetMonController = queryInterface<INetworkMonitorController>(vConfig.get("controller"));
}

//
//
//
bool HttpParserFactory::tuneConnection(std::shared_ptr<const ConnectionInfo> pConInfo)
{
	if (!m_pNetMonController->isEventEnabled(RawEvent::NETMON_REQUEST_DATA_HTTP))
		return false;

	if (!pf_addFilter(pConInfo->nInternalId, FT_HTTP, FF_READ_ONLY_OUT | FF_READ_ONLY_IN | 
		FF_HTTP_KEEP_PIPELINING | FF_HTTP_BLOCK_SPDY))
		return false;
	return true;
}

//
//
//
std::string_view HttpParserFactory::getName()
{
	return "HTTP-parser";
}

//
//
//
ITcpParserFactory::ParserInfo HttpParserFactory::createParser(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject)
{
	auto eObjectType = pObject->getType();
	ENDPOINT_ID id = pConInfo->nInternalId;
	if (!checkFilterType(FT_HTTP, eObjectType))
	{
		if (pf_isFilterActive(id, FT_HTTP))
			return { ParserInfo::Status::Ignored };
		else
			return { ParserInfo::Status::Incompatible };
	}

	return { ParserInfo::Status::Created, HttpParser::create(m_pNetMonController) };
}

} // namespace win
} // namespace netmon
} // namespace openEdr

/// @}
