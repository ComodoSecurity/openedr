//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (14.08.2019)
// Reviewer: Denis Bogdanov (02.09.2019)
//
///
/// @file Network Monitor Controller implementation
///
/// @addtogroup netmon Network Monitor library
/// @{
#include "pch.h"
#include "controller.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "netmon"

namespace openEdr {
namespace netmon {

//
//
//
bool NetworkMonitorController::isEventEnabled(netmon::RawEvent eRawEvent)
{
	const uint64_t c_nFlag = 1ui64 << (uint64_t)eRawEvent;
	return (m_nSentEventsFlags & c_nFlag) != 0;
}

// 
// 
// 
void NetworkMonitorController::sendEvent(Variant vEvent)
{
	if (!m_pReceiver)
		error::InvalidArgument(SL, "Receiver interface is undefined").throwException();

	m_pReceiver->put(vEvent);
}

//
//
//
template<typename FnAddData>
void NetworkMonitorController::sendEvent(RawEvent eRawEvent, Size nPid, FnAddData fnAddData)
{
	if (!isEventEnabled(eRawEvent))
		return;

	// Calc LLE type
	Event eLleType;
	switch (eRawEvent)
	{
	case netmon::RawEvent::NETMON_CONNECT_IN:
		eLleType = Event::LLE_NETWORK_CONNECT_IN;
		break;
	case netmon::RawEvent::NETMON_CONNECT_OUT:
		eLleType = Event::LLE_NETWORK_CONNECT_OUT;
		break;
	case netmon::RawEvent::NETMON_LISTEN:
		eLleType = Event::LLE_NETWORK_LISTEN;
		break;
	default:
		error::InvalidArgument("Invalid raw event type").throwException();
	}

	Dictionary vEvent {
		{"baseType", eLleType},
		{"rawEventId", createRaw(CLSID_NetworkMonitorController, (uint32_t)eRawEvent)},
		{"tickTime", ::GetTickCount64()},
		{"process", Dictionary {{"pid", nPid}} },
	};

	bool fSendEvent = fnAddData(vEvent);
	if (fSendEvent)
		sendEvent(vEvent);
}

//
//
//
void NetworkMonitorController::sendConnectionEvent(std::shared_ptr<const ConnectionInfo> pConInfo, ConnectionStatus eStatus)
{
	// Calc RawEvent
	RawEvent eRawEvent;
	if (pConInfo->eDirection == Direction::In)
		eRawEvent = RawEvent::NETMON_CONNECT_IN;
	else if (pConInfo->eDirection == Direction::Out)
		eRawEvent = RawEvent::NETMON_CONNECT_OUT;
	else
		return;

	sendEvent(eRawEvent, pConInfo->nPid, [&pConInfo, eStatus](Dictionary& vEvent) -> bool
	{
		Variant vConnection = pConInfo->vConnection;
		if (eStatus != ConnectionStatus::Success)
		{
			vConnection = vConnection.clone();
			vConnection.put("status", eStatus);
		}

		vEvent.put("connection", vConnection);
		vEvent.put("tickTime", pConInfo->nTickTime); // time is changed for connection event
		return true;
	});
}

//
//
//
void NetworkMonitorController::notifyNewConnection(ObjPtr<IConnection> pConnection)
{
	auto pConInfo = pConnection->getInfo();
	sendConnectionEvent(pConInfo, ConnectionStatus::Success);

	// TODO: Create universal tunable creation of ParserFactory
	if (pConInfo->eIpProtocol == IpProtocol::Tcp && pConInfo->eDirection == Direction::Out)
	{
		pConnection->addParserFactory(m_pHttpParserFactory);
		pConnection->addParserFactory(m_pFtpParserFactory);
	}
	else if (pConInfo->eIpProtocol == IpProtocol::Udp && pConInfo->remote.nPort == 53)
	{
		pConnection->addParserFactory(m_pDnsUdpParserFactory);
	}
}

//
//
//
void NetworkMonitorController::notifyUnsuccessfulConnection(std::shared_ptr<const ConnectionInfo> pConInfo)
{
	sendConnectionEvent(pConInfo, ConnectionStatus::GenericFail);
}

//
//
//
void NetworkMonitorController::notifyNewListen(ObjPtr<const ListenInfo> pListenInfo)
{
	sendEvent(RawEvent::NETMON_LISTEN, pListenInfo->nPid, [&pListenInfo](Dictionary& vEvent) -> bool
	{
		vEvent.put("address", convertAddressInfo(pListenInfo->local));
		vEvent.put("protocol", pListenInfo->eIpProtocol);
		return true;
	});
}

//
//
//
Variant NetworkMonitorController::getProtocolOptionsInt(std::string_view sProtocolName)
{
	Variant vProtocolOptions = m_vProtocolOptions;
	if (vProtocolOptions.isNull())
		return Variant();

	auto vOptions = vProtocolOptions.getSafe(sProtocolName);
	if (!vOptions.has_value())
		return Variant();

	if (vOptions->isNull())
		return Variant();
	if (vOptions->isDictionaryLike())
		return vOptions.value();
	if (vOptions->getType() == Variant::ValueType::Boolean)
		return Dictionary({ {"enable", vOptions.value()} });

	error::LogicError(SL, FMT("Invalid options for protocol <" << sProtocolName << ">. Value: "
		<< vOptions.value())).throwException();
}

//
//
//
Variant NetworkMonitorController::getProtocolOptions(std::string_view sProtocolName)
{
	Variant vAllOptions = getProtocolOptionsInt("all");
	Variant vProtocolOptions = getProtocolOptionsInt(sProtocolName);

	if (vAllOptions.isNull())
	{
		if (vProtocolOptions.isNull())
			return Dictionary();
		return vProtocolOptions;
	}
	else
	{
		if (vProtocolOptions.isNull())
			return vAllOptions;
		return vAllOptions.clone().merge(vProtocolOptions, Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict);
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Initialization / Finalization
//

//
//
//
NetworkMonitorController::NetworkMonitorController()
{
}

//
//
//
NetworkMonitorController::~NetworkMonitorController()
{
}

//
//
//
void NetworkMonitorController::finalConstruct(Variant vConfig)
{
	m_pReceiver = queryInterfaceSafe<IDataReceiver>(vConfig.get("receiver", {}));

	m_nSentEventsFlags = (uint64_t)vConfig.get("eventFlags", (uint64_t) m_nSentEventsFlags);
	m_vProtocolOptions = vConfig.get("protocolsOptions", Variant());

	Dictionary vNetFilterWrapperCfg {
		{"controller", getPtrFromThis()},
	};

	m_pNetFilterWrapper = queryInterface<INetFilterWrapper>(createObject(CLSID_NetFilterWrapper, vNetFilterWrapperCfg));

	// Create parserFactories
	m_pHttpParserFactory = createObject(CLSID_HttpParserFactory, Dictionary({ {"controller", getPtrFromThis()} }));
	m_pFtpParserFactory = createObject(CLSID_FtpParserFactory, Dictionary({ {"controller", getPtrFromThis()} }));
	m_pDnsUdpParserFactory = createObject(CLSID_DnsUdpParserFactory, Dictionary({ {"controller", getPtrFromThis()} }));
}

//
//
//
void NetworkMonitorController::loadState(Variant vState)
{
}

//
//
//
Variant NetworkMonitorController::saveState()
{
	return {};
}

//
//
//
void NetworkMonitorController::start()
{
	m_pNetFilterWrapper->start();
	m_fStarted = true;
}

//
//
//
void NetworkMonitorController::stop()
{
	if (!m_fStarted)
		return;
	m_pNetFilterWrapper->stop();
}

//
//
//
void NetworkMonitorController::shutdown()
{
	m_pNetFilterWrapper->shutdown();
	m_pNetFilterWrapper.reset();
	m_pHttpParserFactory.reset();
	m_pFtpParserFactory.reset();
	m_pDnsUdpParserFactory.reset();
}

//
//
//
Variant NetworkMonitorController::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN
		LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant SystemMonitorController::execute()
	///
	/// ##### start()
	/// Starts the controller.
	///
	if (vCommand == "start")
	{
		start();
		return {};
	}

	///
	/// @fn Variant SystemMonitorController::execute()
	///
	/// ##### stop()
	/// Stops the controller.
	///
	else if (vCommand == "stop")
	{
		stop();
		return {};
	}

	///
	/// @fn Variant SystemMonitorController::execute()
	///
	/// ##### shutdown()
	/// Stops the driver and free the controller's resources.
	///
	else if (vCommand == "shutdown")
	{
		shutdown();
		return {};
	}

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("SystemMonitorController doesn't support a command <"
		<< vCommand << ">")).throwException();
}

} // namespace netmon
} // namespace openEdr

/// @}
