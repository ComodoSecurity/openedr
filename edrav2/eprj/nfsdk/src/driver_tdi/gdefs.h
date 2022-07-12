//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _GDEFS_H
#define _GDEFS_H

#define MAX_PATH 256

// Standard device names
#define NF_TCP_SYSTEM_DEVICE	L"\\Device\\Tcp"
#define NF_TCP6_SYSTEM_DEVICE	L"\\Device\\Tcp6"
#define NF_UDP_SYSTEM_DEVICE	L"\\Device\\Udp"
#define NF_UDP6_SYSTEM_DEVICE	L"\\Device\\Udp6"

#define TDI_ADDRESS_MAX_LENGTH	TDI_ADDRESS_LENGTH_OSI_TSAP
#define	TA_ADDRESS_MAX		(sizeof(TA_ADDRESS) - 1 + TDI_ADDRESS_MAX_LENGTH)
#define TDI_ADDRESS_INFO_MAX	(sizeof(TDI_ADDRESS_INFO) - 1 + TDI_ADDRESS_MAX_LENGTH)

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned long   u_long;
typedef unsigned int    u_int;

#pragma pack(push, 1)

typedef struct in_addr 
{  
	union 
	{
		struct { u_char s_b1, s_b2, s_b3, s_b4; } S_un_b;
		struct { u_short s_w1,s_w2; } S_un_w;
		u_long S_addr;  
	} S_un;
};

struct sockaddr_in 
{
	u_short	sin_family;
	u_short	sin_port;
	struct in_addr sin_addr;
	char sin_zero[8];
};

struct in6_addr 
{
    union {
        u_char Byte[16];
        u_short Word[8];
    } u;
};

struct sockaddr_in6 
{
    short   sin6_family;        /* AF_INET6 */
    u_short sin6_port;          /* Transport level port number */
    u_long  sin6_flowinfo;      /* IPv6 flow information */
    struct in6_addr sin6_addr;  /* IPv6 address */
    u_long sin6_scope_id;       /* set of interfaces for a scope */
};

typedef UNALIGNED union _sockaddr_gen 
{
    struct sockaddr_in  AddressIn;
    struct sockaddr_in6 AddressIn6;
} sockaddr_gen;

typedef UNALIGNED union _gen_addr
{
    struct in_addr sin_addr;
    struct in6_addr sin6_addr;
} gen_addr;

#pragma pack(pop)

#define TDI_SNDBUF	32 * 1024

#define ntohs(_x_) ((((unsigned char*)&_x_)[0] << 8) & 0xFF00) | ((unsigned char*)&_x_)[1] 

#endif // _GDEFS_H