//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (11.03.2019)
// Reviewer: Denis Bogdanov (15.03.2019)
//
///
/// @file AWS objects implementation
///
/// @addtogroup cloud Cloud communication library
/// @{
#include "pch.h"
#include "aws.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "aws"

namespace openEdr {
namespace cloud {
namespace aws {

//////////////////////////////////////////////////////////////////////////////
//
// Core
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
Core::~Core()
{
	Aws::ShutdownAPI(m_options);
}

//
//
//
void Core::finalConstruct(Variant vConfig)
{
	Variant vFullConfig;
	Variant vBaseConfig = getCatalogData("app.config.cloud.aws", {});
	if (!vBaseConfig.isNull())
		vFullConfig = vBaseConfig.clone();
	if (!vConfig.isNull())
		vFullConfig.merge(vConfig, Variant::MergeMode::All);

	m_options.loggingOptions.logLevel = static_cast<Aws::Utils::Logging::LogLevel>(
		vConfig.get("logLevel", Aws::Utils::Logging::LogLevel::Off));
	Aws::InitAPI(m_options);
}

//////////////////////////////////////////////////////////////////////////////
//
// FirehoseClient
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
void FirehoseClient::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	// Thread-safe AWS Core initialization
	Variant vAwsCore = initCatalogData("objects.awsCore",
		createObjProxy(Dictionary({ { "clsid", CLSID_AwsCore } }), /* fCached */ true));
	m_pAwsCore = queryInterface<IObject>(vAwsCore);

	Variant vFullConfig;
	Variant vBaseConfig = getCatalogData("app.config.aws", {});
	if (!vBaseConfig.isNull())
		vFullConfig = vBaseConfig.clone();
	if (!vConfig.isNull())
		vFullConfig.merge(vConfig, Variant::MergeMode::All);

	m_sDeliveryStream = vFullConfig["deliveryStream"];
	Aws::Auth::AWSCredentials credentials(vFullConfig["accessKey"], vFullConfig["secretKey"]);
	Aws::Client::ClientConfiguration config;
	config.region = vFullConfig.get("region", Aws::Region::US_WEST_2);
	m_pClient = std::make_unique<Aws::Firehose::FirehoseClient>(credentials, config);
	TRACE_END("Error during configuration");
}

//
//
//
bool FirehoseClient::publish(const Variant& vData)
{
	TRACE_BEGIN;
	Aws::Firehose::Model::PutRecordBatchRequest request;
	request.SetDeliveryStreamName(m_sDeliveryStream);

	Aws::Vector<Aws::Firehose::Model::Record> verRecords;
	if (vData.isDictionaryLike())
	{
		std::string sData = variant::serializeToJson(vData, variant::JsonFormat::SingleLine);
		sData += '\n';
		Aws::Utils::ByteBuffer bytes((unsigned char*)sData.c_str(), sData.length());
		Aws::Firehose::Model::Record record;
		record.SetData(bytes);
		verRecords.emplace_back(record);
	}
	else if (vData.isSequenceLike())
		for (const auto& vElem : Sequence(vData))
		{
			std::string sData = variant::serializeToJson(vElem, variant::JsonFormat::SingleLine);
			sData += '\n';
			Aws::Utils::ByteBuffer bytes((unsigned char*)sData.c_str(), sData.length());
			Aws::Firehose::Model::Record record;
			record.SetData(bytes);
			verRecords.emplace_back(record);
		}
	else
		error::InvalidArgument(SL, "Field <data> must be a sequence or a dictionary").throwException();

	request.SetRecords(verRecords);
	{
		std::scoped_lock lock(m_mtxClient);
		auto outcome = m_pClient->PutRecordBatch(request);
		if (outcome.IsSuccess())
		{
			LOGLVL(Trace, "Data has been successfully published");
			return true;
		}
		LOGLVL(Debug, "Failed to publish (" << outcome.GetError().GetMessage() << ")");
	}

	return false;
	TRACE_END("Error during publishing data to the server");
}

///
/// #### Supported commands
///
Variant FirehoseClient::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command's parameters:\n" << vParams);

	///
	/// @fn Variant ValkyrieService::execute()
	///
	/// ##### submit()
	/// Submits the data to the server.
	///   * data [Variant] - data to be sent.
	///   * default [bool, opt] - returned value if exception is occured.
	///
	/// Returns true if submission is success (or item should be skipped).
	/// Returns false if submission failed and repeated submit is required.
	///
	if (vCommand == "submit")
	{
		if (!vParams.has("data"))
			error::InvalidArgument(SL, "Missing field <data> in parameters").throwException();
		if (!vParams.has("default"))
			return publish(vParams["data"]);

		CMD_TRY
		{
			return publish(vParams["data"]);
		}
			CMD_PREPARE_CATCH
			catch (const error::Exception& ex)
		{
			LOGLVL(Debug, ex.what());
			return vParams["default"];
		}
	}

	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace aws
} // namespace cloud
} // namespace openEdr

/// @}
