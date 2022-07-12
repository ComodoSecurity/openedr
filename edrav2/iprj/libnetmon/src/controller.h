//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (14.08.2019)
// Reviewer: Denis Bogdanov (02.09.2019)
//
///
/// @file Network Monitor Controller declaration
///
/// @addtogroup netmon Network Monitor library
/// @{
#pragma once
#include "objects.h"
#include "netmon.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "libnetmon"

namespace cmd {
namespace netmon {

///
/// Service that receive raw events from network driver and handle them.
///
class NetworkMonitorController : public ObjectBase<CLSID_NetworkMonitorController>,
	public INetworkMonitorController,
	public ICommandProcessor, 
	public IService
{
private:

	//////////////////////////////////////////////////////////////////////////
	//
	// NetFilter SDK wrapper
	//
	ObjPtr<INetFilterWrapper> m_pNetFilterWrapper;
	ObjPtr<IObject> m_pHttpParserFactory;
	ObjPtr<IObject> m_pFtpParserFactory;
	ObjPtr<IObject> m_pDnsUdpParserFactory;


	//////////////////////////////////////////////////////////////////////////
	//
	// Event sending
	//

	ObjPtr<IDataReceiver> m_pReceiver;
	std::atomic_uint64_t m_nSentEventsFlags = c_nDefaultEventsFlags;
	std::atomic_bool m_fStarted = false;

	//
	// Sends the event.
	//
	// @param eRawEvent - raw event id.
	// @param nPid - process id.
	// @param fnAddData - function to add all additional event data
	//	Signature: bool fnAddData(Dictionary& vEvent);
	//  if fnAddData() returns false, this function returns null.
	//	fnAddData() should add "connection" field.
	// 
	template<typename FnAddData>
	void sendEvent(RawEvent eRawEvent, Size nPid, FnAddData fnAddData);

	void sendConnectionEvent(std::shared_ptr<const ConnectionInfo> pConInfo, ConnectionStatus eStatus);

	//////////////////////////////////////////////////////////////////////////
	//
	// Event callbacks
	//

	void notifyNewConnection(ObjPtr<IConnection> pConnection) override;
	void notifyUnsuccessfulConnection(std::shared_ptr<const ConnectionInfo> pConInfo) override;
	void notifyNewListen(ObjPtr<const ListenInfo> pListenInfo) override;

	bool isEventEnabled(netmon::RawEvent eEvent) override;
	void sendEvent(Variant vEvent) override;

	//////////////////////////////////////////////////////////////////////////
	//
	// Protocols configurations
	//

	Variant m_vProtocolOptions;

	//
	// Returns Protocol options
	//
	Variant getProtocolOptions(std::string_view sProtocolName) override;
	Variant getProtocolOptionsInt(std::string_view sProtocolName);

protected:
	/// Constructor.
	NetworkMonitorController();
	
	/// Destructor.
	virtual ~NetworkMonitorController() override;

public:

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - object's configuration including the following fields:
	/// * protocolOptions - dictionary with key - protocol name, values - protocol 
	///		options. See "Protocol options".
	/// * receiver - object that receive parsed data.
	/// * eventFlags - filter for sent events (flags, default - all).
	///
	/// Protocol options:
	///   Supported protocol names: 
	///     - "all" - default options for all protocol
	///     - "tcp"
	///     - "udp"
	///   Supported protocol options: 
	///     "Protocol options" can be 
	///       * dictionary with protocol options. Each protocol can have individual options.
	///       * boolean. It is shortcut for {"enable" : Specified_values}.
	///       * null. Default rules is used
	///     Common protocol options:
	///       * "enable" - enable/disable protocol catching.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor interface

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IService interface
	
	/// @copydoc IService::loadState()
	void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	Variant saveState() override;
	/// @copydoc IService::start()
	void start() override;
	/// @copydoc IService::start()
	void stop() override;
	/// @copydoc IService::shutdown()
	void shutdown() override;
};

} // namespace netmon
} // namespace cmd

/// @}
