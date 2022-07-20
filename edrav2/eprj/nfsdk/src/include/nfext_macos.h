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

#define NFEXT_BUNDLEID	"nfext"

#define NFEXT_MAX_IP_ADDRESS_LENGTH	16
#define NFEXT_MAX_PATH 256

enum NFEXT_REQUEST
{
	NFEXT_ADD_RULE = 1,
	NFEXT_DELETE_RULES = 2,
	NFEXT_ADD_UDP_RULE = 3,
};

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
	NFEXT_USE_REMOTE_IP = 1,		// Use remoteIp
	NFEXT_USE_REMOTE_IP_MASK = 2,	// Use remoteIpMask
	NFEXT_USE_REMOTE_PORTS = 4,		// Use remotePorts
	NFEXT_USE_PID = 8,				// Use pid
	NFEXT_USE_UID = 16,				// Use uid
	NFEXT_USE_PROCESS_NAME = 32,	// Use processName
} NFEXT_RULE_FIELDS;

typedef struct _NFEXT_RULE
{
	// See NFEXT_RULE_FIELDS
	unsigned int	fieldsMask;	
	// AF_INET for IPv4 and AF_INET6 for IPv6
	unsigned short	ip_family;	
	// Remote IP or network 
	char	remoteIp[NFEXT_MAX_IP_ADDRESS_LENGTH];	
	// Remote IP mask
	char	remoteIpMask[NFEXT_MAX_IP_ADDRESS_LENGTH]; 
	// Remote ports
	NFEXT_PORT_RANGE	remotePorts;

	// Where to redirect the connection
	union 
	{
		struct sockaddr_in	addr4;		// ipv4 remote addr
		struct sockaddr_in6	addr6;		// ipv6 remote addr
	} redirectTo;
	
	// Process id
	pid_t			pid;
	// User id
	pid_t			uid;
	// Process name mask
	char			processName[NFEXT_MAX_PATH];
	// See NFEXT_FILTERING_FLAG
	unsigned long	filteringFlag;
} NFEXT_RULE, *PNFEXT_RULE;

#define NFEXT_INFO_MAGIC	0xEEBBCCDD

typedef struct _NFEXT_INFO
{
	int		magic;		// Must be NFEXT_INFO_MAGIC
	// Original remote address of redirected connection
	union
	{
		struct sockaddr_in	addr4;		// ipv4 remote addr
		struct sockaddr_in6	addr6;		// ipv6 remote addr
	} remote_addr;
	pid_t			pid;				// pid that created the socket 
	pid_t			uid;				// used id that created the socket
} NFEXT_INFO, *PNFEXT_INFO;

#pragma pack(pop)

#endif