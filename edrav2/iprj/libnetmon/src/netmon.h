//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file NetFilter-independed interfaces 
///
/// @addtogroup netmon Network Monitor library
/// @{
#pragma once

namespace openEdr {
namespace netmon {

//////////////////////////////////////////////////////////////////////////
//
// Connection info
//

///
/// Connection direction.
///
enum class Direction : uint8_t
{
	In,   ///< Incoming TCP connection or UDP packet
	Out,  ///< Outgoing TCP connection or UDP packet
	Both, ///< Any direction
};

///
/// Address family.
///
enum class AddressFamily : uint8_t
{
	Inet = 2,   ///< IPv4, AF_INET
	Inet6 = 23, ///< IPv6, AF_INET6
};


///
/// Address information.
///
struct AddressInfo
{
	std::string sIp; ///< IPv4 or IPv6 address
	uint16_t nPort;  ///< Port number
};

///
/// Connection information.
///
struct ConnectionInfo
{
	IpProtocol eIpProtocol; ///< Ip protocol type (TCP, UDP ...)
	Direction eDirection;   ///< incoming/outgoing
	AddressFamily eFamily;  ///< Support Inet and Inet6 only
	uint32_t nPid;          ///< pid of process which made the connection

	AddressInfo local;      ///< Local address
	AddressInfo remote;     ///< Remote address

	Time nTickTime;         ///< event time
	uint64_t nInternalId;   ///< Internal connection id (inside NetFilter SDK)

	std::string sDisplayName; ///< Display string
	Variant vConnection;	///< Variant representation
};

//
// Converts address descriptor to Variant.
//
inline Variant convertAddressInfo(const AddressInfo& info, uint64_t* pnId = nullptr)
{
	crypt::xxh::Hasher hasher;
	crypt::updateHash(hasher, info.sIp);
	crypt::updateHash(hasher, info.nPort);
	uint64_t nId = hasher.finalize();
	if (pnId != nullptr)
		*pnId = nId;

	return Dictionary{
		{"id", nId},
		{"ip", info.sIp},
		{"port", info.nPort},
	};
}

//
// Convert ConnectionInfo to Variant
//
inline Variant convertConnectionInfo(const ConnectionInfo& conInfo, netmon::ConnectionStatus eStatus)
{
	// for hash calculation
	uint64_t nConnectionIdArray[3];
	nConnectionIdArray[0] = conInfo.nPid;

	Variant vLocal = convertAddressInfo(conInfo.local, &nConnectionIdArray[1]);
	Variant vRemote = convertAddressInfo(conInfo.remote, &nConnectionIdArray[2]);

	// calc id
	uint64_t nId = crypt::xxh::getHash(&nConnectionIdArray, sizeof(nConnectionIdArray));

	return Dictionary{
		{"id", nId},
		{"local", vLocal},
		{"remote", vRemote},
		{"protocol", conInfo.eIpProtocol},
		{"status", eStatus},
	};
}

//
//
//
inline std::string getDisplayString(const ConnectionInfo& info)
{
	return FMT(
		(info.eIpProtocol == IpProtocol::Tcp ? "TCP " : "UDP ") <<
		(info.eDirection == Direction::In ? "IN  " : "OUT ") <<
		"pid <" << info.nPid << "> remote <" <<
		info.remote.sIp << ":" << info.remote.nPort << "> local <" <<
		info.local.sIp << ":" << info.local.nPort << ">"
	);
}

///
/// Connection information.
///
struct ListenInfo
{
	IpProtocol eIpProtocol; ///< Ip protocol type (TCP, UDP ...)
	AddressFamily eFamily;  ///< Support Inet and Inet6 only
	uint32_t nPid;          ///< pid of process which made the connection

	AddressInfo local;      ///< Local address

	Time nTickTime;         ///< event time
};


///
/// Interface of connection.
///
class IConnection : OBJECT_INTERFACE
{
public:

	///
	/// Returns connection info.
	///
	/// @returns The function returns a shared pointer to connection info.
	///
	virtual std::shared_ptr<const ConnectionInfo> getInfo() = 0;

	///
	/// Adds ParserFactory.
	///
	/// @param pParserFactory - a smart pointer to a parser factory.
	///
	virtual void addParserFactory(ObjPtr<IObject> pParserFactory) = 0;

	///
	/// Returns that connection has parsers (connection is currently being processed).
	///
	/// @returns The function returns `true` if the connection has parsers.
	///
	virtual bool hasParsers() = 0;
};

//////////////////////////////////////////////////////////////////////////
//
// Contr
//

///
/// Interface of wrapper of NetFilterSDK and ProtocolFilter libraries.
///
class INetFilterWrapper : OBJECT_INTERFACE
{
public:
	// IService methods analogs
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void shutdown() = 0;
};


///
/// Internal interface of NetworkMonitorController.
///
class INetworkMonitorController : OBJECT_INTERFACE
{
public:
	///
	/// Sends the event.
	///
	/// @param vEvent - the event descriptor.
	///
	virtual void sendEvent(Variant vEvent) = 0;

	///
	/// Test event enabled.
	///
	/// @param eEvent - raw event identifier.
	/// @returns The function returns `true` if the specified event is enabled.
	///
	virtual bool isEventEnabled(RawEvent eEvent) = 0;

	///
	/// Callback on new connection.
	///
	/// @param pConnection - a smart pointer to the connection.
	///
	virtual void notifyNewConnection(ObjPtr<IConnection> pConnection) = 0;

	///
	/// Callback on unsuccessful connection.
	///
	/// @param pConnectionInfo - a shared pointer to the connection info.
	///
	virtual void notifyUnsuccessfulConnection(std::shared_ptr<const ConnectionInfo> pConnectionInfo) = 0;

	///
	/// Callback on new listen.
	///
	/// @param pListenInfo - a smart pointer to the listen info.
	///
	virtual void notifyNewListen(ObjPtr<const ListenInfo> pListenInfo) = 0;

	///
	/// Returns protocol options by name.
	///
	/// @param sProtocolName - a name of the protocol.
	/// @returns The function returns a data packet with options of a specified protocol.
	///
	virtual Variant getProtocolOptions(std::string_view sProtocolName) = 0;
};


//////////////////////////////////////////////////////////////////////////
//
// Parsers
//


///
/// Result of dataAvailable().
///
enum class DataStatus
{
	Processed,    ///< Data was processed by driver. Remove other parsers
	Ignored,      ///< Data was NOT processed by driver. But this data can happend in the parser
	Incompatible, ///< Data is not compatible with the parser. Remove the parser.
};

///
/// Result of dataPartAvailable().
///
/// If any Parser supports the object, other parsers are removed.
/// If no one Parser supports the object, DPCR_BYPASS is returned.
///
enum class DataPartStatus
{
	/// Parser supports this data/object.
	/// Continue receiving the data and indicate the same object with more content via dataPartAvailable. 
	/// converted to DPCR_MORE_DATA_REQUIRED
	NeedMoreData,

	/// Parser supports this data/object.
	/// Stop calling dataPartAvailable, wait until receiving the full content and indicate it via dataAvailable.
	/// The content goes to destination immediately.
	/// converted to DPCR_FILTER_READ_ONLY
	NeedFullObject,

	/// Parser supports this data/object.
	/// Do not call dataPartAvailable or dataAvailable for the current object,
	/// just passthrough it to destination.
	/// converted to DPCR_BYPASS
	BypassObject,

	/// Parser does NOT support this data/object.
	/// But this data can happend in the parser
	Ignored,

	/// Parser does NOT support this data/object.
	/// Remove the parser.
	Incompatible,
};

//////////////////////////////////////////////////////////////////////////
//
// UDP parsers
//

///
/// Interface of UDP Parser.
///
class IUdpParser : OBJECT_INTERFACE
{
public:

	///
	/// Callback on data send/receive.
	///
	/// @param fSend - if true it is send, else it is receive.
	/// @param pConInfo - ConnectionInfo.
	/// @param pBuf - pointer to data buffer.
	/// @param nLen - size of data buffer.
	///
	/// @return DataStatus
	///
	virtual DataStatus notifyData(bool fSend, std::shared_ptr<const ConnectionInfo> pConInfo, const uint8_t* pBuf, size_t nLen) = 0;

	///
	/// Callback on close.
	///
	/// @param pConInfo - a shared pointer to a connection info.
	///
	virtual void notifyClose(std::shared_ptr<const ConnectionInfo> pConInfo) = 0;
};


///
/// Interface of TCP Parser Factory.
///
class IUdpParserFactory : OBJECT_INTERFACE
{
public:
	///
	/// Returns name of Parser.
	///
	/// Returned string has same lifetime as object.
	///
	/// @return The function returns a name of the parser.
	///
	virtual std::string_view getName() = 0;

	///
	/// Creates a parser for process Connection and object.
	/// 
	/// @param pConInfo - a shared pointer to the connection info.
	/// @return The function returns a smart pointer to created object or `nullptr` 
	/// if connection is not supported.
	///
	virtual ObjPtr<IUdpParser> createParser(std::shared_ptr<const ConnectionInfo> pConInfo) = 0;
};

} // namespace netmon
} // namespace openEdr

/// @}
