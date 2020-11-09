//
// edrav2.libcore
// 
// Author: Denis Bogdanov (26.02.2019)
// Reviewer: Yury Podpruzhnikov (13.03.2019)
//
///
/// @file Data generator class implementation
///
/// @addtogroup dataGenerators
/// @{
#include "pch.h"
#include "io.hpp"
#include "datagenerator.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "datagen"

namespace openEdr {

//
//
//
DataGenerator::~DataGenerator()
{
	terminate();
};

///
/// Initializes the object with following parameters:
///   * data [dict] - Named data sets (each item is a sequence of a dictionary);
///   * start [str] - Name of inisial data set. If parameter is specified the 
///                   generation is started just after object's creation;
///   * receiver [obj, opt] - Pointer to IDataReceiver object. If it is specified, the
///                           SamplesProvider put data to it in async mode;
///   * count [int, opt] - Count of the items for generation. If it lager than
///                        the provided data size, then at the end the current  
///						   data set is used repeatedly from the start;
///   * period [int, opt] - Period of generation in milliseconds (it is used only 
///                         when 'receiver' parameter is specified;
///
void DataGenerator::finalConstruct(Variant vConfig)
{
	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "finalConstruct() supports only dictionary as a parameter")
			.throwException();

	if (!vConfig.has("data"))
		error::InvalidArgument(SL, "Data source is not specified or invalid").throwException();
	
	m_vData = vConfig["data"];	
	m_nPeriod = vConfig.get("period", m_nPeriod);
	m_fAutoStop = vConfig.get("autoStop", m_fAutoStop);
	
	if (vConfig.has("receiver"))
		m_pReceiver = vConfig["receiver"];

	if (vConfig.has("start"))
		start(vConfig.get("start", "default"), vConfig.get("count", c_nMaxSize));
}

//
//
//
bool DataGenerator::asyncSendData()
{
	//LOGLVL(Debug, "Async data sending");
	auto v = get();
	if (!v.has_value())
	{
		if (!m_fAutoStop)
		{
			if (m_nPeriod == 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return true;
		}
		stop();
		return false;
	}
	//LOGLVL(Trace, "Data:\n" << v.value());
	m_pReceiver->put(v.value());
	return true;
}

//
//
//
void DataGenerator::start(std::string_view sName, Size nCount)
{
	stop();
	std::scoped_lock lock(m_mtxData);
	m_fStop = false;
	m_nMaxCount = nCount;

	// Config-based data
	if (!m_vData.has(sName))
		error::NotFound(SL, FMT("Data set <" << sName << "> isn't found")).throwException();

	LOGLVL(Debug, "Start data generating using <" << sName << "> data set");

	auto vData = m_vData[sName];
	if (vData.isSequenceLike())
		m_seqCurrData = Sequence(vData);
	else if (vData.isDictionaryLike())
		m_seqCurrData.push_back(vData);
	else if (vData.isObject())
		m_pCurrProvider = queryInterface<IDataProvider>(vData);
	else
		error::InvalidArgument(SL, FMT("Data set <" << sName << "> has an invalid type")).throwException();

	if (m_nMaxCount == c_nMaxSize)
	{
		if (m_pCurrProvider)
		{
			auto pInfo = queryInterfaceSafe<IInformationProvider>(m_pCurrProvider);
			if (pInfo)
				m_nMaxCount = pInfo->getInfo().get("size", m_nMaxCount);
		}
		else
			m_nMaxCount = m_seqCurrData.getSize();
	}

	if (!m_pReceiver) return;
	
	LOGLVL(Debug, "Restart timer for async sending");
	if (m_pTimerCtx) m_pTimerCtx->reschedule();
	else m_pTimerCtx = m_threadPool.runWithDelay(m_nPeriod, &DataGenerator::asyncSendData, this);
}

//
// 
//
void DataGenerator::terminate()
{
	stop();
	m_threadPool.stop();
}

//
// 
//
void DataGenerator::stop()
{
	std::scoped_lock lock(m_mtxData);
	if (!m_fStop)
		LOGLVL(Debug, "Stop current data generating");
	m_fStop = true;
	m_nCurrCount = 0;
	m_nMaxCount = 0;
	m_seqCurrData = Sequence();
	m_pCurrProvider.reset();
	if (m_pTimerCtx)
		m_pTimerCtx->cancel();
}

//
//
//
std::optional<Variant> DataGenerator::get()
{
	std::scoped_lock lock(m_mtxData);
	if (m_fStop || m_nCurrCount >= m_nMaxCount) return std::nullopt;
	Size idx = m_nCurrCount++;
	++m_nTotalCount;

	// Get from internal data
	if (!m_seqCurrData.isEmpty())
		return m_seqCurrData[idx % m_seqCurrData.getSize()].clone();

	//if (m_pCurrProvider->getClassId() == CLSID_Queue)
 	//	m_pCurrProvider->getId();

	auto data = m_pCurrProvider->get();
	if (data.has_value())
		return data.value();
	
	--m_nTotalCount;
	--m_nCurrCount;

	if (m_fAutoStop && m_nMaxCount == c_nMaxSize)
		m_nMaxCount = m_nCurrCount;

	return std::nullopt;
}

//
//
//
Variant DataGenerator::getProgressInfo()
{
	std::scoped_lock sync(m_mtxData);
	return Dictionary({
			{"currPoint", m_nTotalCount},
			{"totalPoints", (m_fStop) ? m_nTotalCount : m_nMaxCount}
		});
}

//
//
//
Variant DataGenerator::getInfo()
{
	std::scoped_lock sync(m_mtxData);
	return Dictionary({
			{"maxCount", m_nMaxCount},
			{"stopped", m_fStop},
			{"period", m_nPeriod},
			{"count", m_nCurrCount}
		});
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant DataGenerator::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant DataGenerator::execute()
	///
	/// ##### get()
	/// Gets the data.
	///
	if (vCommand == "get")
	{
		auto v = get();
		return v.has_value()? v.value() : Variant();
	}

	///
	/// @fn Variant DataGenerator::execute()
	///
	/// ##### start()
	/// Starts samples generation.
	///   * name [str] - name of data set (default: 'default');
	///   * count [int] - count of the items for generation.
	/// 
	else if (vCommand == "start")
	{
		start(vParams.get("name", "default"), vParams.get("count", c_nMaxSize));
		return {};
	}

	///
	/// @fn Variant DataGenerator::execute()
	///
	/// ##### stop()
	/// Stops samples generation.
	/// 
	else if (vCommand == "stop")
	{
		stop();
		return {};
	}

	///
	/// @fn Variant DataGenerator::execute()
	///
	/// ##### terminate()
	/// Stops samples generation and frees all resources.
	/// 
	else if (vCommand == "terminate")
	{
		terminate();
		return {};
	}

	error::OperationNotSupported(SL, 
		FMT("SamplesProvider doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

////////////////////////////////////////////////////////////////////////////////
//
// RandomDataProvider class
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
void RandomDataGenerator::finalConstruct(Variant vConfig)
{
	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "finalConstruct() supports only dictionary as a parameter")
			.throwException();
	m_nStartValue = vConfig.get("start", 0);
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant RandomDataGenerator::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant RandomDataGenerator::execute()
	///
	/// ##### getNext()
	/// Returns next sequence value (starting from 'startValue' in config).
	///
	if (vCommand == "getNext")
	{
		return m_nStartValue++;
	}

	///
	/// @fn Variant RandomDataGenerator::execute()
	///
	/// ##### getIsoDateTime()
	///   * shift [int] - shift from current time (in ms).
	/// Returns date-time stamp in ISO 8601 format.
	///
	else if (vCommand == "getIsoDateTime")
	{
		// TODO: Move to common function
		int32_t nShift = vParams.get("shift", 0);
		auto now = std::chrono::system_clock::now() + std::chrono::milliseconds(nShift);
		auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		auto fraction = now - seconds;
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
		time_t cnow = std::chrono::system_clock::to_time_t(now);

		tm tmData;
		gmtime_s(&tmData, &cnow);
		return FMT(std::put_time(&tmData, "%Y-%m-%dT%H:%M:%S.") << 
			std::setfill('0') << std::setw(3) << milliseconds.count() << "Z");
	} 

	///
	/// @fn Variant RandomDataGenerator::execute()
	///
	/// ##### getDateTime()
	///   * shift [int] - shift from current time (in ms).
	/// Returns date-time stamp (in ms since epoch value).
	///
	else if (vCommand == "getDateTime")
	{
		int32_t nShift = vParams.get("shift", 0);
		auto now = std::chrono::system_clock::now() + std::chrono::milliseconds(nShift);
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()).count();
	}

	///
	/// @fn Variant RandomDataGenerator::execute()
	/// @fn Variant RandomDataGenerator::execute()
	///
	/// ##### select()
	/// Randomly returns one of elements of 'data' sequence.
	///   * data [seq] - a sequence for data selection.
	/// 
	else if (vCommand == "select")
	{
		if (!vParams.has("data"))
			error::InvalidArgument(SL, "Parameter <data> is not specified").throwException();

		Sequence seqTemplates;
		if (vParams["data"].isSequenceLike())
			seqTemplates = Sequence(vParams["data"]);
		else
			seqTemplates.push_back(vParams["data"]);
		
		Size idx = 0;
		if (seqTemplates.getSize() > 0)
			idx = m_fnRandGen() % seqTemplates.getSize();
		return seqTemplates[idx].clone();
	}

	///
	/// @fn Variant RandomDataGenerator::execute()
	///
	/// ##### generate()
	/// Randomly returns one of elements of 'data' sequence.
	///   * data [seq] - a sequence for data selection.
	/// 
	else if (vCommand == "generate")
	{
		if (!vParams.has("size"))
		{
			Size nBegin = vParams.get("begin", 0);
			Size nEnd = vParams.get("end", c_nMaxSize);
			return nBegin + m_fnRandGen() % (nEnd - nBegin);
		}
		Size nStrSize = vParams.get("size", 8);
		std::string sData;
		for (Size n = 0; n < nStrSize; ++n)
			sData.push_back('a' + m_fnRandGen() % ('z' - 'a'));
		return sData;
	}

	error::OperationNotSupported(SL,
		FMT("RandomDataGenerator doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace openEdr 
/// @}
