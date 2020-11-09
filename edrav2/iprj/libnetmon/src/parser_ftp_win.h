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
/// Ftp commands
///
enum class FtpCmd : uint32_t
{
	Invalid = 0,
	PWD =  'PWD ',
	XPWD = 'XPWD',
	CDUP = 'CDUP',
	XCUP = 'XCUP',
	CWD =  'CWD ',
	REIN = 'REIN',
	XCWD = 'XCWD',
	RETR = 'RETR',
};

///
/// Ftp request info
///
struct FtpRequest
{
	FtpCmd eCmd = FtpCmd::Invalid;
	std::string sArgs;
};

///
/// Ftp response
///
struct FtpResponse
{
	size_t nCode;
	std::string sArgs;
};


class FtpPath
{
private:
	std::vector<std::string> m_parts;
	bool m_fHasRoot = false;

	enum class PartType
	{
		Common, // non-special
		Empty, // ""
		Dot, // "."
		DotDot, // ".."
	};


	static PartType detectPartType(const std::string& sPart)
	{
		switch (sPart.size())
		{
		case 2:
			if (sPart[0] == '.' && sPart[1] == '.')
				return PartType::DotDot;
			break;
		case 1:
			if (sPart[0] == '.')
				return PartType::Dot;
			break;
		case 0:
			return PartType::Empty;
			break;
		}
		return PartType::Common;
	}

	static bool isNotCommonPart(const std::string& sPart)
	{
		return detectPartType(sPart) != PartType::Common;
	}


	//
	// Normalizes path with root only
	//
	// Removes '..', '.'
	// Leaves path as is if error was found
	//
	void normalize()
	{
		// Skip paths without root
		if (!m_fHasRoot)
			return;

		if (m_parts.empty())
			return;

		// Test for existence special parts and that backup is required
		bool fNeedBackup = false;
		if (isNotCommonPart(m_parts[0]))
		{
			fNeedBackup = true;
		}
		else
		{
			auto nSpecialPartCount = std::count_if(m_parts.begin(), m_parts.end(), isNotCommonPart);
			// Test no need normalization
			if (nSpecialPartCount == 0)
				return;
			if(nSpecialPartCount > 1)
				fNeedBackup = true;

		}

		// Create backup;
		std::vector<std::string> backup;
		if (fNeedBackup)
			backup = m_parts;

		// Normalization
		for (size_t i = 0; i < m_parts.size();)
		{
			PartType eType = detectPartType(m_parts[i]);

			// Process part
			switch (eType)
			{
			case PartType::Empty:
			case PartType::Dot:
				// Remove this
				m_parts.erase(m_parts.begin() + i);
				break;
			case PartType::DotDot:
				// Remove previous and this
				if (i == 0)
				{
					// restore backup
					if (!backup.empty())
						m_parts = backup;
					return;
				}
				// Remove this
				m_parts.erase(m_parts.begin() + i);
				// Remove previous
				m_parts.erase(m_parts.begin() + i - 1);
				--i;
				break;
			default:
				++i;
				break;
			}
		}
	}

public:

	// Default constructor, move and copy
	FtpPath() = default;
	~FtpPath() = default;
	FtpPath(const FtpPath&) = default;
	FtpPath(FtpPath&&) = default;
	FtpPath& operator=(const FtpPath&) = default;
	FtpPath& operator=(FtpPath&&) = default;


	//
	// Constructor from string
	//
	explicit FtpPath(std::string_view sPath)
	{
		if (sPath.empty())
			return;

		m_fHasRoot = sPath[0] == '/';

		std::string_view sPathTail = sPath;
		while (!sPathTail.empty())
		{
			std::string_view sPart;

			// Cut a part
			auto nSlashPos = sPathTail.find_first_of('/');
			if (nSlashPos == std::string_view::npos)
			{
				sPart = sPathTail;
				sPathTail = std::string_view();
			}
			else
			{
				sPart = sPathTail.substr(0, nSlashPos);
				sPathTail = sPathTail.substr(nSlashPos+1);
			}

			// Append part
			if(!sPart.empty())
				m_parts.push_back(std::string(sPart));
		}

		normalize();
	}

	//
	// Path is from root
	//
	bool hasRoot() const
	{
		return m_fHasRoot;
	}

	FtpPath& append(FtpPath&& path)
	{
		if (path.m_fHasRoot)
		{
			*this = std::move(path);
			return *this;
		}

		m_parts.reserve(m_parts.size() + path.m_parts.size());
		m_parts.insert(m_parts.end(),
			std::make_move_iterator(path.m_parts.begin()),
			std::make_move_iterator(path.m_parts.end())
		);

		normalize();
		return *this;
	}

	FtpPath& append(const FtpPath& path)
	{
		FtpPath copy(path);
		return append(std::move(copy));
	}

	std::string getStr() const
	{
		if (m_parts.empty())
			return (m_fHasRoot ? "/" : "");

		size_t nReservedSize = 0;
		for (auto& part : m_parts)
			nReservedSize += part.size() + 1;
		std::string sVal;
		sVal.reserve(nReservedSize);

		bool fAddSlash = m_fHasRoot;
		for (auto& part : m_parts)
		{
			if (fAddSlash)
				sVal += "/";
			sVal += part;
			fAddSlash = true;
		}

		return sVal;
	}
};




///
/// FTP parser.
///
class FtpParser : public ObjectBase<>,
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

	FtpPath m_curPath;

	FtpRequest m_nLastRequest;
	void processFtpControlData(std::shared_ptr<const ConnectionInfo> pConInfo, std::string_view sData, bool fRequest);

protected:
	FtpParser() : m_curPath("/") {}
	~FtpParser() override {}
public:

	// Inherited via ITcpParser
	DataStatus dataAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) override;
	DataPartStatus dataPartAvailable(std::shared_ptr<const ConnectionInfo> pConInfo, ProtocolFilters::PFObject* pObject) override;

	static ObjPtr<FtpParser> create(ObjPtr<INetworkMonitorController> pNetMonController);
};


///
/// FTP parser factory.
///
class FtpParserFactory : public ObjectBase<CLSID_FtpParserFactory>,
	public ITcpParserFactory
{
private:
	ObjPtr<INetworkMonitorController> m_pNetMonController;  //< Callback to NetworkMonitorController

protected:
	FtpParserFactory() {}
	~FtpParserFactory() override {}
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
