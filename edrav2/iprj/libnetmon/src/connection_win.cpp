//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file Connection objects
///
#include "pch_win.h"
#include "connection_win.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "netmon"

namespace cmd {
namespace netmon {
namespace win {

//////////////////////////////////////////////////////////////////////////

using namespace nfapi;
using namespace ProtocolFilters;


//////////////////////////////////////////////////////////////////////////
//
// TcpConnection
//

//
//
//
TcpConnection::TcpConnection()
{
}

//
//
//
TcpConnection::~TcpConnection()
{
}

//
//
//
std::shared_ptr<const ConnectionInfo> TcpConnection::getInfo()
{
	return m_pInfo;
}

//
//
//
void TcpConnection::addParserFactory(ObjPtr<IObject> pObject)
{
	// Tune connection
	auto pParserFactory = queryInterface<ITcpParserFactory>(pObject);
	if (!pParserFactory->tuneConnection(m_pInfo))
	{
		LOGLVL(Debug, "ParserFactory <" << pParserFactory->getName() << "> has decline connection <" <<
			m_pInfo->sDisplayName << ">");
		return;
	}

	// Add ParserFactory
	{
		std::scoped_lock lock(m_mtxParsing);
		m_parserFactoryList.push_back(pParserFactory);
	}
}

//
//
//
void TcpConnection::ProcessParserFactories(ProtocolFilters::PFObject* pObject)
{
	if (m_parserFactoryList.empty())
		return;

	auto iterEnd = m_parserFactoryList.end();
	for (auto iter = m_parserFactoryList.begin(); iter != iterEnd;)
	{
		using ParserInfo = ITcpParserFactory::ParserInfo;
		auto parserInfo = (*iter)->createParser(m_pInfo, pObject);

		switch (parserInfo.eStatus)
		{
			// Ignored - do nothing
			case ParserInfo::Status::Ignored:
			{
				++iter;
				break;
			}
			// Created - add parser and erase ParserFactory
			case ParserInfo::Status::Created:
			{
				// Add parser
				if (!parserInfo.pParser)
					error::RuntimeError(SL, FMT("ParserFactory <" << (*iter)->getName() << " return empty parser.")).log();
				else
					m_parserList.push_back(parserInfo.pParser);

				// Remove this factory
				iter = m_parserFactoryList.erase(iter);
				break;
			}
			// Incompatible and others - just remove this factory
			default:
			{
				iter = m_parserFactoryList.erase(iter);
				break;
			}
		}
	}
}

//
//
//
void TcpConnection::dataAvailable(ProtocolFilters::PFObject* pObject)
{
	std::scoped_lock lock(m_mtxParsing);

	// Process factories
	ProcessParserFactories(pObject);

	// Process main parser
	if (m_pMainParser)
	{
		auto eStatus = m_pMainParser->dataAvailable(m_pInfo, pObject);
		if (eStatus == DataStatus::Incompatible)
			m_pMainParser.reset();
		return;
	}

	// Process parser-candidate
	if (m_parserList.empty())
		return;

	auto iterEnd = m_parserList.end();
	for (auto iter = m_parserList.begin(); iter != iterEnd;)
	{
		bool fMainParserIsDetected = false;
		auto eStatus = (*iter)->dataAvailable(m_pInfo, pObject);
		switch (eStatus)
		{
			case DataStatus::Ignored:
			{
				++iter;
				break;
			}
			case DataStatus::Processed:
			{
				fMainParserIsDetected = true;
				break;
			}
			// DataStatus::Incompatible and others - remove parser
			default:
			{
				iter = m_parserList.erase(iter);
				break;
			}
		}

		// Set main parser and remove candidates
		if (fMainParserIsDetected)
		{
			m_pMainParser = (*iter);
			m_parserList.clear();
			break;
		}
	}
}

//
//
//
PF_DATA_PART_CHECK_RESULT convertDataPartStatus(const DataPartStatus eStatus)
{
	switch (eStatus)
	{
	case DataPartStatus::NeedMoreData:
		return DPCR_MORE_DATA_REQUIRED;
	case DataPartStatus::NeedFullObject:
		return DPCR_FILTER_READ_ONLY;
	case DataPartStatus::BypassObject:
		return DPCR_BYPASS;
	default:
		return DPCR_BYPASS;
	};
}

//
//
//
PF_DATA_PART_CHECK_RESULT TcpConnection::dataPartAvailable(ProtocolFilters::PFObject* pObject)
{
	std::scoped_lock lock(m_mtxParsing);

	// Process factories
	ProcessParserFactories(pObject);

	// Process main parser
	if (m_pMainParser)
	{
		auto eStatus = m_pMainParser->dataPartAvailable(m_pInfo, pObject);
		if (eStatus == DataPartStatus::Incompatible)
		{
			m_pMainParser.reset();
			return DPCR_BYPASS;
		}
		if (eStatus == DataPartStatus::Ignored)
		{
			return DPCR_BYPASS;
		}
		return convertDataPartStatus(eStatus);
	}

	// Process parser-candidate
	if (m_parserList.empty())
		return DPCR_BYPASS;

	auto iterEnd = m_parserList.end();
	for (auto iter = m_parserList.begin(); iter != iterEnd;)
	{
		auto eStatus = (*iter)->dataPartAvailable(m_pInfo, pObject);
		if (eStatus == DataPartStatus::Ignored)
		{
			++iter;
		}
		else if (eStatus == DataPartStatus::Incompatible)
		{
			iter = m_parserList.erase(iter);
		}
		else
		{
			m_pMainParser = (*iter);
			m_parserList.clear();
			return convertDataPartStatus(eStatus);
		}
	}

	// return DPCR_BYPASS if no one parser supports object
	return DPCR_BYPASS;
}

//
//
//
ObjPtr<TcpConnection> TcpConnection::create(std::shared_ptr<const ConnectionInfo> pInfo)
{
	auto pThis = createObject<TcpConnection>();
	pThis->m_pInfo = pInfo;

	return pThis;
}

//
//
//
bool TcpConnection::hasParsers()
{
	std::scoped_lock lock(m_mtxParsing);
	if (m_parserFactoryList.empty() && m_parserList.empty() && !m_pMainParser)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
//
// UdpConnection
//


//
//
//
UdpConnection::UdpConnection()
{
}

//
//
//
UdpConnection::~UdpConnection()
{
}

//
//
//
std::shared_ptr<const ConnectionInfo> UdpConnection::getInfo()
{
	return m_pInfo;
}

//
//
//
bool UdpConnection::hasParsers()
{
	std::scoped_lock lock(m_mtxParsing);
	return m_pMainParser != nullptr || !m_parserList.empty();
}

//
//
//
ObjPtr<UdpConnection> UdpConnection::create(std::shared_ptr<const ConnectionInfo> pInfo)
{
	auto pThis = createObject<UdpConnection>();
	pThis->m_pInfo = pInfo;
	return pThis;
}


//
//
//
void UdpConnection::addParserFactory(ObjPtr<IObject> pObject)
{
	// Create Parser
	auto pParserFactory = queryInterface<IUdpParserFactory>(pObject);
	auto pParser = pParserFactory->createParser(m_pInfo);
	if (pParser == nullptr)
		return;

	// Add Parser
	{
		std::scoped_lock lock(m_mtxParsing);
		m_parserList.push_back(pParser);
	}
}

//
//
//
void UdpConnection::notifyData(bool fSend, const uint8_t* pBuf, size_t nLen)
{
	std::scoped_lock lock(m_mtxParsing);

	// Process main parser
	if (m_pMainParser)
	{
		auto eStatus = m_pMainParser->notifyData(fSend, m_pInfo, pBuf, nLen);
		if (eStatus == DataStatus::Incompatible)
			m_pMainParser.reset();
		return;
	}

	// Process parser-candidate
	if (m_parserList.empty())
		return;

	auto iterEnd = m_parserList.end();
	for (auto iter = m_parserList.begin(); iter != iterEnd;)
	{
		bool fMainParserIsDetected = false;
		auto eStatus = (*iter)->notifyData(fSend, m_pInfo, pBuf, nLen);
		switch (eStatus)
		{
			case DataStatus::Ignored:
			{
				++iter;
				break;
			}
			case DataStatus::Processed:
			{
				fMainParserIsDetected = true;
				break;
			}
			// DataStatus::Incompatible and others - remove parser
			default:
			{
				iter = m_parserList.erase(iter);
				break;
			}
		}

		// Set main parser and remove candidates
		if (fMainParserIsDetected)
		{
			m_pMainParser = (*iter);
			m_parserList.clear();
			break;
		}
	}
}

//
//
//
void UdpConnection::notifyClose()
{
}

} // namespace win
} // namespace netmon
} // namespace cmd
