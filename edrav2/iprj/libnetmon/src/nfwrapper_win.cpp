//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file Wrapper of NetFilterSDK and ProtocolFilter libraries
///
#include "pch_win.h"
#include "connection_win.h"
#include "nfwrapper_win.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "netmon"

namespace cmd {
namespace netmon {
namespace win {

//////////////////////////////////////////////////////////////////////////

using namespace nfapi;
using namespace ProtocolFilters;

constexpr Size c_nMaxAddressSize = 80;

//
// Create address descriptor
//
void createAddressInfo(AddressInfo& result, const unsigned char(&addrInfo)[NF_MAX_ADDRESS_LENGTH], AddressFamily eFamily)
{
	// Extract ip and port
	uint16_t nPort = 0;
	char sIp[c_nMaxAddressSize] = {};
	switch (eFamily)
	{
		case AddressFamily::Inet6:
		{
			auto pSockAddr = (sockaddr_in6*)&addrInfo;
			nPort = ntohs(pSockAddr->sin6_port);
			if (nullptr == inet_ntop(AF_INET6, &pSockAddr->sin6_addr, sIp, std::size(sIp)))
				error::win::WinApiError(SL, WSAGetLastError(), "Can't extract ip address").throwException();
			break;
		}
		case AddressFamily::Inet:
		{
			auto pSockAddr = (sockaddr_in*)&addrInfo;
			nPort = ntohs(pSockAddr->sin_port);
			if (nullptr == inet_ntop(AF_INET, &pSockAddr->sin_addr, sIp, std::size(sIp)))
				error::win::WinApiError(SL, WSAGetLastError(), "Can't extract ip address").throwException();
			break;
		}
		default:
			error::RuntimeError(SL, FMT("Invalid ip family: " << (size_t)eFamily)).throwException();
	}

	result.sIp = sIp;
	result.nPort = nPort;
}

//
// Create TCP connection info
//
std::shared_ptr<ConnectionInfo> createConnectionInfo(ENDPOINT_ID id, Direction eDirection, PNF_TCP_CONN_INFO pConnInfo)
{
	auto pInfo = std::make_shared<ConnectionInfo>();

	pInfo->eIpProtocol = IpProtocol::Tcp;
	pInfo->eDirection = eDirection;
	pInfo->eFamily = AddressFamily(pConnInfo->ip_family);
	// Set System pid if it is 0
	pInfo->nPid = pConnInfo->processId == 0 ? 4 : pConnInfo->processId;
	pInfo->nInternalId = id;
	pInfo->nTickTime = getTickCount();

	createAddressInfo(pInfo->local, pConnInfo->localAddress, pInfo->eFamily);
	createAddressInfo(pInfo->remote, pConnInfo->remoteAddress, pInfo->eFamily);

	pInfo->sDisplayName = getDisplayString(*pInfo);
	pInfo->vConnection = convertConnectionInfo(*pInfo, ConnectionStatus::Success);

	return pInfo;
}

//
// Create UDP connection descriptor
//
std::shared_ptr<ConnectionInfo> createConnectionInfo(ENDPOINT_ID id, Direction eDirection,
	PNF_UDP_CONN_INFO pConnInfo, const unsigned char* pRemoteAddress)
{
	auto pInfo = std::make_shared<ConnectionInfo>();

	pInfo->eIpProtocol = IpProtocol::Udp;
	pInfo->eDirection = eDirection;
	pInfo->eFamily = AddressFamily(pConnInfo->ip_family);
	// Set System pid if it is 0
	pInfo->nPid = pConnInfo->processId == 0 ? 4 : pConnInfo->processId;
	pInfo->nInternalId = id;
	pInfo->nTickTime = getTickCount();

	createAddressInfo(pInfo->local, pConnInfo->localAddress, pInfo->eFamily);
	unsigned char remoteAddress[NF_MAX_ADDRESS_LENGTH];
	memcpy(remoteAddress, pRemoteAddress, sizeof(remoteAddress));
	createAddressInfo(pInfo->remote, remoteAddress, pInfo->eFamily);

	pInfo->sDisplayName = getDisplayString(*pInfo);
	pInfo->vConnection = convertConnectionInfo(*pInfo, ConnectionStatus::Success);

	return pInfo;
}


//
// Safe disable filtering
//
void disableTcpFilteringSafe(ENDPOINT_ID id)
{
	if(pf_canDisableFiltering(id))
		nf_tcpDisableFiltering(id);
}

//////////////////////////////////////////////////////////////////////////
//
// NF_EventHandler callbacks
//

//
//
//
void NetFilterWrapper::threadStart()
{
	LOGLVL(Detailed, "Start worker thread");
}

//
//
//
void NetFilterWrapper::threadEnd()
{
	LOGLVL(Detailed, "Stop worker thread");
}
	
//////////////////////////////////////////////////////////////////////////
//
// TCP events
//

//
//
//
void NetFilterWrapper::tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	// Create delayed event for failed connection
	CMD_TRY
	{
		// Process repeated call of connect()
		sendFailedConnection(id);

		// Add event into list
		auto pConnectionInfo = createConnectionInfo(id, Direction::Out, pConnInfo);
		addConnectionRequest(pConnectionInfo, c_nTcpConnectTimeout);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Can't create connection event from process <" << pConnInfo->processId << ">"));
	}
}

//
//
//
void NetFilterWrapper::tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	CMD_TRY
	{
		Direction eDirection;
		switch (pConnInfo->direction)
		{
		case NF_D_OUT:
			eDirection = Direction::Out;
			break;
		case NF_D_IN:
			eDirection = Direction::In;
			break;
		default:
			disableTcpFilteringSafe(id);
			return;
		}

		// Remove connecion request
		removeConnectionRequest(id);

		// Notify
		auto pInfo = createConnectionInfo(id, eDirection, pConnInfo);
		auto pConnection = TcpConnection::create(pInfo);
		m_pNetMonController->notifyNewConnection(pConnection);
		
		// Skip connection without parsers
		if (!pConnection->hasParsers())
		{
			disableTcpFilteringSafe(id);
			return;
		}

		// Add connection to map
		{
			std::scoped_lock lock(m_mtxConnections);
			m_mapTcpConnections[id] = pConnection;
		}
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Error in process tcpConnected() process <" << pConnInfo->processId << ">"));
	}
}

//
//
//
void NetFilterWrapper::tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	CMD_TRY
	{
		sendFailedConnection(id);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception & e)
	{
		e.log(SL, FMT("Error in process tcpClosed() process <" << pConnInfo->processId << ">"));
	}
}
	
//
//
//
void NetFilterWrapper::dataAvailableInt(nfapi::ENDPOINT_ID id, ProtocolFilters::PFObject* pObject)
{
	CMD_TRY
	{
		// Find connection
		auto pConnection = getTcpConnection(id);
		if (!pConnection)
		{
			disableTcpFilteringSafe(id);
			return;
		}

		pConnection->dataAvailable(pObject);

		// Remove Connection if it does not have parsers
		if (!pConnection->hasParsers())
		{
			removeTcpConnection(id);
			disableTcpFilteringSafe(id);
		}

	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Error in process dataAvailable() connection <" << id << ">"));
	}
}

//
//
//
void NetFilterWrapper::dataAvailable(nfapi::ENDPOINT_ID id, PFObject* pObject)
{
	dataAvailableInt(id, pObject);
	pf_postObject(id, pObject);
}

//
//
//
PF_DATA_PART_CHECK_RESULT NetFilterWrapper::dataPartAvailable(ENDPOINT_ID id, PFObject* pObject)
{
	CMD_TRY
	{
		// Find connection
		auto pConnection = getTcpConnection(id);
		if (!pConnection)
		{
			disableTcpFilteringSafe(id);
			return DPCR_BYPASS;
		}

		auto eStatus = pConnection->dataPartAvailable(pObject);

		// Remove Connection if it does not have parsers
		if (!pConnection->hasParsers())
		{
			removeTcpConnection(id);
			disableTcpFilteringSafe(id);
		}

		return eStatus;
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Error in process dataPartAvailable() connection <" << id << ">"));
		disableTcpFilteringSafe(id);
		return DPCR_BYPASS;
	}
}

//
//
//
void NetFilterWrapper::tcpListened(const nfapi::cmdedr::ListenInfo* pListenInfo)
{
	CMD_TRY
	{
		// Create ListenInfo
		auto pInfo = std::make_shared<ListenInfo>();
		pInfo->eIpProtocol = IpProtocol::Tcp;
		pInfo->eFamily = AddressFamily(pListenInfo->ip_family);
		// Set System pid if it is 0
		pInfo->nPid = pListenInfo->nPid == 0 ? 4 : pListenInfo->nPid;
		pInfo->nTickTime = getTickCount();
		createAddressInfo(pInfo->local, pListenInfo->localAddr, pInfo->eFamily);
		
		// Skip if port == 0
		if (pInfo->local.nPort == 0)
			return;

		// Notify
		m_pNetMonController->notifyNewListen(pInfo);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Error in process tcpListened() process <" << pListenInfo->nPid << ">"));
	}
}

//////////////////////////////////////////////////////////////////////////
//
// UDP events
//

//
//
//
void NetFilterWrapper::udpCreated(ENDPOINT_ID /*id*/, PNF_UDP_CONN_INFO pConnInfo)
{
	CMD_TRY
	{
		// Create ListenInfo
		auto pInfo = std::make_shared<ListenInfo>();
		pInfo->eIpProtocol = IpProtocol::Udp;
		pInfo->eFamily = AddressFamily(pConnInfo->ip_family);
		// Set System pid if it is 0
		pInfo->nPid = pConnInfo->processId == 0 ? 4 : pConnInfo->processId;
		pInfo->nTickTime = getTickCount();
		createAddressInfo(pInfo->local, pConnInfo->localAddress, pInfo->eFamily);

		// Skip if port == 0
		if (pInfo->local.nPort == 0)
			return;

		// Notify
		m_pNetMonController->notifyNewListen(pInfo);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception & e)
	{
		e.log(SL, FMT("Error in process udpCreated() process <" << pConnInfo->processId << ">"));
	}
}

//
//
//
void NetFilterWrapper::udpClosed(nfapi::ENDPOINT_ID id, nfapi::PNF_UDP_CONN_INFO pConnInfo)
{
	CMD_TRY
	{
		// Find and remove Connection
		ObjPtr<UdpConnection> pConnection;
		{
			std::scoped_lock lock(m_mtxConnections);
			auto iter = m_mapUdpConnections.find(id);
			if (iter == m_mapUdpConnections.end())
				return;

			pConnection = iter->second;
			m_mapUdpConnections.erase(iter);
		}

		// Notify
		pConnection->notifyClose();
	}
	CMD_PREPARE_CATCH
	catch (error::Exception & e)
	{
		e.log(SL, FMT("Error in processing udpClosed() for process <" << pConnInfo->processId << ">"));
	}
}

//
//
//
NF_STATUS NetFilterWrapper::udpPostReceive(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options)
{
	if (!processUdpData(false, id, remoteAddress, buf, len, options))
	{
		// Don't parse other packets
		nf_udpDisableFiltering(id);
	}

	// Send the packet to application
	return nf_udpPostReceive(id, remoteAddress, buf, len, options);
}

//
//
//
NF_STATUS NetFilterWrapper::udpPostSend(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options)
{
	if (!processUdpData(true, id, remoteAddress, buf, len, options))
	{
		// Don't parse other packets
		nf_udpDisableFiltering(id);
	}

	// Send the packet to application
	return nf_udpPostSend(id, remoteAddress, buf, len, options);
}

//
//
//
bool NetFilterWrapper::processUdpData(bool fSend, nfapi::ENDPOINT_ID id, const unsigned char* remoteAddress,
	const char* buf, int len, nfapi::PNF_UDP_OPTIONS /*options*/)
{
	CMD_TRY
	{
		// Find exist connection
		ObjPtr<UdpConnection> pConnection;
		{
			std::scoped_lock lock(m_mtxConnections);
			auto iter = m_mapUdpConnections.find(id);
			if (iter != m_mapUdpConnections.end())
				pConnection = iter->second;
		}

		// Create new connection
		if (pConnection == nullptr)
		{
			// Create Connection
			if (remoteAddress == nullptr)
				return false;
			NF_UDP_CONN_INFO udpConnInfo = {};
			if (nf_getUDPConnInfo(id, &udpConnInfo) != NF_STATUS_SUCCESS)
				return false;
			// Detect incoming or outgoing type based on fSend
			Direction eDirection = (fSend ? Direction::Out : Direction::In);
			auto pInfo = createConnectionInfo(id, eDirection, &udpConnInfo, remoteAddress);
			pConnection = UdpConnection::create(pInfo);

			// Notify Controller
			m_pNetMonController->notifyNewConnection(pConnection);

			if (!pConnection->hasParsers())
				return false;

			// Add Connection to map
			{
				std::scoped_lock lock(m_mtxConnections);
				auto iter = m_mapUdpConnections.find(id);
				if (iter != m_mapUdpConnections.end())
					pConnection = iter->second;
				else
					m_mapUdpConnections[id] = pConnection;
			}
		}

		// Pass data to connection
		if (pConnection == nullptr)
			return false;
		pConnection->notifyData(fSend, (const uint8_t*) buf, (size_t)len);
		if (pConnection->hasParsers())
			return true;

		// Remove Connection without parsers
		{
			std::scoped_lock lock(m_mtxConnections);
			m_mapUdpConnections.erase(id);
			return false;
		}
	}
	CMD_PREPARE_CATCH
	catch (error::Exception & e)
	{
		e.log(SL, FMT("Error in processing udp packet"));
		return false;
	}
}



//////////////////////////////////////////////////////////////////////////
//
// Detection of failed connect()
//

bool NetFilterWrapper::detectFailedConnect()
{
	CMD_TRY
	{
		using namespace std::chrono;
		auto tNow = steady_clock::now();


		// Collect expired connections
		std::vector<std::shared_ptr<ConnectionInfo>> failedConnections;
		{
			std::scoped_lock lock(m_mtxConnectRequests);

			for (auto itInfo = m_mapConnectRequests.begin(); itInfo != m_mapConnectRequests.end();)
			{
				if (itInfo->second.nExpiredTime > tNow)
				{
					++itInfo;
					continue;
				}

				failedConnections.push_back(itInfo->second.pConnInfo);
				itInfo = m_mapConnectRequests.erase(itInfo);
			}
		}

		// Send events to receiver
		for (auto& pConnInfo : failedConnections)
			m_pNetMonController->notifyUnsuccessfulConnection(pConnInfo);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, "Error on detection of failed connect");
	}

	return true;
}

//
// Add connection into detection list
//
void NetFilterWrapper::addConnectionRequest(std::shared_ptr<ConnectionInfo> pConnInfo, Time nTimeoutMs)
{
	using namespace std::chrono;
	auto expireTime = steady_clock::now() + milliseconds(nTimeoutMs);

	std::scoped_lock lock(m_mtxConnectRequests);
	m_mapConnectRequests[pConnInfo->nInternalId] = ConnectRequestInfo{ pConnInfo, expireTime };
}

//
// Remove connection from detection list if it is exist
//
void NetFilterWrapper::removeConnectionRequest(ENDPOINT_ID nId)
{
	std::scoped_lock lock(m_mtxConnectRequests);
	m_mapConnectRequests.erase(nId);
}

//
// Send failed connection if it is exist with specified ENDPOINT_ID
//
void NetFilterWrapper::sendFailedConnection(ENDPOINT_ID nId)
{
	std::shared_ptr<ConnectionInfo> pConnInfo;
	{
		std::scoped_lock lock(m_mtxConnectRequests);
		auto iter = m_mapConnectRequests.find(nId);
		if (iter != m_mapConnectRequests.end())
		{
			pConnInfo = iter->second.pConnInfo;
			m_mapConnectRequests.erase(iter);
		}
	}

	if (pConnInfo)
		m_pNetMonController->notifyUnsuccessfulConnection(pConnInfo);
}

//////////////////////////////////////////////////////////////////////////
//
// Initialization / Finalization
//

//
//
//
NetFilterWrapper::NetFilterWrapper()
{
}

//
//
//
NetFilterWrapper::~NetFilterWrapper()
{
}

//
//
//
void NetFilterWrapper::finalConstruct(Variant vConfig)
{
	m_pNetMonController = queryInterface<INetworkMonitorController>(vConfig.get("controller"));

	// WSAStartup is required to call winsocks.dll function in the process
	{
		WSADATA wsaData;
		if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			error::win::WinApiError(SL, "Can't WSAStartup").throwException();
	}

	//
	// Initialize detection of failed connect()
	//
	{
		// avoid recursive pointing timer <-> NetworkMonitorController
		ObjWeakPtr<NetFilterWrapper> weakPtrToThis(getPtrFromThis(this));
		m_timerDetectFailedConnect = runWithDelay(c_nFailConnectCheckPeriod, [weakPtrToThis]() -> bool
		{
			auto pThis = weakPtrToThis.lock();
			if (pThis == nullptr)
				return false;
			return pThis->detectFailedConnect();
		});
	}

	nf_adjustProcessPriviledges();
}

//
//
//
void NetFilterWrapper::start()
{
	// Init ProtocolFilter
	std::filesystem::path pathTemp = std::wstring_view(getCatalogData("app.tempPath"));
	if (!pf_init(this, pathTemp.generic_wstring().c_str()))
		error::RuntimeError(SL, "Failed to init ProtocolFilter").throwException();

	// Initialize the library and start filtering thread
	static constexpr char c_sDrvSvcName[] = "edrdrv"; //< Name of Windows service
	nf_setOptions(1, NFF_DISABLE_AUTO_REGISTER | NFF_DISABLE_AUTO_START);
	nfapi::cmdedr::setCustomEventHandler(this);
	if (nf_init(c_sDrvSvcName, pf_getNFEventHandler()) != NF_STATUS_SUCCESS)
		error::RuntimeError(SL, "Failed to connect to driver").throwException();

	// Add NetFilter SDK rules
	std::vector<NF_RULE> rules;

	// ATTENTION: local IPv6 traffic is allowed by default
	// Do not filter local IPv4 traffic
	{
		NF_RULE rule = {};
		rule.filteringFlag = NF_ALLOW;
		rule.ip_family = AF_INET;

		bool fAddRule = true;
		if (inet_pton(AF_INET, "127.0.0.1", &rule.remoteIpAddress) <= 0)
		{
			error::win::WinApiError(SL, "Can't convert localhost IP").log();
			fAddRule = false;
		}
		if (inet_pton(AF_INET, "255.0.0.0", &rule.remoteIpAddressMask) <= 0)
		{
			error::win::WinApiError(SL, "Can't convert localhost mask").log();
			fAddRule = false;
		}
		if (fAddRule)
			rules.push_back(rule);
	}

	// Filter all TCP traffic
	if (m_pNetMonController->getProtocolOptions("tcp").get("enable", true))
	{
		NF_RULE rule = {};
		rule.protocol = IPPROTO_TCP; // Use WinAPI constants beause it is library interface
		// We can use NF_DISABLE_REDIRECT_PROTECTION. We don't redirect connection, so we can monitor proxy connections
		rule.filteringFlag = NF_FILTER | NF_INDICATE_CONNECT_REQUESTS | NF_DISABLE_REDIRECT_PROTECTION;
		rules.push_back(rule);
	}

	// Filter all UDP traffic
	if (m_pNetMonController->getProtocolOptions("udp").get("enable", true))
	{
		NF_RULE rule = {};
		rule.protocol = IPPROTO_UDP; // Use WinAPI constants beause it is library interface
		rule.filteringFlag = NF_FILTER | NF_DISABLE_REDIRECT_PROTECTION;
		rules.push_back(rule);
	}

	// Send rules to driver
	NF_STATUS eSetRulesResult = NF_STATUS_SUCCESS;
	if (rules.empty())
		eSetRulesResult = nf_deleteRules();
	else
		eSetRulesResult = nf_setRules(rules.data(), (int)rules.size());
	if (eSetRulesResult != NF_STATUS_SUCCESS)
		error::RuntimeError(SL, "Failed to add rule").throwException();
}

//
//
//
void NetFilterWrapper::stop()
{
	// Free the library
	nf_deleteRules();
	nf_free();
	pf_free();

	// Remove connections
	{
		std::scoped_lock lock(m_mtxConnections);
		m_mapTcpConnections.clear();
		m_mapUdpConnections.clear();
	}
}

//
//
//
void NetFilterWrapper::shutdown()
{
	m_pNetMonController.reset();
	::WSACleanup();
}

} // namespace win
} // namespace netmon
} // namespace cmd
