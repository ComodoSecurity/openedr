//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (20.05.2019)
// Reviewer: Yury Podpruzhnikov (04.06.2019)
//
///
/// @file Google Cloud objects implementation
///
/// @addtogroup cloud Cloud communication library
/// @{
#include "pch.h"
#include "gcp.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "gcp"

namespace openEdr {
namespace cloud {
namespace gcp {


static ProtobufDeleter g_protobufDeleter(nullptr,
	[](int* /*ptr*/)
	{
		// Free all internal static variables to avoid memory leaks
		google::protobuf::ShutdownProtobufLibrary();
	});

ProtobufDeleter getProtobufDeleter()
{
	return g_protobufDeleter;
}

//
//
//
void PubSubClient::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	m_vConfig = vConfig.clone();
	updateSettings(vConfig);
	m_nTimeout = vConfig.get("timeout", m_nTimeout);

	m_fStatEnabled = vConfig.get("enableStatistic", m_fStatEnabled);
	m_nStatFlushTimeout = vConfig.get("statFlushTimeout", m_nStatFlushTimeout);

	Size nNumOfAttempts = vConfig.get("numOfAttempts", m_nNumOfAttempts);
	if (nNumOfAttempts > 0)
		m_nNumOfAttempts = nNumOfAttempts;
	else
		LOGWRN("<numOfAttempts> is too small. Default is used (<" << m_nNumOfAttempts << ">)");

	// Define high 8 bits to a random value in order to provide a unique sequence IDs
	// across the all possibly running PubSubClient instances.
	auto nSeed = (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::mt19937 rng(nSeed);
	std::uniform_int_distribution<std::mt19937::result_type> dist(1, UINT8_MAX);
	m_nSequenceId = (uint32_t)dist(rng) << 24;

	if (m_fStatEnabled)
	{
		auto optStatFilePath = vConfig.getSafe("statFilePath");
		if (!optStatFilePath.has_value())
			error::InvalidArgument(SL, "Missing <statFilePath>").throwException();

		std::filesystem::path pathStatFile = optStatFilePath.value().convertToPath();
		m_pStatWriteStream = queryInterface<io::IWritableStream>(io::createFileStream(pathStatFile,
			io::FileMode::Write | io::FileMode::ShareRead | io::FileMode::CreatePath | io::FileMode::Truncate));

		// avoid recursive pointing timer <-> PubSubClient
		// It is better to start timer on the first filtration request
		ObjWeakPtr<PubSubClient> weakPtrToThis(getPtrFromThis(this));
		m_pStatFlushTimer = runWithDelay(m_nStatFlushTimeout,
			[weakPtrToThis]() -> bool
			{
				auto pThis = weakPtrToThis.lock();
				if (pThis == nullptr)
					return false;
				return pThis->flushStatistic();
			});

		subscribeToMessage(Message::AppFinishing, getPtrFromThis(this), "GcpPubSubClient");
	}

	subscribeToMessage(Message::CloudConfigurationIsChanged, getPtrFromThis(this), "GcpPubSubClient");
	TRACE_END("Error during configuration");
}

//
//
//
void PubSubClient::updateSettings(Variant vConfig)
{
	TRACE_BEGIN;
	if (!m_vConfig.has("saCredentials"))
		error::InvalidArgument(SL, "Missing field <saCredentials>").throwException();
	if (!m_vConfig.has("pubSubTopic"))
		error::InvalidArgument(SL, "Missing field <pubSubTopic>").throwException();

	std::scoped_lock lock(m_mtxPublisher);

	if (vConfig.has("saCredentials"))
	{
		connect(vConfig["saCredentials"]);
		m_sTopic = FMT("projects/" << m_sProjectId << "/topics/" <<
			(std::string_view)(vConfig.has("pubSubTopic") ? vConfig["pubSubTopic"] : m_vConfig["pubSubTopic"]));
	}
	
	if (vConfig.has("pubSubTopic"))
		m_sTopic = FMT("projects/" << m_sProjectId << "/topics/" << (std::string_view)vConfig["pubSubTopic"]);
	TRACE_END("Error during settings update");
}

//
// Run this function only under mutex m_mtxPublisher
//
void PubSubClient::connect(const std::string& sSaCredentials)
{
	//auto pCredentials = ::grpc::GoogleDefaultCredentials();
	auto pCallCredentials = ::grpc::ServiceAccountJWTAccessCredentials(sSaCredentials);
	::grpc::SslCredentialsOptions sslOptions;
	sslOptions.pem_root_certs = m_vConfig["caCertificates"];
	auto pSslCredentials = ::grpc::SslCredentials(sslOptions);
	auto pChannelCredentials = ::grpc::CompositeChannelCredentials(pSslCredentials, pCallCredentials);
	auto pChannel = ::grpc::CreateChannel("pubsub.googleapis.com", pChannelCredentials);
	m_pPublisher = ::google::pubsub::v1::Publisher::NewStub(pChannel);
	m_sProjectId = variant::deserializeFromJson(sSaCredentials)["project_id"];

	// Save credentials for further reconnect attempts
	m_sSaCredentials = sSaCredentials;
}

//
//
//
void PubSubClient::reconnect()
{
	std::scoped_lock lock(m_mtxPublisher);
	connect(m_sSaCredentials);
}

//
//
//
bool PubSubClient::publish(const Variant& vData)
{
	TRACE_BEGIN;
	::google::pubsub::v1::PublishRequest request;
	if (vData.isDictionaryLike())
		request.add_messages()->set_data(prepareSingleEvent(vData));
	else if (vData.isSequenceLike())
	{
		for (const auto& vElem : Sequence(vData))
			request.add_messages()->set_data(prepareSingleEvent(vElem));
	}
	else
		error::InvalidArgument(SL, "Field <data> must be a sequence or a dictionary").throwException();

	Size nTimeout = m_nTimeout;
	constexpr double c_dFactor = 1.5;
	
	::grpc::Status status;
	Size nAttempt = 0;
	while (++nAttempt <= m_nNumOfAttempts)
	{
		// Check if application is finishing
		std::string sAppStage = getCatalogData("app.stage", "finished");
		if ((sAppStage == "finishing") || (sAppStage == "finished"))
		{
			LOGLVL(Debug, "Skip any further attempts due the application is finishing now");
			return false;
		}

		status = publishInt(request, nTimeout);
		if (status.ok())
		{
			LOGLVL(Trace, "Data has been successfully published");
			if (m_fServerUnavailable)
			{
				LOGINF("Remote server is available again");
				m_fServerUnavailable = false;
			}
			if (m_fStatEnabled)
			{
				if (vData.isDictionaryLike())
					addStatistic(vData);
				else if (vData.isSequenceLike())
				{
					for (const auto& vElem : Sequence(vData))
						addStatistic(vElem);
				}
			}
			return true;
		}
		if ((status.error_code() == ::grpc::StatusCode::UNAUTHENTICATED) ||
			(status.error_code() == ::grpc::StatusCode::UNAVAILABLE))
			reconnect();

		LOGLVL(Debug, "[" << nAttempt << "\\" << m_nNumOfAttempts << "] Failed to publish (" <<
			status.error_code() << " - " << status.error_message() << ". " << status.error_details() << ")");
		nTimeout = std::lrint(nTimeout * c_dFactor);
	}
	
	if (!m_fServerUnavailable)
	{
		LOGINF("Remote server is unavailable (" << status.error_code() << " - " << status.error_message() <<
			")");
		m_fServerUnavailable = true;
	}
	return false;
	TRACE_END("Error during publishing data to the server");
}

//
//
//
void PubSubClient::addStatistic(const Variant& vData)
{
	std::string sEventData = prepareSingleEvent(vData);

	std::string sEventType0;
	auto optEventType0 = vData.getSafe("baseType");
	if (optEventType0.has_value() && !optEventType0.value().isNull())
	{
		try
		{
			sEventType0 = getEventTypeString(optEventType0.value());
		}
		catch (error::InvalidArgument&) {}
	}
	if (sEventType0.empty())
		sEventType0 = "<MLE>";

	std::string sEventType1;
	auto optEventType1 = vData.getSafe("type");
	if (optEventType1.has_value())
		sEventType1 = variant::convert<variant::ValueType::String>(optEventType1.value());
	if (sEventType1.empty())
		sEventType1 = "<OTHER>";

	Size nSize = sEventData.size();

	std::scoped_lock lock(m_mtxStat);
	auto it0 = m_statData.find(sEventType0);
	if (it0 == m_statData.end())
	{
		EventStatMap1 newItem{ { sEventType1, EventStat(1, nSize) }};
		m_statData.emplace(sEventType0, newItem);
		return;
	}

	auto it1 = it0->second.find(sEventType1);
	if (it1 == it0->second.end())
	{
		it0->second.emplace(sEventType1, EventStat(1, nSize));
		return;
	}

	it1->second.nCount++;
	it1->second.nSize += nSize;
}

//
//
//
Variant PubSubClient::getStatistic()
{
	Variant vResult = Dictionary();
	std::scoped_lock lock(m_mtxStat);
	for (const auto& [sEventType0, data1] : m_statData)
	{
		for (const auto& [sEventType1, stat] : data1)
		{
			std::string sEventType(sEventType0);
			sEventType += '.';
			sEventType += sEventType1;
			vResult.put(sEventType, Dictionary({
				{"count", stat.nCount},
				{"size", stat.nSize}
				}));
		}
	}
	return vResult;
}

//
//
//
template<class...Durations, class DurationIn>
std::tuple<Durations...> break_down_durations(DurationIn d)
{
	std::tuple<Durations...> retval;
	using discard = int[];
	(void)discard
	{
		0, (void((
			(std::get<Durations>(retval) = std::chrono::duration_cast<Durations>(d)),
			(d -= std::chrono::duration_cast<DurationIn>(std::get<Durations>(retval)))
			)), 0)...
	};
	return retval;
}

//
//
//
inline std::string formatTimeDuration(Time duration)
{
	using days = std::chrono::duration<int, std::ratio<86400>>;

	auto clean_duration = break_down_durations<days, std::chrono::hours, std::chrono::minutes,
		std::chrono::seconds>(std::chrono::milliseconds(duration));

	auto nDays = std::get<0>(clean_duration).count();
	auto nHours = std::get<1>(clean_duration).count();
	auto nMinutes = std::get<2>(clean_duration).count();
	auto nSeconds = std::get<3>(clean_duration).count();

	std::string s;
	if (nDays > 0)
	{
		s += std::to_string(nDays);
		s += "d ";
	}
	if (nHours > 0)
	{
		s += std::to_string(nHours);
		s += "h ";
	}
	if (nMinutes > 0)
	{
		s += std::to_string(nMinutes);
		s += "m ";
	}
	if (nSeconds > 0)
	{
		s += std::to_string(nSeconds);
		s += 's';
	}
	return s;
}

//
//
//
bool PubSubClient::flushStatistic()
{
	TRACE_BEGIN;
	if (!m_pStatWriteStream)
		return false;

	// Get a local copy of all the statistic
	EventStatMap statData;
	{
		std::scoped_lock lock(m_mtxStat);
		if (m_statData.empty())
			return true;
		statData = m_statData;
	}

	constexpr std::array<std::array<const char*, 5>, 2> columnNames = {{
		{ "EventType", "Total", "", "By policy", "" },
		{ "", "Count", "Size", "Count", "Size" } }};
	std::array<Size, 5> columnWidths = { strlen(columnNames[0][0]), strlen(columnNames[0][1]),
		strlen(columnNames[1][2]), strlen(columnNames[1][3]), strlen(columnNames[1][4]) };
	constexpr Size c_nInterColumnSpacing = 2;
	constexpr Size c_nIndent = 4;

	// 1-st pass to calculate the width of the columns
	EventStat totalStat;

	EventStatIndex index;
	for (const auto& [sEventType0, data1] : statData)
	{
		EventStatIndexItem index0Item(sEventType0);
		for (const auto& [sEventType1, stat] : data1)
		{
			std::string sEventType(c_nIndent, ' ');
			sEventType += sEventType1;

			index0Item.stat += stat;
			index0Item.index.insert(std::make_pair(stat.nSize, EventStatIndexItem(sEventType, stat)));

			Size nEventTypeWidth = sEventType.size();
			if (nEventTypeWidth > columnWidths[0])
				columnWidths[0] = nEventTypeWidth;
		}
		index.insert(std::make_pair(index0Item.stat.nSize, index0Item));
		totalStat += index0Item.stat;

		Size nEventTypeWidth = sEventType0.size();
		if (nEventTypeWidth > columnWidths[0])
			columnWidths[0] = nEventTypeWidth;
	}

	// Fix width for "Count" and "Size" columns
	auto sTotalCount = std::to_string(totalStat.nCount);
	Size nTotalCountWidth = sTotalCount.size();
	if (nTotalCountWidth > columnWidths[1])
		columnWidths[1] = nTotalCountWidth;
	if (nTotalCountWidth > columnWidths[3])
		columnWidths[3] = nTotalCountWidth;
	auto sTotalSize = std::to_string(totalStat.nSize);
	Size nTotalSizeWidth = sTotalSize.size();
	if (nTotalSizeWidth > columnWidths[2])
		columnWidths[2] = nTotalSizeWidth;
	if (nTotalSizeWidth > columnWidths[4])
		columnWidths[4] = nTotalSizeWidth;

	// 2-nd pass to output data to the body
	std::string sBody;
	for (const auto& [nSize0, index0Item] : index)
	{
		// Event type (left-aligned)
		sBody += index0Item.sEventType;
		Size nEventTypeWidth = index0Item.sEventType.size();
		if (nEventTypeWidth < columnWidths[0])
			sBody += std::string(columnWidths[0] - nEventTypeWidth, ' ');
		sBody += std::string(c_nInterColumnSpacing + 1, ' ');

		// Count (right-aligned)
		auto sCount = std::to_string(index0Item.stat.nCount);
		Size nCountWidth = sCount.size();
		if (nCountWidth < columnWidths[1])
			sBody += std::string(columnWidths[1] - nCountWidth, ' ');
		sBody += sCount;
		sBody += std::string(c_nInterColumnSpacing, ' ');

		// Size (right-aligned)
		auto sSize = std::to_string(index0Item.stat.nSize);
		Size nSizeWidth = sSize.size();
		if (nSizeWidth < columnWidths[2])
			sBody += std::string(columnWidths[2] - nSizeWidth, ' ');
		sBody += sSize;
		sBody += '\n';

		for (const auto& [nSize1, index1Item] : index0Item.index)
		{
			// Event type (left-aligned)
			sBody += index1Item.sEventType;
			nEventTypeWidth = index1Item.sEventType.size();
			if (nEventTypeWidth < columnWidths[0])
				sBody += std::string(columnWidths[0] - nEventTypeWidth, ' ');

			// Add blank spacing for 2nd and 3rd columns
			sBody += std::string(columnWidths[1] + columnWidths[2] + c_nInterColumnSpacing * 3 + 2, ' ');

			// Count (right-aligned)
			sCount = std::to_string(index1Item.stat.nCount);
			nCountWidth = sCount.size();
			if (nCountWidth < columnWidths[3])
				sBody += std::string(columnWidths[3] - nCountWidth, ' ');
			sBody += sCount;
			sBody += std::string(c_nInterColumnSpacing, ' ');

			// Size (right-aligned)
			sSize = std::to_string(index1Item.stat.nSize);
			nSizeWidth = sSize.size();
			if (nSizeWidth < columnWidths[4])
				sBody += std::string(columnWidths[4] - nSizeWidth, ' ');
			sBody += sSize;
			sBody += '\n';
		}
	}

	// Construct the header
	std::string sHeader;

	// Time period: 1d 3h 33m 12s (since 2019-10-16T12:01:54.000Z)
	Time appStartTime = getCatalogData("app.startTime", 0);
	Time duration = getCurrentTime() - appStartTime;
	sHeader += "Time period: ";
	sHeader += formatTimeDuration(duration);
	sHeader += " (since ";
	sHeader += getIsoTime(appStartTime);
	sHeader += ")\n\n";

	Size nLineWidth = std::accumulate(columnWidths.begin(), columnWidths.end(),
		c_nInterColumnSpacing * (columnWidths.size() - 1) + 2);
	sHeader += std::string(nLineWidth, '-');
	sHeader += '\n';

	for (Size nLine = 0; nLine < 2; ++nLine)
	{
		for (Size nColumn = 0; nColumn < columnWidths.size(); ++nColumn)
		{
			// Insert spacing for right-aligned heading
			if ((nColumn != 0) && (nLine != 0))
				sHeader += std::string(columnWidths[nColumn] - strlen(columnNames[nLine][nColumn]), ' ');
			sHeader += columnNames[nLine][nColumn];
			// Insert spacing for left-aligned heading
			if (((nColumn == 0) || (nLine == 0)) &&
				(columnWidths[nColumn] > strlen(columnNames[nLine][nColumn])))
				sHeader += std::string(columnWidths[nColumn] - strlen(columnNames[nLine][nColumn]), ' ');
			// Insert pipe symbol before 2nd and 4th columns
			if ((nColumn == 0) || (nColumn == 2))
				sHeader += " | ";
			else
				sHeader += std::string(c_nInterColumnSpacing, ' ');
		}
		// Cutoff the spacing after the last column
		sHeader.resize(sHeader.size() - c_nInterColumnSpacing);
		sHeader += '\n';
	}

	sHeader += std::string(nLineWidth, '-');
	sHeader += '\n';

	// Construct the footer
	constexpr char c_sTotalCaption[] = "Total";
	std::string sFooter(nLineWidth, '-');
	sFooter += "\n";
	sFooter += c_sTotalCaption;
	sFooter += std::string(columnWidths[0] - strlen(c_sTotalCaption), ' ');
	sFooter += std::string(c_nInterColumnSpacing + 1, ' ');
	sFooter += std::string(columnWidths[1] - nTotalCountWidth, ' ');
	sFooter += sTotalCount;
	sFooter += std::string(c_nInterColumnSpacing, ' ');
	sFooter += std::string(columnWidths[2] - nTotalSizeWidth, ' ');
	sFooter += sTotalSize;
	sFooter += std::string(c_nInterColumnSpacing + 1, ' ');
	sFooter += std::string(columnWidths[3] - nTotalCountWidth, ' ');
	sFooter += sTotalCount;
	sFooter += std::string(c_nInterColumnSpacing, ' ');
	sFooter += std::string(columnWidths[4] - nTotalSizeWidth, ' ');
	sFooter += sTotalSize;
	sFooter += "\n";

	// Write all to the stream
	m_pStatWriteStream->setSize(0);
	io::write(m_pStatWriteStream, sHeader);
	io::write(m_pStatWriteStream, sBody);
	io::write(m_pStatWriteStream, sFooter);
	m_pStatWriteStream->flush();
	LOGLVL(Debug, "Event sending statistic has been flushed");
	return true;
	TRACE_END("Error while flushing the statistic");
}

//
//
//
std::string PubSubClient::prepareSingleEvent(const Variant& vData)
{
	Variant vTmp = vData.clone();
	vTmp.put("sequenceId", ++m_nSequenceId);
	std::string sResult = variant::serializeToJson(vTmp, variant::JsonFormat::SingleLine);
	sResult += '\n';
	return sResult;
}

//
//
//
::grpc::Status PubSubClient::publishInt(::google::pubsub::v1::PublishRequest& request, Size nTimeout)
{
	std::scoped_lock lock(m_mtxPublisher);
	::grpc::ClientContext context;
	::google::pubsub::v1::PublishResponse response;

	request.set_topic(m_sTopic);

	if (nTimeout != 0)
	{
		auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(nTimeout);
		context.set_deadline(deadline);
	}
	LOGLVL(Trace, "Publish data to server <timeout=" << nTimeout << ">");

	return m_pPublisher->Publish(&context, request, &response);
}

///
/// #### Supported commands
///
Variant PubSubClient::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command's parameters:\n" << vParams);

	///
	/// @fn Variant GcpPubSubClient::execute()
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
			error::InvalidArgument(SL, "Missing field <data>").throwException();
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
	///
	/// @fn Variant GcpPubSubClient::execute()
	///
	/// ##### updateSettings()
	/// Updates the settings.
	///
	else if (vCommand == "updateSettings")
	{
		updateSettings(vParams);
		return true;
	}
	///
	/// @fn Variant GcpPubSubClient::execute()
	///
	/// ##### getStatistic()
	/// Gets the event sending statistic.
	///
	else if (vCommand == "getStatistic")
		return getStatistic();

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
}

//
// Messages handler
//
Variant PubSubClient::execute(Variant vParam)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process message <" << vParam["messageId"] << ">");
	Variant vData = vParam.get("data", {});
	if (!vData.isNull())
		LOGLVL(Trace, "Message's data:\n" << vData);

	if (vParam["messageId"] == Message::CloudConfigurationIsChanged)
	{
		// TODO: Here we read a GCP configuration from Global Catalog directly
		// because there is no more convenient way to pass parameters via
		// sending a message
		Variant vConfig = getCatalogData("app.config.cloud.gcp", Dictionary());
		updateSettings(vConfig);
		return {};
	}
	else if (vParam["messageId"] == Message::AppFinishing)
	{
		if (!m_fStatEnabled)
			return {};
		if (m_pStatFlushTimer)
			m_pStatFlushTimer->cancel();
		flushStatistic();
		return {};
	}
	return {};
	TRACE_END(FMT("Error during processing of the message <" << vParam["messageId"] << ">"));
}

//
//
//
void logFunction(gpr_log_func_args* args)
{
	if (args->severity == GPR_LOG_SEVERITY_ERROR)
	{
		openEdr::SourceLocation srcLoc{ openEdr::SourceLocationTag(), args->file, args->line, CMD_COMPONENT };
		error::RuntimeError(srcLoc, args->message).log();
	}
	else if (args->severity == GPR_LOG_SEVERITY_INFO)
	{
		LOGLVL(Detailed, args->message);
	}
	else if (args->severity == GPR_LOG_SEVERITY_DEBUG)
	{
		LOGLVL(Debug, args->message);
	}
}

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "gcpctrl"

//
//
//
void Controller::finalConstruct(Variant vConfig)
{
}

///
/// #### Supported commands
///
Variant Controller::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant GcpController::execute()
	///
	/// ##### initializeLogging()
	/// Initializes logging of GCP library.
	/// Parameters:
	/// * logLevel [int, opt] - -1-disabled, 0-debug, 1-info, 2-error(default).
	/// * tracers [seq, opt] - list of enabled tracers. 
	///     Available tracer names see values of "GRPC_TRACE" in  
	///     https://github.com/grpc/grpc/blob/master/doc/environment_variables.md
	///
	if (vCommand == "initializeLogging")
	{
		int nLogLevel = vParams.get("logLevel", GPR_LOG_SEVERITY_ERROR);
		if (nLogLevel < GPR_LOG_VERBOSITY_UNSET || nLogLevel > GPR_LOG_SEVERITY_ERROR)
			error::InvalidArgument(SL, FMT("logLevel = " << nLogLevel)).throwException();
		gpr_set_log_verbosity((gpr_log_severity)nLogLevel);

		if (nLogLevel == GPR_LOG_VERBOSITY_UNSET)
		{
			grpc_tracer_set_enabled("all", false);
			return {};
		}

		gpr_set_log_function(logFunction);

		Variant vTracers = vParams.get("tracers", Variant());
		if (!vTracers.isNull())
			for (Variant vTracerName : vTracers)
				grpc_tracer_set_enabled(std::string(vTracerName).c_str(), true);

		return {};
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
}

} // namespace gcp
} // namespace cloud
} // namespace openEdr
/// @}
