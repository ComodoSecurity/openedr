//
// 	NetFilterSDK 
// 	Copyright (C) Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#pragma once

#pragma pack(push, 1)

enum eSOCKS_VERSION
{
	SOCKS_4 = 4,
	SOCKS_5 = 5
};

enum eSOCKS4_COMMAND
{
	S4C_CONNECT = 1,
	S4C_BIND = 2
};

struct SOCKS4_REQUEST
{
	char	version;
	char	command;
	unsigned short port;
	unsigned int ip;
	char userid[1];
};

enum eSOCKS4_RESCODE
{
	S4RC_SUCCESS = 0x5a
};

struct SOCKS4_RESPONSE
{
	char	reserved;
	char	res_code;
	unsigned short port;
	unsigned int ip;
};

enum eSOCKS5_COMMAND
{
	S5C_AUTH = -1,
	S5C_CONNECT = 1,
	S5C_BIND = 2,
	S5C_UDP_ASSOCIATE = 3
};

struct SOCKS5_AUTH_REQUEST
{
	char	version;
	unsigned char	nmethods;
	unsigned char	methods[1];
};

enum eSOCKS5_AUTH_METHOD
{
	S5AM_NONE = 0,
	S5AM_GSSAPI = 1,
	S5AM_UNPW = 2
};

struct SOCK5_AUTH_RESPONSE
{
	char	version;
	unsigned char	method;
};

enum eSOCKS5_ADDRESS_TYPE
{
	SOCKS5_ADDR_IPV4 = 1,
	SOCKS5_ADDR_DOMAIN = 3,
	SOCKS5_ADDR_IPV6 = 4
};

struct SOCKS5_REQUEST
{
	char	version;
	char	command;
	char	reserved;
	char	address_type;
	char	address[1];
};

struct SOCKS5_REQUEST_IPv4
{
	char	version;
	char	command;
	char	reserved;
	char	address_type;
	unsigned int address;
	unsigned short port;
};

struct SOCKS5_REQUEST_IPv6
{
	char	version;
	char	command;
	char	reserved;
	char	address_type;
	char	address[16];
	unsigned short port;
};

struct SOCKS5_RESPONSE
{
	char	version;
	char	res_code;
	char	reserved;
	char	address_type;
	char	address[1];
};

struct SOCKS5_RESPONSE_IPv4
{
	char	version;
	char	res_code;
	char	reserved;
	char	address_type;
	unsigned int address;
	unsigned short port;
};

struct SOCKS5_RESPONSE_IPv6
{
	char	version;
	char	res_code;
	char	reserved;
	char	address_type;
	char	address[16];
	unsigned short port;
};

struct SOCKS5_UDP_REQUEST
{
	unsigned short reserved;
	char	frag;
	char	address_type;
};

struct SOCKS5_UDP_REQUEST_IPv4
{
	unsigned short reserved;
	char	frag;
	char	address_type;
	unsigned int address;
	unsigned short port;
};

struct SOCKS5_UDP_REQUEST_IPv6
{
	unsigned short reserved;
	char	frag;
	char	address_type;
	char	address[16];
	unsigned short port;
};

union SOCKS45_REQUEST
{
	SOCKS4_REQUEST	socks4_req;
	SOCKS5_AUTH_REQUEST	socks5_auth_req;
	SOCKS5_REQUEST	socks5_req;
};

#pragma pack(pop)

