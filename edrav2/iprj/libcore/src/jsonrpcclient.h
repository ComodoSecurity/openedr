//
// EDRAv2.libcore project.
//
// Author: Yury Ermakov (17.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file JSON-RPC client declaration
///
/// @addtogroup ipc Inter-process communication
/// Objects described here are intended to establish a communication channel between
/// several running processes.
/// @{
#pragma once
#include <command.hpp>
#include <common.hpp>
#include <queue.hpp>
#include <objects.h>
#include "jsonrpc.h"

namespace openEdr {
namespace ipc {

///
/// JSON-RPC client object class.
///
class JsonRpcClient : public ObjectBase<CLSID_JsonRpcClient>, 
	public IDataReceiver, 
	public ICommandProcessor,
	public IQueueNotificationAcceptor
{
private:
	std::unique_ptr<jsonrpc::TcpSocketClient> m_pSocket;
	std::unique_ptr<jsonrpc::HttpClient> m_pHttpClient;
	std::unique_ptr<jsonrpc::Client> m_pClient;
	std::string m_sConnectionInfo;
	std::mutex m_mtxClient;
	Size m_nTimeout = c_nMaxSize;
	ChannelMode m_eChannelMode = ChannelMode::Plain;
	Variant m_vSerializeConfig;

	ObjWeakPtr<IDataProvider> m_pProvider;
	ThreadPool m_threadPool{"JsonClient::InteractionPool"};

	//void encryptData(const Variant& vData, Json::Value& result);
	//Variant decryptData(const Json::Value& data);

protected:
	///
	/// Constructor.
	///
	JsonRpcClient() {}
	
	///
	/// Destructor.
	///
	virtual ~JsonRpcClient() {}

	//
	//
	//
	void processQueueEvent();

public:
	
	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **host** [str] - a hostname or the IPv4 address on which the client should try to connect
	///   **port** [int] - a port on which the client should try to connect
	///   **protocol** [str, opt] - the connection protocol: HTTP (by default), TCP
	///   **timeout** [int, opt] - an operation timeout (if supported by protocol).
	///      By default, 10 sec. For infinite timeout, use value -1.
	///   **channelMode** [str, opt] - a mode of the communication channel. Possible values:
	///    "plain" (by default), "encrypted".
	///   **encryption** [str] - an encryption argorithm to be used. This field is mandatory
	///      if channelMode is set to "encrypted".
	///   **verbose** [bool, opt] - display more verbose information on inner operations (e.g. cURL)
	///      (default: false).
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IDataReceiver

	/// @copydoc IDataReceiver::put()
	void put(const Variant& vData) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IQueueNotificationAcceptor

	/// @copydoc IQueueNotificationAcceptor::notifyAddQueueData()
	void notifyAddQueueData(Variant vTag) override;
	/// @copydoc IQueueNotificationAcceptor::notifyQueueOverflowWarning()
	void notifyQueueOverflowWarning(Variant vTag) override;
};

} // namespace ipc
} // namespace openEdr

/// @}
