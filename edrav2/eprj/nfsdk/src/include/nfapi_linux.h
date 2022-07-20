//
// 	NetFilterSDK 
// 	Copyright (C) Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#ifndef _NFEXTAPI
#define _NFEXTAPI

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define UNALIGNED
#define _MAX_PATH 260
#define MAX_PATH 260
#define __int64 long long
#define _snprintf snprintf
#define _vsnprintf vsnprintf

#include "nfevents.h"

#define NFAPI_API

#define NFEXT_MAX_IP_ADDRESS_LENGTH	16
#define NFEXT_MAX_PATH 256

#pragma pack(push,1)

// Port range [from:to] for rules. Port numbers must be in host format!
typedef struct _NFEXT_PORT_RANGE
{
	unsigned short from;
	unsigned short to;
} NFEXT_PORT_RANGE, *PNFEXT_PORT_RANGE;

typedef enum _NFEXT_FILTERING_FLAG
{
	NFEXT_BYPASS = 0,		// Bypass connection/packet
	NFEXT_REDIRECT = 1,		// Redirect connection
	NFEXT_BLOCK = 2,		// Block connection/packet
} NFEXT_FILTERING_FLAG;

typedef enum _NFEXT_RULE_FIELDS
{
	NFEXT_USE_DST_IP = 1,		// Use dstIp
	NFEXT_USE_DST_PORTS = 2,	// Use dstPorts
	NFEXT_USE_SRC_IP = 4,		// Use srcIp
	NFEXT_USE_SRC_PORTS = 8,	// Use srcPorts
	NFEXT_USE_UID = 16,			// Use uid
	NFEXT_USE_GID = 32,			// Use gid
} NFEXT_RULE_FIELDS;

typedef enum _NFEXT_RULE_TYPE
{
	NFEXT_NAT_RULE = 0,			// Redirection and access control on NAT level
	NFEXT_PACKET_RULE = 1,		// Access control on packet level
} NFEXT_RULE_TYPE;

typedef struct _NFEXT_RULE
{
	// See NFEXT_RULE_TYPE
	int				ruleType;	
	// Protocol (IPPROTO_TCP, IPPROTO_UDP)
	int				protocol;	
	// Direction (NF_D_IN, NF_D_OUT, NF_D_BOTH or zero)
	int				direction;	
	// AF_INET for IPv4 and AF_INET6 for IPv6
	unsigned short	ip_family;	
	// See NFEXT_RULE_FIELDS
	unsigned int	fieldsMask;	

	// Destination IP or network 
	char	dstIp[NFEXT_MAX_IP_ADDRESS_LENGTH];	
	// Destination IP mask
	unsigned char	dstIpMask; 
	// Destination ports
	NFEXT_PORT_RANGE	dstPorts;

	// Source IP or network 
	char	srcIp[NFEXT_MAX_IP_ADDRESS_LENGTH];	
	// Source IP mask
	unsigned char	srcIpMask; 
	// Source ports
	NFEXT_PORT_RANGE	srcPorts;

	// User id
	pid_t			uid;

	// Group id
	pid_t			gid;

	// See NFEXT_FILTERING_FLAG
	unsigned long	filteringFlag;
} NFEXT_RULE, *PNFEXT_RULE;

#pragma pack(pop)


#ifndef _C_API
	namespace nfapi
	{
		#define NFAPI_NS	nfapi::
		#define NFAPI_CC	
#else // _C_API
#ifdef WIN32
	#define NFAPI_CC __cdecl
#else
	#define NFAPI_CC 
#endif
	#define NFAPI_NS
	#ifdef __cplusplus
	namespace nfapi
	{
	}
	extern "C" 
	{
	#endif
#endif // _C_API

/**
* Initializes the internal data structures and starts the filtering thread.
* @param pHandler Pointer to event handling object
**/
NFAPI_API NF_STATUS NFAPI_CC nf_init(const char * driverName, NF_EventHandler * pHandler);

/**
* Stops the filtering thread, breaks all filtered connections and closes
* a connection with the hooking driver.
**/
NFAPI_API void NFAPI_CC 
nf_free();


//
// TCP control routines
//

/**
* Suspends or resumes indicating of sends and receives for specified connection.
* @param id Connection identifier
* @param suspended TRUE(1) for suspend, FALSE(0) for resume 
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_tcpSetConnectionState(ENDPOINT_ID id, int suspended);

/**
* Sends the buffer to remote server via specified connection.
* @param id Connection identifier
* @param buf Pointer to data buffer
* @param len Buffer length
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_tcpPostSend(ENDPOINT_ID id, const char * buf, int len);

/**
* Indicates the buffer to local process via specified connection.
* @param id Unique connection identifier
* @param buf Pointer to data buffer
* @param len Buffer length
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_tcpPostReceive(ENDPOINT_ID id, const char * buf, int len);

/**
* Breaks the connection with given id.
* @param id Connection identifier
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_tcpClose(ENDPOINT_ID id);

/**
 *	Sets the timeout for TCP connections and returns old timeout.
 *	@param timeout Timeout value in milliseconds. Specify zero value to disable timeouts.
 */
NFAPI_API unsigned long NFAPI_CC 
nf_setTCPTimeout(unsigned long timeout);

/**
 *	Disables indicating TCP packets to user mode for the specified endpoint
 *  @param id Socket identifier
 */
NFAPI_API NF_STATUS NFAPI_CC 
nf_tcpDisableFiltering(ENDPOINT_ID id);


//
// UDP control routines
//

/**
* Suspends or resumes indicating of sends and receives for specified socket.
* @param id Socket identifier
* @param suspended TRUE(1) for suspend, FALSE(0) for resume 
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_udpSetConnectionState(ENDPOINT_ID id, int suspended);

/**
* Sends the buffer to remote server via specified socket.
* @param id Socket identifier
* @param options UDP options
* @param remoteAddress Destination address
* @param buf Pointer to data buffer
* @param len Buffer length
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_udpPostSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options);

/**
* Indicates the buffer to local process via specified socket.
* @param id Unique connection identifier
* @param options UDP options
* @param remoteAddress Source address
* @param buf Pointer to data buffer
* @param len Buffer length
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_udpPostReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options);

/**
 *	Disables indicating UDP packets to user mode for the specified endpoint
 *  @param id Socket identifier
 */
NFAPI_API NF_STATUS NFAPI_CC 
nf_udpDisableFiltering(ENDPOINT_ID id);


//
// Filtering rules 
//

/**
* Replace the rules with the specified array.
* @param pRules Array of <tt>NFEXT_RULE</tt> structures
* @param count Number of items in array
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_setRules(PNFEXT_RULE pRules, int count);


/**
* Removes all rules
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_deleteRules();

/**
* Returns in pConnInfo the properties of TCP connection with specified id.
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_getTCPConnInfo(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);

/**
* Returns a port used by local proxy.
**/
NFAPI_API unsigned short NFAPI_CC 
nf_getProxyPort();

/**
* Returns owner of the specified connection as user id.
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_getUid(ENDPOINT_ID id, unsigned int * pUid);

/**
* Returns user name for user id.
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_getUserName(unsigned int uid, char * buf, int len);

/**
* Returns process name for process id.
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_getProcessName(unsigned int pid, char * buf, int len);

/**
* Raises the limit on the number of files to at least the specified value.
**/
NFAPI_API NF_STATUS NFAPI_CC
nf_requireFileLimit(int file_limit);

/**
* Returns number of active TCP connections.
**/
NFAPI_API unsigned long NFAPI_CC 
nf_getConnCount();

#ifdef __cplusplus
}
#endif



#endif