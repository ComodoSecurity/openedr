//
// EDRAv2.libcore project.
//
// Author: Yury Ermakov (30.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file JSON-RPC server implementation
///
#include "pch.h"
#include "jsonrpc.h"
#include "jsonrpcserver.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "jsonrpcsvr"

namespace openEdr {
namespace ipc {

namespace detail {

//
// Internal server class
//
class Server : public jsonrpc::AbstractServer<Server>
{
private:
	Variant m_vProcessor;
	ChannelMode m_eChannelMode;
	Variant m_vSerializeConfig;

public:
	//
	// Constructor
	//
	Server(jsonrpc::HttpServer &server, Variant vProcessor, ChannelMode eChannelMode,
		Variant vSerializeConfig) :
		jsonrpc::AbstractServer<Server>(server, jsonrpc::JSONRPC_SERVER_V2_PROXY),
		m_vProcessor(vProcessor), m_eChannelMode(eChannelMode), m_vSerializeConfig(vSerializeConfig) {}

	//
	// Method calls handler
	//
	void HandleMethodCall(const std::string& name, const Json::Value& input, Json::Value& output) override
	{
		ChannelMode eCallChannelMode = ChannelMode::Plain;
		CMD_TRY
		{
			Variant vParams;
			Variant vCommand;
	
			if (name == c_sEncryptedFieldName)
			{
				if ((m_eChannelMode != ChannelMode::Encrypted) && (m_eChannelMode != ChannelMode::Both))
					error::OperationNotSupported(SL, "Encrypted commands are not supported").throwException();
				eCallChannelMode = ChannelMode::Encrypted;
				auto vData = decryptJsonValue(input);
				vCommand = vData["command"];
				vParams = vData["params"];
			}
			else
			{
				vCommand = name;
				vParams = convertJsonValueToVariant(input);
			}

			if ((m_eChannelMode != ChannelMode::Both) && (m_eChannelMode != eCallChannelMode))
				error::OperationNotSupported(SL, FMT("Wrong RPC channel mode <" <<
					getChannelModeString(eCallChannelMode) << ">")).throwException();

			if (vParams.isEmpty())
				LOGLVL(Debug, "Process request <" << vCommand << ">");
			else
				LOGLVL(Debug, "Process request <" << vCommand << ">, params:\n" << vParams);
			Variant vResult = createCommand(m_vProcessor, vCommand, vParams)->execute();
			if (vResult.isEmpty())
				LOGLVL(Debug, "Send empty response");
			else
				LOGLVL(Debug, "Send response:\n" << vResult);

			if (eCallChannelMode == ChannelMode::Encrypted)
				encryptJsonValue(vResult, output, m_vSerializeConfig);
			else
				output = convertVariantToJsonValue(vResult);
		}
		CMD_PREPARE_CATCH
		catch (error::TypeError& ex)
		{
			ex.log(SL, "Invalid parameters of JSON request");
			Json::Value data;
			if (eCallChannelMode == ChannelMode::Encrypted)
			{
				Variant vData = Dictionary({ {"exception", ex.serialize()} });
				encryptJsonValue(vData, data, m_vSerializeConfig);
			}
			else
				data["exception"] = convertVariantToJsonValue(ex.serialize());
			throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_RPC_INVALID_PARAMS, ex.what(), data);
		}
		catch (error::Exception& ex)
		{
			ex.log(SL, "JSON RPC server processes an exception as a response");
			Json::Value data;
			if (eCallChannelMode == ChannelMode::Encrypted)
			{
				Variant vData = Dictionary({ {"exception", ex.serialize()} });
				encryptJsonValue(vData, data, m_vSerializeConfig);
			}
			else
				data["exception"] = convertVariantToJsonValue(ex.serialize());
			throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_RPC_INTERNAL_ERROR, ex.what(), data);
		}
	}
};

} // namespace detail

//
//
//
JsonRpcServer::JsonRpcServer()
{
}

//
//
//
JsonRpcServer::~JsonRpcServer()
{
	CMD_TRY
	{
		stopServer();
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.log(SL, "Error during the object destruction");
	}
}

//
//
//
void JsonRpcServer::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;

	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "Dictionary expected").throwException();
	if (!vConfig.has("port"))
		error::InvalidArgument(SL, "Missing field <port>").throwException();
	if (!vConfig.has("processor"))
		error::InvalidArgument(SL, "Missing field <processor>").throwException();

	ChannelMode eChannelMode = ChannelMode::Both;
	Variant vSerializeConfig;
	if (vConfig.has("channelMode"))
	{
		eChannelMode = getChannelModeFromString(vConfig["channelMode"]);
		if (eChannelMode == ChannelMode::Invalid)
			error::InvalidArgument(SL, FMT("Invalid channel mode <" <<
				vConfig["channelMode"] << ">")).throwException();

		if ((eChannelMode == ChannelMode::Encrypted) || (eChannelMode == ChannelMode::Both))
		{
			std::string sEncryption = vConfig.get("encryption", "");
			if (sEncryption.empty())
				error::InvalidArgument(SL, "Field <encryption> is missing or empty").throwException();
			vSerializeConfig = Dictionary({ {"encryption", sEncryption} });
		}
	}

	m_pHttpServer = std::make_unique<jsonrpc::HttpServer>(vConfig.get("port"), "", "",
		vConfig.get("numThreads", 0));
	m_pServer = std::make_unique<detail::Server>(*m_pHttpServer, vConfig.get("processor"),
		eChannelMode, vSerializeConfig);

	LOGLVL(Debug, "JSON-RPC server is created (channelMode: <" <<
		getChannelModeString(eChannelMode) << ">)");
	TRACE_END("Error during configuration");
}

//
//
//
Variant JsonRpcServer::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	if (vParams.isEmpty())
		LOGLVL(Debug, "Execute command <" << vCommand << ">");
	else
		LOGLVL(Debug, "Execute command <" << vCommand << ">, params:\n" << vParams);

	if (vCommand == "start")
		startServer();
	else if (vCommand == "stop")
		stopServer();
	else
		error::OperationNotSupported(SL, FMT("Unknown command <" << vCommand << ">")).throwException();
	
	return {};

	TRACE_END("Error during command execution");
}

//
//
//
void JsonRpcServer::startServer()
{
	std::scoped_lock _lock(m_mtxServer);
	if (!m_pServer->StartListening())
		error::RuntimeError(SL, "Can't start JSON-RPC server").throwException();
	LOGLVL(Detailed, "JSON-RPC server is started");
}

//
//
//
void JsonRpcServer::stopServer()
{
	std::scoped_lock _lock(m_mtxServer);
	if (m_pServer && !m_pServer->StopListening())
		error::RuntimeError(SL, "Can't stop JSON-RPC server").throwException();
	LOGLVL(Detailed, "JSON-RPC server is stopped");
}

} // namespace ipc
} // namespace openEdr 
