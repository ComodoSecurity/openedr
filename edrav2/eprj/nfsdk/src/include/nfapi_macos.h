//
// 	NetFilterSDK 
// 	Copyright (C) Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#ifndef _NFAPI_MACOS
#define _NFAPI_MACOS

#include "nfext_macos.h"

#define UNALIGNED
#define _MAX_PATH 260
#define MAX_PATH 260
#define __int64 long long
#define _snprintf snprintf
#define _vsnprintf vsnprintf

#include "nfevents.h"

#define NFAPI_API



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
* @param driverName The name of TDI hooking driver, without ".sys" extension.
* @param pHandler Pointer to event handling object
**/
NFAPI_API NF_STATUS NFAPI_CC nf_init(const char * driverName, NF_EventHandler * pHandler);

/**
* Stops the filtering thread, breaks all filtered connections and closes
* a connection with the hooking driver.
**/
NFAPI_API void NFAPI_CC 
nf_free();

/**
* Registers and starts a driver with specified name (without ".sys" extension)
* @param driverName 
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_registerDriver(const char * driverName);

/**
* Unregisters a driver with specified name (without ".sys" extension)
* @param driverName 
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_unRegisterDriver(const char * driverName);


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
* Add a rule to the head of rules list in driver.
* @param pRule See <tt>NFEXT_RULE</tt>
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_addRule(PNFEXT_RULE pRule);

NFAPI_API NF_STATUS NFAPI_CC 
nf_addUdpRule(PNFEXT_RULE pRule);

/**
* Removes all rules from driver.
**/
NFAPI_API NF_STATUS NFAPI_CC 
nf_deleteRules();

/**
*	Detaches from filtered TCP/UDP sockets
**/
NFAPI_API NF_STATUS NFAPI_CC
nf_disableFiltering();

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