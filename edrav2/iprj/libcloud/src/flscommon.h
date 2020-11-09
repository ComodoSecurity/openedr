//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (19.03.2019)
// Reviewer: Denis Bogdanov (25.03.2019)
//
///
/// @file File Lookup Service (FLS) objects declaration
///
#pragma once
#include <fls.hpp>

namespace openEdr {
namespace cloud {
namespace fls {

constexpr Size c_nGuidSize = 16;
constexpr Size c_nHashSize = 20;
	
using HashList = std::vector<Hash>;
using FileVerdictMap = std::map<Hash, FileVerdict>;

//
//
//
class Client
{
protected:
	std::string m_sHost;
	Size m_nPort;

public:

	//
	// Constructor.
	//
	Client(std::string_view sHost, Size nPort);

	//
	// Destructor.
	//
	virtual ~Client();

	//
	// Gets a verdict of the file(s).
	//
	virtual FileVerdictMap getFileVerdict(const HashList& hashList) = 0;

	//
	// Get verdict of the vendor(s)
	//
	virtual FileVerdictMap getVendorVerdict(const HashList& hashList) = 0;
};

} // namespace fls
} // namespace cloud
} // namespace openEdr
