//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file Http parser
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
/// Http parser.
///
class HttpParser : public ObjectBase<>,
	public ITcpParser
{
private:
	ObjPtr<INetworkMonitorController> m_pNetMonController;  //< Callback to NetworkMonitorController

	//
	// Send event
	//
	// @param eRawEvent - raw event id
	// @param pConInfo - ConnectionInfo
	// @param fnAddData - function to add all additional event data
	//        Signature: bool fnAddData(Dictionary& vEvent);
	//        if fnAddData() returns false, this function returns null.
	//        fnAddData() should add "connection" field.
	// 
	template<typename FnAddData>
	void sendEvent(RawEvent eRawEvent, std::shared_ptr<const ConnectionInfo> pConInfo, FnAddData fnAddData);

protected:
	HttpParser() {}
	~HttpParser() override {}
public:

	// ITcpParser

	/// @copydoc ITcpParser::dataAvailable()
	DataStatus dataAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) override;
	/// @copydoc ITcpParser::dataPartAvailable()
	DataPartStatus dataPartAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) override;

	static ObjPtr<HttpParser> create(ObjPtr<INetworkMonitorController> pNetMonController);
};


///
/// Http parser factory.
///
class HttpParserFactory : public ObjectBase<CLSID_HttpParserFactory>,
	public ITcpParserFactory
{
private:
	ObjPtr<INetworkMonitorController> m_pNetMonController;  //< Callback to NetworkMonitorController

protected:
	HttpParserFactory() {}
	~HttpParserFactory() override {}
public:
	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - object's configuration including the following fields:
	/// * controller - callback to controller.
	///
	void finalConstruct(Variant vConfig);

	// ITcpParserFactory

	/// @copydoc ITcpParserFactory::tuneConnection()
	bool tuneConnection(std::shared_ptr<const ConnectionInfo>) override;
	/// @copydoc ITcpParserFactory::getName()
	std::string_view getName() override;
	/// @copydoc ITcpParserFactory::createParser()
	virtual ITcpParserFactory::ParserInfo createParser(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) override;
};

} // namespace win
} // namespace netmon
} // namespace openEdr

/// @}
