//
// derav2.libcore project.
//
// Author: Yury Ermakov (17.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file JSON-RPC client implementation
///
#include "pch.h"
#include "jsonrpcclient.h"
#include <service.hpp>

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "jsonrpcclt"

namespace cmd {
namespace ipc {

//
//
//
void JsonRpcClient::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;

	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "Dictionary expected").throwException();
	if (!vConfig.has("host"))
		error::InvalidArgument(SL, "Missing field <host>").throwException();
	if (!vConfig.has("port"))
		error::InvalidArgument(SL, "Missing field <port>").throwException();

	Size nTimeout = vConfig.get("timeout", 10); // default timeout = 10 sec
	m_nTimeout = (nTimeout == -1) ? c_nMaxSize : nTimeout * 1000;

	Variant vProtocol = vConfig.get("protocol", "HTTP");
	if (vProtocol == "HTTP")
	{
		std::string sUrl = FMT(std::string(vConfig["host"]) << ":" << vConfig["port"]);
		m_pHttpClient = std::make_unique<jsonrpc::HttpClient>(sUrl, vConfig.get("verbose", false));
		if (m_nTimeout != -1)
			m_pHttpClient->SetTimeout((long)m_nTimeout);
		m_pClient = std::make_unique<jsonrpc::Client>(*m_pHttpClient);
		m_sConnectionInfo = "http://" + sUrl;
	}
	else if (vProtocol == "TCP")
	{
		m_pSocket = std::make_unique<jsonrpc::TcpSocketClient>(vConfig["host"], vConfig["port"]);
		m_pClient = std::make_unique<jsonrpc::Client>(*m_pSocket);
		m_sConnectionInfo = FMT("tcp://" << vConfig["host"] << ":" << vConfig["port"]);
	}
	else
		error::InvalidArgument(SL, FMT("Unknown protocol <" << vProtocol << ">")).throwException();

	if (vConfig.has("channelMode"))
	{
		m_eChannelMode = getChannelModeFromString(vConfig["channelMode"]);
		if ((m_eChannelMode != ChannelMode::Plain) && (m_eChannelMode != ChannelMode::Encrypted))
			error::InvalidArgument(SL, FMT("Invalid channel mode <" <<
				vConfig["channelMode"] << ">")).throwException();

		if (m_eChannelMode == ChannelMode::Encrypted)
		{
			std::string sEncryption = vConfig.get("encryption", "");
			if (sEncryption.empty())
				error::InvalidArgument(SL, "Field <encryption> is missing or empty").throwException();
			m_vSerializeConfig = Dictionary({ {"encryption", sEncryption} });
		}
	}
	LOGLVL(Debug, "JSON-RPC client is created (channelMode: <" <<
		getChannelModeString(m_eChannelMode) << ">)");
	TRACE_END("Error during configuration");
}

//
//
//
void JsonRpcClient::put(const Variant& vData)
{
	(void)execute("put", Dictionary({ {"data", vData} }));
}

//
//
//
Variant JsonRpcClient::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, FMT("Call method <" << vCommand << ">"));
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Parameters:\n" << vParams);

	if (!vParams.isNull() && !vParams.isDictionaryLike() && !vParams.isSequenceLike())
		error::InvalidArgument(SL, "Invalid type of parameters").throwException();

	Json::Value params;
	if (m_eChannelMode == ChannelMode::Encrypted)
	{
		Variant vData = Dictionary({ {"command", vCommand}, {"params", vParams} });
		encryptJsonValue(vData, params, m_vSerializeConfig);
	}
	else if (!vParams.isNull())
		params = convertVariantToJsonValue(vParams);

	Json::Value result;
	using namespace std::chrono_literals;
	std::chrono::milliseconds msTimeout(m_nTimeout);
	const double c_dMinDelay = 500;					// 500 ms
	const double c_dMaxDelay = 2.0 * 60.0 * 1000.0;	// 2 min
	const double c_dFactor = 3.0;
	const double c_dJitter = 0.1;
	double dDelay = c_dMinDelay;
	std::uniform_real_distribution<double> unif(0, 1);
	std::default_random_engine dre;
	auto startTime = std::chrono::steady_clock::now();
	Size nRetryCount = 0;
	while (true)
	{
		try
		{
			std::scoped_lock _lock(m_mtxClient);
			m_pClient->CallMethod(((m_eChannelMode == ChannelMode::Encrypted) ? 
				c_sEncryptedFieldName : vCommand), params, result);
			break;
		}
		catch (const jsonrpc::JsonRpcException& ex)
		{
			if (ex.GetCode() != jsonrpc::Errors::ERROR_SERVER_CONNECTOR &&
				ex.GetCode() != jsonrpc::Errors::ERROR_CLIENT_CONNECTOR)
			{
				Json::Value data = ex.GetData();
				if (m_eChannelMode == ChannelMode::Encrypted)
				{
					Variant vData;
					CMD_TRY
					{
						vData = decryptJsonValue(data);
					}
					CMD_PREPARE_CATCH
					catch (error::Exception& ex)
					{
						ex.logAsWarning("Failed to decrypt the RPC response");
					}
					if (vData.isDictionaryLike() && vData.has("exception"))
						error::Exception(vData["exception"]).throwException();
				}
				else if (data.isMember("exception"))
				{
					Variant vException = convertJsonValueToVariant(data["exception"]);
					error::Exception(vException).throwException();
				}
				error::RuntimeError(SL, ex.what()).throwException();
			}
			LOGLVL(Debug, "Error during RPC call server: <" << m_sConnectionInfo 
				<< ">, attempt <" << ++nRetryCount << ">, delay <" << (int)dDelay << "ms> (" << ex.what() << ")");

			auto msDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - startTime);
			auto msDelay = std::chrono::milliseconds((int)dDelay);
			if (m_nTimeout != c_nMaxSize)
			{
				if (msDuration > msTimeout)
					error::ConnectionError(SL, FMT("Error during RPC call server: < " << 
						m_sConnectionInfo << ">. " << ex.what())).throwException();
				if (msDuration + msDelay > msTimeout)
					msDelay = msTimeout - msDuration;
			}

			std::this_thread::sleep_for(msDelay);

			// Here we do the exponential backoff
			dDelay = std::min(dDelay * c_dFactor, c_dMaxDelay);
			dDelay += unif(dre) * c_dJitter * (msDuration.count() % 1000);
		}
	}

	if (m_eChannelMode == ChannelMode::Encrypted)
		return decryptJsonValue(result);

	return convertJsonValueToVariant(result);
	TRACE_END("Error during command execution");
}

//
//
//
void JsonRpcClient::processQueueEvent()
{
	CMD_TRY
	{
		auto pProvider = m_pProvider.lock();
		if (pProvider == nullptr)
			error::InvalidArgument(SL, "Provider interface is undefined").throwException();
		auto vEvent = pProvider->get();
		if (vEvent)
			put(vEvent.value());
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, "Fail to parse event from queue");
	}
	catch (...)
	{
		error::RuntimeError(SL, "Fail to parse event from queue").log();
	}
}

//
//
//
void JsonRpcClient::notifyAddQueueData(Variant vTag)
{
	if (m_pProvider.expired())
	{
		auto pQm = queryInterface<IQueueManager>(queryService("queueManager"));
		m_pProvider = queryInterface<IDataProvider>(pQm->getQueue(std::string(vTag)));
	}
	m_threadPool.run(&JsonRpcClient::processQueueEvent, this);
}

//
//
//
void JsonRpcClient::notifyQueueOverflowWarning(Variant vTag)
{
}

} // namespace ipc
} // namespace cmd 
