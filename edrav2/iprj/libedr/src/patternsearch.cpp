//
// edrav2.libsysmon project
//
// Pattern searcher implementation
//
// Author: Denis Kroshin (18.03.2019)
// Reviewer: Denis Bogdanov (20.03.2019)
//
#include "pch.h"
#include "patternsearch.h"

namespace cmd {

#undef CMD_COMPONENT
#define CMD_COMPONENT "pttrnsch"

//
//
//
void PatternSeacher::finalConstruct(Variant vConfig)
{
	m_pReceiver = queryInterface<IDataReceiver>(vConfig.get("receiver"));
	m_nPurgeMask = vConfig.get("purgeMask", c_nPurgeMask);
	m_nPurgeTimeout = vConfig.get("purgeTimeout", c_nPurgeTimeout);
}

//
//
//
PatternSeacher::Hash PatternSeacher::generateId(Variant vEvent)
{
	TRACE_BEGIN
	std::string nHash = getByPath(vEvent, "file.rawHash");
	sys::win::ProcId sProcId = getByPath(vEvent, "process.id");

	crypt::sha1::Context ctx;
	crypt::sha1::init(&ctx);
	crypt::sha1::update(&ctx, nHash.data(), nHash.size());
	crypt::sha1::update(&ctx, sProcId.data(), sProcId.size());
	crypt::sha1::Hash hash = crypt::sha1::finalize(&ctx);
	return string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
	TRACE_END("Fail to generate id")
}

//
//
//
Sequence PatternSeacher::matchFileEvents(Item item)
{
	TRACE_BEGIN
	Sequence vOut;
	Variant vSrc = item.vRead;
	for (auto vDst : item.vWrite)
	{
		auto sSrcType = getByPath(vSrc, "file.volume.type", "FIXED");
		auto sDstType = getByPath(vDst, "file.volume.type", "FIXED");
		Event eEvent = Event::LLE_NONE;
		if (sSrcType == "FIXED" && sDstType == "REMOVABLE")
			eEvent = Event::MLE_FILE_COPY_TO_USB;
		else if (sSrcType == "FIXED" && sDstType == "NETWORK")
			eEvent = Event::MLE_FILE_COPY_TO_SHARE;
		else if (sSrcType == "REMOVABLE" && sDstType == "FIXED")
			eEvent = Event::MLE_FILE_COPY_FROM_USB;
		else if (sSrcType == "NETWORK" && sDstType == "FIXED")
			eEvent = Event::MLE_FILE_COPY_FROM_SHARE;
		else
			continue;

		Dictionary vEvent;
		vEvent.put("type", eEvent);
		vEvent.put("time", vDst.get("time"));
		vEvent.put("tickTime", vDst.get("tickTime"));
		Variant vProc = vSrc.get("process");
		vEvent.put("process", vProc);
		Variant vSecurity = vProc.get("security", {});

		Variant vFile = variant::createLambdaProxy([vSecurity, vDst]() -> Variant
		{
			auto pFileInformation = queryInterface<sys::win::IFileInformation>(queryService("fileDataProvider"));
			Variant vFile = vDst.get("file");
			if(!vFile.has("security"))
				vFile.put("security", vSecurity);
			return pFileInformation->getFileInfo(vFile);
		}, true);

		Variant vFileOld = variant::createLambdaProxy([vSecurity, vSrc]() -> Variant
		{
			auto pFileInformation = queryInterface<sys::win::IFileInformation>(queryService("fileDataProvider"));
			Variant vFile = vSrc.get("file");
			if (!vFile.has("security"))
				vFile.put("security", vSecurity);
			return pFileInformation->getFileInfo(vFile);
		}, true);

		vEvent.put("file", variant::createLambdaProxy([vFile, vFileOld]() {
			Variant vRes = vFile;
			vRes.put("old", vFileOld);
			return vRes;
		}, true));

		vOut.push_back(vEvent);
	}

	// Clear write events
	item.vWrite.clear();
	resetFlag(item.eType, EventType::Write);
	return vOut;
	TRACE_END("Fail to match file events")
}

//
//
//
void PatternSeacher::purgeItems()
{
	std::scoped_lock _lock(m_mtxSync);
	LOGLVL(Debug, "Purge start <PatternSeacher>. Size <" << m_pItems.size() << ">");
	uint64_t nCurTime = sys::win::getCurrentTime();
	for (auto itItem = m_pItems.begin(); itItem != m_pItems.end();)
	{
		// Add process deletion monitoring if needed
		uint64_t nDelTime = itItem->second.nTime;
		if (nCurTime > nDelTime && nCurTime - nDelTime >= m_nPurgeTimeout)
		{
			itItem = m_pItems.erase(itItem);
		}
		else
			++itItem;
	}
	LOGLVL(Debug, "Purge finished <PatternSeacher>. Size <" << m_pItems.size() << ">");
}


//
//
//
void PatternSeacher::put(const Variant& vEvent)
{
	TRACE_BEGIN;
	Event eType = vEvent.get("type");
	if (eType != Event::LLE_FILE_DATA_READ_FULL &&
		eType != Event::LLE_FILE_DATA_WRITE_FULL)
		return m_pReceiver->put(vEvent);

	Sequence vEvents;
	{
		std::scoped_lock _lock(m_mtxSync);
		Item& sItem = m_pItems[generateId(vEvent)];
		sItem.nTime = std::max<Time>(sItem.nTime, vEvent.get("time"));
		if (eType == Event::LLE_FILE_DATA_READ_FULL)
		{
			setFlag(sItem.eType, EventType::Read);
			sItem.vRead = vEvent;
		}
		else if (eType == Event::LLE_FILE_DATA_WRITE_FULL)
		{
			setFlag(sItem.eType, EventType::Write);
			sItem.vWrite.push_back(vEvent);
		}
		if (testMask(sItem.eType, EventType::Read | EventType::Write))
			vEvents = matchFileEvents(sItem);
	}

	for (auto event : vEvents)
		m_pReceiver->put(event);

	// Purge items in async thread
	if (((++m_nCounter) & m_nPurgeMask) == 0)
		run(&PatternSeacher::purgeItems, getPtrFromThis(this));

	TRACE_END(FMT("Fail to parse event <" << vEvent.get("type", 0) << ">"))
}

//
//
//
Variant PatternSeacher::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant PatternSeacher::execute()
	///
	/// ##### put()
	/// Put data to the pattern searcher
	///   * data [var] - data;
	///
	if (vCommand == "put")
	{
		put(vParams["data"]);
		return {};
	}

	error::OperationNotSupported(SL,
		FMT("PatternSeacher doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace cmd 
