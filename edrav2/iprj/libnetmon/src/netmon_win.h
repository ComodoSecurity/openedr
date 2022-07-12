//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file NetFilter-depended interfaces 
///
/// @addtogroup netmon Network Monitor library
/// @{
#pragma once

#include "netmon.h"

namespace cmd {
namespace netmon {
namespace win {

///
/// Interface of TCP Parser.
///
class ITcpParser : OBJECT_INTERFACE
{
public:
	//
	// Callback for ProtocolFilters::PFEvents::dataAvailable();
	//
	virtual DataStatus dataAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) = 0;

	//
	// Callback for ProtocolFilters::PFEvents::dataPartAvailable();
	//
	virtual DataPartStatus dataPartAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) = 0;
};


///
/// Interface of TCP Parser Factory.
///
class ITcpParserFactory : OBJECT_INTERFACE
{
public:
	///
	/// Tunes the connection for parsing.
	///
	/// @param pConInfo - a shared pointer to the connection info.
	/// @return The function returns `false` if ParserFactory will not process Connection.
	///
	virtual bool tuneConnection(std::shared_ptr<const ConnectionInfo> pConInfo) = 0;

	///
	/// Returns name of Parser.
	///
	/// Returned string has same lifetime as object.
	///
	/// @returns The function returns the name of the parser.
	///
	virtual std::string_view getName() = 0;

	///
	/// Result of createParser().
	///
	struct ParserInfo
	{
		enum class Status
		{
			Created,      ///< Parser is created. Add it and remove the factory
			Ignored,      ///< Data is ignored. Call the factory with next factory
			Incompatible, ///< Data is incompatible. Remove the factory.
		};

		Status eStatus;
		ObjPtr<ITcpParser> pParser;
	};

	///
	/// Creates a parser for process Connection and object.
	///
	/// @param pConInfo - a shared pointer to the connection info.
	/// @param pObject - a pointer to the ProtocolFilters object.
	/// @returns The function returns a ParserInfo struct.
	///
	virtual ITcpParserFactory::ParserInfo createParser(std::shared_ptr<const ConnectionInfo> pConInfo, 
		ProtocolFilters::PFObject* pObject) = 0;
};


//////////////////////////////////////////////////////////////////////////
//
// Utils
//

//
// It checks eObjectType is a type of specified filter (FilterType)
//
inline bool checkFilterType(ProtocolFilters::PF_FilterType eFilterType, ProtocolFilters::tPF_ObjectType eObjectType)
{
	size_t nFilterType = (size_t)eFilterType;
	size_t nObjectType = (size_t)eObjectType;
	if (nObjectType < nFilterType)
		return false;
	if(nObjectType - nFilterType >= FT_STEP)
		return false;
	return true;
}

///
/// Converts the object stream into string.
///
/// @param pObject - object which owns stream.
/// @param nStreamIndex - stream index.
/// @param fTrim - trim whitespaces.
/// @return The function returns `std::optional` value with a text string 
///		or `std::nullopt` if stream is not exist.
///
inline std::optional<std::string> convertStreamToString(ProtocolFilters::PFObject* pObject, int nStreamIndex,
	bool fTrim)
{
	if(pObject == nullptr)
		return std::nullopt;

	ProtocolFilters::PFStream* pStream = pObject->getStream(nStreamIndex);
	if (!pStream)
		return std::nullopt;

	std::string sData;
	sData.resize((size_t)pStream->size());
	pStream->seek(0, FILE_BEGIN);
	auto nReadSize = pStream->read((char*)sData.data(), (ProtocolFilters::tStreamSize)sData.size());
	sData.resize((size_t)nReadSize);

	if (!fTrim)
		return sData;

	// Remove first whitespace
	auto nStartPos = sData.find_first_not_of("\r\n\t ");
	if (nStartPos == std::string::npos)
		return std::string();

	// Remove last whitespace
	auto nEndPos = sData.find_last_not_of("\r\n\t ");
	if (nEndPos == std::string::npos)
		return std::string();

	if(nEndPos < nStartPos)
		return std::string();

	if (nStartPos == 0)
	{
		sData.resize(nEndPos+1);
		return sData;
	}

	return sData.substr(nStartPos, nEndPos - nStartPos + 1);
}

} // namespace win
} // namespace netmon
} // namespace cmd

/// @}
