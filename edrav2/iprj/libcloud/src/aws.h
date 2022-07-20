//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (11.03.2019)
// Reviewer: Denis Bogdanov (15.03.2019)
//
///
/// @file AWS objects declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once
#include <objects.h>

namespace cmd {
namespace cloud {
namespace aws {

///
/// AWS Core initializer/deinitializer.
///
class Core : public ObjectBase<CLSID_AwsCore>
{
private:
	Aws::SDKOptions m_options;

protected:

	///
	/// Destructor.
	///
	~Core() override;

public:

	///
	/// Object final construction.
	///
	/// During object's construction the base config from the Catalog's 'aws'
	/// section is used (if exists). The custom configuration supplied in 
	/// parameters overrides corresponent fields of the base config.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **logLevel** (Optional) a level (integer) of the AWS SDK's logging
	///   (0-Off .. 6-Trace). By default, the logging is disabled (Off).
	///
	void finalConstruct(Variant vConfig);
};

///
/// Amazon Kinesis Data Firehose client.
///
class FirehoseClient : public ObjectBase<CLSID_AwsFirehoseClient>,
	public ICommandProcessor
{
private:

	ObjPtr<IObject> m_pAwsCore = nullptr;
	std::unique_ptr<Aws::Firehose::FirehoseClient> m_pClient = nullptr;
	std::mutex m_mtxClient;
	std::string m_sDeliveryStream;

	bool publish(const Variant& vData);

public:

	///
	/// Object final construction.
	///
	/// During object's construction the base config from the Catalog's 'aws'
	/// section is used (if exists). The custom configuration supplied in 
	/// parameters overrides corresponent fields of the base config.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **accessKey** - the access key for AWS authentication
	///   **secretKey** - the secret key for AWS authentication
	///   **deliveryStream** - the name of the Firehose's delivery stream
	///   **region** - (Optional) the name of AWS region. By default,
	///  us-west-2.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute(Variant,Variant)
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace aws
} // namespace cloud
} // namespace cmd

/// @} 
