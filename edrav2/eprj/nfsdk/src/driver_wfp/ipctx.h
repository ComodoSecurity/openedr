//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _IPCTX_H
#define _IPCTX_H

#include "hashtable.h"
#include "nfdriver.h"

typedef struct _IPCTX
{
	LIST_ENTRY	pendedPackets;	// List of NF_IP_PACKET

	ULONG		pendedSendBytes;	// Number of bytes in outbound packets from pendedSendPackets
	ULONG		pendedRecvBytes;	// Number of bytes in inbound packets from pendedRecvPackets

	KSPIN_LOCK	lock;				// Context spinlock
} IPCTX, *PIPCTX;

typedef struct _NF_IP_PACKET
{	
	LIST_ENTRY			entry;
	NF_IP_PACKET_OPTIONS options;
	BOOLEAN				isSend;
	char *				dataBuffer;
	ULONG				dataLength;
} NF_IP_PACKET, *PNF_IP_PACKET;

typedef struct _IP_HEADER_V4_
{
   union
   {
      UINT8 versionAndHeaderLength;
      struct
      {
         UINT8 headerLength : 4;
         UINT8 version : 4;
      };
   };
   union
   {
      UINT8  typeOfService;
      UINT8  differentiatedServicesCodePoint;
      struct
      {
         UINT8 explicitCongestionNotification : 2;
         UINT8 _typeOfService : 6;
      };
   };
   UINT16 totalLength;
   UINT16 identification;
   union
   {
      UINT16 flagsAndFragmentOffset;
      struct
      {
         UINT16 fragmentOffset : 13;
         UINT16 flags : 3;
      };
   };
   UINT8  timeToLive;
   UINT8  protocol;
   UINT16 checksum;
   BYTE   pSourceAddress[sizeof(UINT32)];
   BYTE   pDestinationAddress[sizeof(UINT32)];
   UINT8  pOptions[1];        
} IP_HEADER_V4, *PIP_HEADER_V4;

struct iphdr
{
    UINT8  HdrLength:4;
    UINT8  Version:4;
    UINT8  TOS;
    UINT16 Length;
    UINT16 Id;
    UINT16 FragOff0;
    UINT8  TTL;
    UINT8  Protocol;
    UINT16 Checksum;
    UINT32 SrcAddr;
    UINT32 DstAddr;
};

typedef struct _IP_HEADER_V6_
{
   union
   {
      UINT8 pVersionTrafficClassAndFlowLabel[4];
      struct
      {
       UINT8 r1 : 4;
       UINT8 value : 4;
       UINT8 r2;
       UINT8 r3;
       UINT8 r4;
      }version;
   };
   UINT16 payloadLength;
   UINT8  nextHeader;
   UINT8  hopLimit;
   BYTE   pSourceAddress[16];
   BYTE   pDestinationAddress[16];
} IP_HEADER_V6, *PIP_HEADER_V6;


BOOLEAN ipctx_init();
void ipctx_free();
void ipctx_cleanup();
PNF_IP_PACKET ipctx_allocPacket(ULONG dataLength);
void ipctx_freePacket(PNF_IP_PACKET pPacket);

BOOLEAN ipctx_pushPacket(
	const FWPS_INCOMING_VALUES* inFixedValues,
	const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	NET_BUFFER* netBuffer,
	BOOLEAN isSend,
	ULONG flags);

PIPCTX ipctx_get();

ULONG ipctx_getIPHeaderLengthAndProtocol(NET_BUFFER * nb, int * pProtocol);

#endif

