//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (20.05.2019)
// Reviewer: Yury Podpruzhnikov (04.06.2019)
//
///
/// @file Google Cloud objects declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once
#include <objects.h>

namespace openEdr {
namespace cloud {
namespace gcp {

using ProtobufDeleter = std::shared_ptr<int>;

//
// Each protobuf user keep additional pointer to ProtobufDeleter
// to avoid its delete before deletion of all its users
//
ProtobufDeleter getProtobufDeleter();

///
/// Google PubSub client.
///
class PubSubClient : public ObjectBase<CLSID_GcpPubSubClient>,
	public ICommandProcessor,
	public ICommand
{
private:
	ProtobufDeleter m_protobufDeleter = getProtobufDeleter();

	std::unique_ptr<::google::pubsub::v1::Publisher::Stub> m_pPublisher;
	std::mutex m_mtxPublisher;
	Variant m_vConfig;
	std::string m_sSaCredentials;
	std::string m_sTopic;
	std::string m_sProjectId;
	Size m_nTimeout = 5; //< in seconds
	Size m_nNumOfAttempts = 3;
	std::atomic_uint32_t m_nSequenceId = 0;
	bool m_fServerUnavailable = false;
	
	// Event sending statistic
	struct EventStat
	{
		Size nCount = 0;
		Size nSize = 0;

		EventStat() = default;
		//EventStat(const EventStat&) = default;
		//EventStat(EventStat&&) = default;
		//EventStat& operator=(const EventStat&) = default;
		//EventStat& operator=(EventStat&&) = default;

		EventStat(Size _nCount, Size _nSize) :
			nCount(_nCount), nSize(_nSize) {}

		EventStat& operator+=(const EventStat& other)
		{
			nCount += other.nCount;
			nSize += other.nSize;
			return *this;
		}
	};
	using EventStatMap1 = std::unordered_map<std::string, EventStat>;
	using EventStatMap = std::unordered_map<std::string, EventStatMap1>;

	struct EventStatIndexItem;
	using EventStatIndex = std::multimap<Size, EventStatIndexItem, std::greater<Size>>;

	struct EventStatIndexItem
	{
		std::string sEventType;
		EventStat stat;
		EventStatIndex index;

		//EventStatIndexItem() = default;
		//EventStatIndexItem(const EventStatIndexItem&) = default;
		//EventStatIndexItem(EventStatIndexItem&&) = default;
		//EventStatIndexItem& operator=(const EventStatIndexItem&) = default;
		//EventStatIndexItem& operator=(EventStatIndexItem&&) = default;

		EventStatIndexItem(const std::string& _sEventType) :
			sEventType(_sEventType)
		{}
		EventStatIndexItem(const std::string& _sEventType, const EventStat& _stat) :
			sEventType(_sEventType), stat(_stat)
		{}
	};

	EventStatMap m_statData;
	bool m_fStatEnabled = false;
	std::mutex m_mtxStat;
	static constexpr Time c_nDefaultStatFlushTimeout = 5 * 60 * 1000; // 5 min
	Time m_nStatFlushTimeout = c_nDefaultStatFlushTimeout;
	ThreadPool::TimerContextPtr m_pStatFlushTimer;
	ObjPtr<io::IWritableStream>	m_pStatWriteStream;

	void addStatistic(const Variant& vData);
	Variant getStatistic();
	bool flushStatistic();
	void updateSettings(Variant vConfig);
	void connect(const std::string& sSaCredentials);
	void reconnect();
	std::string prepareSingleEvent(const Variant& vData);
	::grpc::Status publishInt(::google::pubsub::v1::PublishRequest& request, Size nTimeout);
	bool publish(const Variant& vData);

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including the following fields:
	///   **saCredentials** [str] - Google PubSub service account's cresdentials.
	///   **pubSubTopic** [str] - the topic name.
	///   **caCertificates** [str] - the bundle of CA root certificates.
	///   **timeout** [int,opt] - timeout (in seconds) (default: 5 sec).
	///     In order to define an infinite timeout, use value `0`.
	///   **numOfAttempts** [int,opt] - number of attempts to send data on the server.
	///   **enableStatistic** [bool,opt] - enable event sendining statistic gathering (default: false).
	///   **statFilePath** [str] - path to the file with event sending statistic. Required only if
	///     enableStatistic is `true`.
	///   **statFlushTimeout** [int,opt] - a timeout of flushing the event sending statistic
	///     (in milliseconds) (default: 5 min).
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute(Variant,Variant)
	Variant execute(Variant vCommand, Variant vParams) override;

	// ICommand

	/// @copydoc ICommand::execute(Variant)
	Variant execute(Variant vParam) override;
	/// @copydoc ICommand::getInfo()
	std::string getDescriptionString() override { return {}; }
};

///
/// GCP library controller.
///
class Controller : public ObjectBase<CLSID_GcpController>,
	public ICommandProcessor
{
protected:

	//
	//
	//
	~Controller() override {}

public:

	///
	/// Object final construction.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace gcp
} // namespace cloud
} // namespace openEdr

/// @} 
