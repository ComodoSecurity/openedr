//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
/// @file Http parser
/// @addtogroup netmon Network Monitor library
/// @{

#include "pch_win.h"
#include "connection_win.h"
#include "parser_ftp_win.h"

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


///
/// It parses FTP command (request).
///
/// @param sData - request string with finishing CRLF.
/// @param info - result data.
/// @return The function returns `true` if it has successfully parsed.
///
static bool parseFtpRequest(std::string_view sData, FtpRequest& info)
{
	// Test unknown encoding
	if (sData.find('\0') != std::string_view::npos)
		return false;

	// Remove last CRLF
	auto nNotCrlfPos = sData.find_last_not_of("\r\n");
	if (nNotCrlfPos == std::string_view::npos)
		return false;
	size_t nRemovedSize = std::min<size_t>(sData.size() - (nNotCrlfPos + 1), 2);
	sData = sData.substr(0, sData.size() - nRemovedSize);

	size_t nCrlfSize = 0;
	for (; nCrlfSize < 2; ++nCrlfSize)
	{
		const unsigned char ch = sData[sData.size() - nCrlfSize - 1];
		if (ch != '\r' && ch != '\n')
			break;
	}
	sData = sData.substr(0, sData.size() - nCrlfSize);


	// Calc command and arg size
	size_t nDataSize = sData.size();
	size_t nCmdSize = 0;
	if (nDataSize < 3)
		return false;
	if (nDataSize == 3 || sData[3] == ' ')
		nCmdSize = 3;
	else if (nDataSize == 4 || sData[4] == ' ')
		nCmdSize = 4;
	else
		return false;

	// Check command, make uppercase and convert to enum
	union
	{
		unsigned char sVal[sizeof(uint32_t)];
		uint32_t nVal;
	} helper;

	// Check minimal FTP command size
	helper.sVal[3] = ' ';
	memcpy(&helper.sVal, sData.data(), nCmdSize);
	// make uppercase and convert to enum
	for (size_t i = 0; i < nCmdSize; ++i)
	{
		const unsigned char origCh = helper.sVal[i];

		if (origCh >= 'a' && origCh <= 'z')
			helper.sVal[i] = origCh - 'a' + 'A';
		else if (origCh >= 'A' && origCh <= 'Z')
			continue;
		else
			return false;
	}
	// convert to enum FtpCmd
	FtpCmd eCmd = FtpCmd(ntohl(helper.nVal));

	// extract and check arguments
	std::string_view sArgs;
	if (nDataSize > nCmdSize + 1)
		sArgs = sData.substr(nCmdSize + 1);
	// Test unknown encoding
	if (sData.find('\0') != std::string_view::npos)
		return false;


	info.eCmd = eCmd;
	info.sArgs = sArgs;
	return true;
}

///
/// It parses FTP server response.
///
/// @param sData - response string with finishing CRLF.
/// @param info - result data.
/// @return The function returns `true` if it has successfully parsed.
///
static bool parseFtpResponse(std::string_view sData, FtpResponse& info)
{
	// Test unknown encoding
	if (sData.find('\0') != std::string_view::npos)
		return false;

	// Remove last CRLF
	auto nNotCrlfPos = sData.find_last_not_of("\r\n");
	if (nNotCrlfPos == std::string_view::npos)
		return false;
	size_t nRemovedSize = std::min<size_t>(sData.size() - (nNotCrlfPos + 1), 2);
	sData = sData.substr(0, sData.size() - nRemovedSize);

	// extact code
	size_t nCode = 0;
	if (sData.size() < 3)
		return false;
	size_t nFactor = 100;
	for (size_t i = 0; i < 3; ++i)
	{
		const unsigned char origCh = sData[i];
		if (origCh < '0' || origCh > '9')
			return false;
		nCode += (origCh - '0') * nFactor;
		nFactor /= 10;
	}

	// extract and check arguments
	std::string_view sArgs;
	if (sData.size() > 4)
		sArgs = sData.substr(4);

	info.nCode = nCode;
	info.sArgs = sArgs;
	return true;
}

//////////////////////////////////////////////////////////////////////////
//
// FtpParser
//

//
//
//
template<typename FnAddData>
void FtpParser::sendEvent(RawEvent eRawEvent, std::shared_ptr<const ConnectionInfo> pConInfo, FnAddData fnAddData)
{
	if (!m_pNetMonController->isEventEnabled(eRawEvent))
		return;

	if(eRawEvent != RawEvent::NETMON_REQUEST_DATA_FTP)
		error::InvalidArgument("Invalid raw event type").throwException();

	Dictionary vEvent {
		{"baseType", Event::LLE_NETWORK_REQUEST_DATA},
		{"rawEventId", createRaw(CLSID_NetworkMonitorController, (uint32_t)eRawEvent)},
		{"tickTime", ::GetTickCount64()},
		{"process", Dictionary {{"pid", pConInfo->nPid}} },
		{"connection", pConInfo->vConnection },
		{"protocol", AppProtocol::FTP},
	};

	bool fSendEvent = fnAddData(vEvent);

	if (fSendEvent)
		m_pNetMonController->sendEvent(vEvent);
}

//
//
//
void FtpParser::processFtpControlData(std::shared_ptr<const ConnectionInfo> pConInfo, std::string_view sData, bool fIsRequest)
{
	if (fIsRequest)
	{
		m_nLastRequest.eCmd = FtpCmd::Invalid;
		if (!parseFtpRequest(sData, m_nLastRequest))
			return;

		switch (m_nLastRequest.eCmd)
		{
		// Process directory change
		case FtpCmd::CWD:
		case FtpCmd::XCWD:
		case FtpCmd::CDUP:
		case FtpCmd::XCUP:
		case FtpCmd::PWD:
		case FtpCmd::XPWD:
		case FtpCmd::REIN:
			break; // wait answer
		case FtpCmd::RETR:
		{
			// Process request file
			std::string sFilePath = FtpPath(m_curPath).append(FtpPath(m_nLastRequest.sArgs)).getStr();
			std::string sUrl = "ftp://" + pConInfo->remote.sIp + sFilePath;

			// Analize object
			sendEvent(RawEvent::NETMON_REQUEST_DATA_HTTP, pConInfo, [&sUrl](Dictionary& vEvent) -> bool
			{
				vEvent.put("url", sUrl);
				vEvent.put("secured", false);
				vEvent.put("command", "RETR");
				return true;
			});

			//LOGINF("ftp cmd: load file <" << sFilePath << ">");
			m_nLastRequest.eCmd = FtpCmd::Invalid;
			break;
		}
		default:
			// Skip command
			m_nLastRequest.eCmd = FtpCmd::Invalid;
		}
	}
	else
	{
		// If command is not waited
		if (m_nLastRequest.eCmd == FtpCmd::Invalid)
			return;
		FtpResponse info;
		if (!parseFtpResponse(sData, info))
			return;

		// If fail or invalid is returned, request is skipped
		if (info.nCode >= 400 || info.nCode < 100)
		{
			m_nLastRequest.eCmd = FtpCmd::Invalid;
			return;
		}

		// If is not success, wait next responce
		if (info.nCode < 200 || info.nCode >= 300)
			return;

		// Process successful commmand
		switch (m_nLastRequest.eCmd)
		{
		//
		case FtpCmd::CWD:
		case FtpCmd::XCWD:
			m_curPath.append(FtpPath(m_nLastRequest.sArgs));
			//LOGINF("ftp: change dir to <" << m_curPath.getStr() << ">");
			break;
		case FtpCmd::CDUP:
		case FtpCmd::XCUP:
			m_curPath.append(FtpPath(".."));
			//LOGINF("ftp: change dir to <" << m_curPath.getStr() << ">");
			break;
		case FtpCmd::PWD:
		case FtpCmd::XPWD:
		{
			// Parse PWD answer
			if(info.nCode != 257)
				break;
			if(info.sArgs.size() < 2)
				break;
			if(info.sArgs[0] != '\"')
				break;

			std::string sPath = info.sArgs.substr(1);
			size_t nPos = 0;
			while (true)
			{
				auto nQuotePos = sPath.find('\"', nPos);
				// Invalid. Last " must exist.
				if (nQuotePos == std::string::npos)
				{
					sPath.clear();
					break;
				}

				// Last " was found
				if (sPath.size() <= nQuotePos + 1 || sPath[nQuotePos + 1] != '\"')
				{
					sPath.resize(nQuotePos);
					break;
				}

				// "" was found
				sPath.erase(nQuotePos, 1);
				nPos = nQuotePos + 1;
			}

			// Invalid path
			if (sPath.empty())
				break;

			m_curPath = FtpPath(sPath);

			//LOGINF("ftp: print current dir <" << m_curPath.getStr() << ">");
			break;
		}
		case FtpCmd::REIN:
			m_curPath = FtpPath("/");
			//LOGINF("ftp: reinitialize");
			break;
		}
		m_nLastRequest.eCmd = FtpCmd::Invalid;
	}
}

//
//
//
DataStatus FtpParser::dataAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject)
{
	auto eObjectType = pObject->getType();
	// FTP data is always processed
	if (checkFilterType(FT_FTP_DATA, eObjectType))
		return DataStatus::Processed;
	// Stop processing if it is not FTP object
	if (!checkFilterType(FT_FTP, eObjectType))
		return DataStatus::Incompatible;

	if (eObjectType != OT_FTP_COMMAND && eObjectType != OT_FTP_RESPONSE)
		return DataStatus::Processed;

	bool fIsRequest = eObjectType == OT_FTP_COMMAND;

	auto sData = convertStreamToString(pObject, 0 /*It is the only stream*/, true);
	if(!sData.has_value())
		return DataStatus::Processed;

	processFtpControlData(pConInfo, sData.value(), fIsRequest);

	return DataStatus::Processed;
}

//
//
//
DataPartStatus FtpParser::dataPartAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject)
{
	auto eObjectType = pObject->getType();
	// Data is always bypassed
	if (checkFilterType(FT_FTP_DATA, eObjectType))
		return DataPartStatus::BypassObject;
	// Stop processing if it is not FTP object
	if (!checkFilterType(FT_FTP, eObjectType))
		return DataPartStatus::Incompatible;

	// control request and response pass to dataAvailable()
	if (eObjectType == OT_FTP_COMMAND || eObjectType == OT_FTP_RESPONSE)
		return DataPartStatus::NeedFullObject;

	// Other FTP obects (?) are bypassed
	return DataPartStatus::BypassObject;
}

//
//
//
ObjPtr<FtpParser> FtpParser::create(ObjPtr<INetworkMonitorController> pNetMonController)
{
	auto pThis = createObject<FtpParser>();
	pThis->m_pNetMonController = pNetMonController;
	return pThis;
}


//////////////////////////////////////////////////////////////////////////
//
// FtpParserFactory
//

void FtpParserFactory::finalConstruct(Variant vConfig)
{
	m_pNetMonController = queryInterface<INetworkMonitorController>(vConfig.get("controller"));
}

//
//
//
bool FtpParserFactory::tuneConnection(std::shared_ptr<const ConnectionInfo> pConInfo)
{
	if (!m_pNetMonController->isEventEnabled(RawEvent::NETMON_REQUEST_DATA_FTP))
		return false;

	if (!pf_addFilter(pConInfo->nInternalId, FT_FTP, FF_READ_ONLY_OUT | FF_READ_ONLY_IN))
		return false;
	return true;
}

//
//
//
std::string_view FtpParserFactory::getName()
{
	return "FTP-parser";
}

//
//
//
ITcpParserFactory::ParserInfo FtpParserFactory::createParser(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject)
{
	auto eObjectType = pObject->getType();
	ENDPOINT_ID id = pConInfo->nInternalId;
	if (!checkFilterType(FT_FTP, eObjectType))
	{
		if (pf_isFilterActive(id, FT_FTP))
			return { ParserInfo::Status::Ignored };
		else
			return { ParserInfo::Status::Incompatible };
	}

	return { ParserInfo::Status::Created, FtpParser::create(m_pNetMonController) };
}

} // namespace win
} // namespace netmon
} // namespace openEdr

/// @}
