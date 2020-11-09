//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file Connection objects
///
/// @addtogroup netmon Network Monitor library
/// @{
#pragma once
#include "objects.h"
#include "netmon_win.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "libnetmon"

namespace openEdr {
namespace netmon {
namespace win {

///
/// TCP Connection implementation.
///
class TcpConnection : public ObjectBase<>,
	public IConnection
{
	std::shared_ptr<const ConnectionInfo> m_pInfo;

	std::mutex m_mtxParsing;
	std::list<ObjPtr<ITcpParserFactory>> m_parserFactoryList;
	std::list<ObjPtr<ITcpParser>> m_parserList; //< List of parser-candidates
	ObjPtr<ITcpParser> m_pMainParser; //< Main (unique) parser (optimization)

	// Create parsers
	// Should be called with locked m_mtxParsing
	void ProcessParserFactories(ProtocolFilters::PFObject* pObject);

protected:
	TcpConnection();
	~TcpConnection() override;
public:

	// IConnection interface
	std::shared_ptr<const ConnectionInfo> getInfo() override;
	void addParserFactory(ObjPtr<IObject> pParserFactory) override;
	virtual bool hasParsers() override;

	///
	/// Callback on dataAvailable().
	///
	/// @param pObject - a pointer to the object.
	///
	void dataAvailable(ProtocolFilters::PFObject* pObject);

	///
	/// Callback on dataPartAvailable().
	///
	/// @param pObject - a pointer to the object.
	/// @returns The function returns the data part check result.
	///
	ProtocolFilters::PF_DATA_PART_CHECK_RESULT dataPartAvailable(ProtocolFilters::PFObject* pObject);

	///
	/// Creator.
	///
	/// @param pInfo - a pointer to connection info.
	/// @returns The function returns a smart pointer to created object.
	///
	static ObjPtr<TcpConnection> create(std::shared_ptr<const ConnectionInfo> pInfo);
};


///
/// UDP Connection implementation.
///
class UdpConnection : public ObjectBase<>,
	public IConnection
{
private:
	std::shared_ptr<const ConnectionInfo> m_pInfo;

	std::mutex m_mtxParsing;
	std::list<ObjPtr<IUdpParser>> m_parserList; //< List of parser-candidates
	ObjPtr<IUdpParser> m_pMainParser; //< Main (unique) parser (optimization)

protected:
	UdpConnection();
	~UdpConnection() override;
public:

	// IConnection interface
	std::shared_ptr<const ConnectionInfo> getInfo() override;
	void addParserFactory(ObjPtr<IObject> pParserFactory) override;
	bool hasParsers() override;

	///
	/// Callback on data send/receive.
	///
	/// @param fSend - if true it is send, else it is receive.
	/// @param pBuf - pointer to data buffer.
	/// @param nLen - size of data buffer.
	///
	void notifyData(bool fSend, const uint8_t* pBuf, size_t nLen);

	///
	/// Callback on close.
	///
	void notifyClose();

	///
	/// Creator.
	///
	/// @param pInfo - a pointer to connection info.
	/// @returns The function returns a smart pointer to created object.
	///
	static ObjPtr<UdpConnection> create(std::shared_ptr<const ConnectionInfo> pInfo);
};

} // namespace win
} // namespace netmon
} // namespace openEdr

/// @}
