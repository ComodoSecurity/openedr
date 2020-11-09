//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file Wrapper of NetFilterSDK and ProtocolFilter libraries
///
/// @addtogroup netmon Network Monitor library
/// @{
#pragma once
#include "objects.h"
#include "connection_win.h"
#include "netmon_win.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "libnetmon"

namespace openEdr {
namespace netmon {
namespace win {


///
/// Wrapper of NetFilterSDK and ProtocolFilter libraries.
///
class NetFilterWrapper : public ObjectBase<CLSID_NetFilterWrapper>,
	public INetFilterWrapper,
	public ProtocolFilters::PFEventsDefault,
	public nfapi::cmdedr::CustomEventHandler
{
private:

	typedef ProtocolFilters::PFEventsDefault PFEventsDefault;

	ObjPtr<INetworkMonitorController> m_pNetMonController;  //< Callback to NetworkMonitorController

	//////////////////////////////////////////////////////////////////////////
	//
	// Connection map
	//
	std::mutex m_mtxConnections; //< mutex for m_mapTcpConnections and m_mapUdpConnections
	std::unordered_map<nfapi::ENDPOINT_ID, ObjPtr<TcpConnection>> m_mapTcpConnections; //< actively processed TCP connections
	ObjPtr<TcpConnection> getTcpConnection(nfapi::ENDPOINT_ID id)
	{
		std::scoped_lock lock(m_mtxConnections);
		auto iter = m_mapTcpConnections.find(id);
		if (iter == m_mapTcpConnections.end())
			return nullptr;
		return iter->second;
	}
	void removeTcpConnection(nfapi::ENDPOINT_ID id)
	{
		std::scoped_lock lock(m_mtxConnections);
		m_mapTcpConnections.erase(id);
	}

	std::unordered_map<nfapi::ENDPOINT_ID, ObjPtr<UdpConnection>> m_mapUdpConnections; //< actively processed UDP connections

	//////////////////////////////////////////////////////////////////////////
	//
	// Detection of failed connect()
	//

	struct ConnectRequestInfo
	{
		std::shared_ptr<ConnectionInfo> pConnInfo;
		std::chrono::time_point<std::chrono::steady_clock> nExpiredTime; // time of generating unsuccessful connection
	};
	std::unordered_map<nfapi::ENDPOINT_ID, ConnectRequestInfo> m_mapConnectRequests;
	std::mutex m_mtxConnectRequests;

	//
	// Detection timer
	//
	bool detectFailedConnect();
	ThreadPool::TimerContextPtr m_timerDetectFailedConnect;
	static constexpr Time c_nTcpConnectTimeout = 30000 /*30 sec*/;
	static constexpr Time c_nFailConnectCheckPeriod = 10000 /*10 sec*/;

	//
	// Add connection into detection list
	//
	void addConnectionRequest(std::shared_ptr<ConnectionInfo> pConnInfo, Time nTimeoutMs);

	//
	// Remove connection from detection list if it is exist
	//
	void removeConnectionRequest(nfapi::ENDPOINT_ID nId);

	//
	// Send failed connection if it is exist with specified ENDPOINT_ID
	//
	void sendFailedConnection(nfapi::ENDPOINT_ID nId);

	void dataAvailableInt(nfapi::ENDPOINT_ID id, ProtocolFilters::PFObject* pObject);

	//
	// Udp data processor
	//
	// @return false to disable connection processing next time
	//
	bool processUdpData(bool fSend, nfapi::ENDPOINT_ID id, const unsigned char* remoteAddress,
		const char* buf, int len, nfapi::PNF_UDP_OPTIONS options);

protected:
	/// Constructor
	NetFilterWrapper();
	
	/// Destructor
	virtual ~NetFilterWrapper() override;

public:

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - object's configuration including the following fields:
	/// * controller - callback to controller.
	///
	void finalConstruct(Variant vConfig);

	// INetFilterWrapper interface
	void start() override;
	void stop() override;
	void shutdown() override;

	// ProtocolFilters::PFEventsDefault interface
	void threadStart() override;
	void threadEnd() override;

	void tcpConnectRequest(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo) override;
	void tcpConnected(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo) override;
	void tcpClosed(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo) override;
	void dataAvailable(nfapi::ENDPOINT_ID id, ProtocolFilters::PFObject* pObject) override;
	ProtocolFilters::PF_DATA_PART_CHECK_RESULT dataPartAvailable(nfapi::ENDPOINT_ID id, ProtocolFilters::PFObject* pObject) override;

	void udpCreated(nfapi::ENDPOINT_ID id, nfapi::PNF_UDP_CONN_INFO pConnInfo) override;
	void udpClosed(nfapi::ENDPOINT_ID id, nfapi::PNF_UDP_CONN_INFO pConnInfo) override;
	NF_STATUS udpPostReceive(nfapi::ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, nfapi::PNF_UDP_OPTIONS options) override;
	NF_STATUS udpPostSend(nfapi::ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, nfapi::PNF_UDP_OPTIONS options) override;

	// nfapi::cmdedr::CustomEventHandler
	void tcpListened(const nfapi::cmdedr::ListenInfo* pListenInfo) override;
};

} // namespace win
} // namespace netmon
} // namespace openEdr

/// @}
