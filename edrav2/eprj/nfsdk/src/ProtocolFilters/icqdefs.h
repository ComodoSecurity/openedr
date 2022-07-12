//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#pragma once

namespace ProtocolFilters
{

#ifndef WIN32
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#define IsEqualGUID(rguid1, rguid2) (!memcmp(&rguid1, &rguid2, sizeof(GUID)))
#endif

#pragma pack(push, 1)

struct OSCAR_FLAP
{
	unsigned char	id;
	unsigned char	channel;
	unsigned short	seq;
	unsigned short	len;
	char	data[1];
};

enum eOSCAR_FLAP_CHANNEL
{
	FC_NEGOTIATION = 1,
	FC_SNAC = 2,
	FC_ERROR = 3,
	FC_CLOSE = 4,
	FC_KEEPALIVE = 5
};

struct OSCAR_SNAC
{
	unsigned short familyId;
	unsigned short subtypeId;
	unsigned short flags;
	unsigned int requestId;
	char	data[1];
};

struct OSCAR_TLV
{
	unsigned short type;
	unsigned short len;
	char	data[1];
};

struct OSCAR_CHAT_MESSAGE_HDR
{
	unsigned __int64 msgId;
	unsigned short msgChannel;
};

struct OSCAR_CHAT_MESSAGE_TEXT_HDR
{
	unsigned short charsetId;
	unsigned short langId;
};

struct OSCAR_CHAT_IN_MESSAGE_HDR
{
	unsigned short warningLevel;
	unsigned short nTlvs;
};

struct OSCAR_CHAT_MESSAGE_CH2_HDR
{
	unsigned short msgType;
	unsigned __int64 msgId;
	GUID capabilityGuid;
};

struct OSCAR_CHAT_MESSAGE_CH2_EXT_HDR
{
	unsigned short len;
	unsigned short protocolVer;
	unsigned short pluginGuid[8];
};

struct OSCAR_CHAT_MESSAGE_CH2_FORMAT
{
	unsigned int	textColor;
	unsigned int	bgColor;
	unsigned int	guidLen;
	char	szGuid[1];
};

#define SZ_GUID_AIM_SUPPORTS_UTF8 "{0946134E-4C7F-11D1-8222-444553540000}"

struct OSCAR_CHAT_MESSAGE_CH2_TEXT
{
	unsigned char msgType;
	unsigned char msgFlags;
	unsigned short msgStatus;
	unsigned short msgPriority;
	unsigned short len;
};

struct OSCAR_CHAT_MESSAGE_CH4_HDR
{
	unsigned int senderUin;
	unsigned char msgType;
	unsigned char msgFlags;
	unsigned short len;
};

struct OSCAR_FILE_TRANSFER_HDR
{
	unsigned short multFlag;
	unsigned short fileCount;
	unsigned int totalBytes;
	char fileName[1];
};

#pragma pack(pop)

}