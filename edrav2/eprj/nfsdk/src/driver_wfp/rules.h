//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _RULES_H
#define _RULES_H

#include "nfdriver.h"
#include "tcpctx.h"
#include "udpctx.h"

NTSTATUS rules_init();
void rules_free();

void rules_add(PNF_RULE pRule, BOOLEAN toHead);
void rules_addEx(PNF_RULE_EX pRule, BOOLEAN toHead);
void rules_remove_all();

void rules_bindingAdd(PNF_BINDING_RULE pRule, BOOLEAN toHead);
void rules_bindingRemove_all();

BOOLEAN rules_add_temp(PNF_RULE pRule);
BOOLEAN rules_add_tempEx(PNF_RULE_EX pRule);
void rules_remove_all_temp();
void rules_set_temp();

NF_FILTERING_FLAG rules_findByTcpCtx(PTCPCTX pCtx);
NF_FILTERING_FLAG rules_findByUdpInfo(PUDPCTX pCtx, char * remoteAddress, UCHAR direction);
NF_FILTERING_FLAG rules_findByBindInfo(PNF_BINDING_RULE pBindInfo);

typedef struct _PACKET_INFO
{
	int				protocol;
	sockaddr_gen	localAddress;
	sockaddr_gen	remoteAddress;
	UCHAR			direction;
} PACKET_INFO, *PPACKET_INFO;

NF_FILTERING_FLAG rules_findByIPInfo(PPACKET_INFO pPacketInfo);

enum RULES_MASK
{
	RM_NONE,
	RM_TCP = 1,
	RM_UDP = 2,
	RM_IP = 4,
	RM_LOCAL_IPV6 = 8
};

ULONG rules_getRulesMask();

#endif