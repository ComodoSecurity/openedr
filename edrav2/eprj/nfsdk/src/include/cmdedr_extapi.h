//
// Comodo EDR extension of NetFilter SDK
//
// External API for nfapi.lib and for wfp driver
//

#pragma once

// use CMDEDR_EXTAPI_DRIVER macros to disable declaration not for driver 

namespace nfapi {
namespace cmdedr {

//
// Data type for Sending data to user mode API
//
enum class CustomDataType : UINT16
{
	ListenInfo,
};

struct ListenInfo
{
	unsigned char localAddr[NF_MAX_ADDRESS_LENGTH];
	unsigned short ip_family; // AF_INET for IPv4 and AF_INET6 for IPv6
	UINT32 nPid;
};


//
// CustomEventHandler
//
class CustomEventHandler
{
public:

	//
	// Listen is called
	//
	virtual void tcpListened(const ListenInfo* pListenInfo) = 0;
};


//
// Set CustomEventHandler
// It should be called before nt_init()
//
void setCustomEventHandler(CustomEventHandler* pCustomEventHandler);

} // namespace cmdedr
} // namespace nfapi




