//
// edrav2.libsysmon project
//
// Pattern searcher declaration
//
// Author: Denis Kroshin (18.03.2019)
// Reviewer: Denis Bogdanov (20.03.2019)
//
#pragma once
#include "..\inc\objects.h"

namespace cmd {

CMD_DEFINE_ENUM_FLAG(EventType)
{
	None = 0x00,
	Read = 0x01,
	Write = 0x02,
};

///
/// Events Pattern Searcher
///
class PatternSeacher : public ObjectBase<CLSID_PatternSeacher>,
	public ICommandProcessor,
	public IDataReceiver
{
private:
	typedef std::string Hash;

	static const Size c_nPurgeMask = 0x3FF;
	static const Size c_nPurgeTimeout = 60 * 1000; // 60 sec

	struct Item
	{
		EventType eType = EventType::None;
		Time nTime = 0;
		Dictionary vRead;
		Sequence vWrite;
	};

	ObjPtr<IDataReceiver> m_pReceiver;

	std::mutex m_mtxSync;
	std::map<Hash, Item> m_pItems;
	Size m_nCounter = 0;
	Size m_nPurgeMask = c_nPurgeMask;
	Size m_nPurgeTimeout = c_nPurgeTimeout;

	inline Hash generateId(Variant vEvent);

	Sequence matchFileEvents(Item item);
	void purgeItems();

	std::wstring getRegistryPath(std::wstring sPath);
	std::wstring getRegistryAbstractPath(std::wstring sPath);

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object's configuration including the following fields:
	///   **receiver** - object that receive MLE events
	///   **purgeMask** - purge process start every (purgeMask+1) event
	///   **purgeTimeout** - time to keep events in before purge
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor
	virtual Variant execute(Variant vCommand, Variant vParams) override;

	// IDataReceiver
	virtual void put(const Variant& vData) override;

};
} // namespace cmd 
