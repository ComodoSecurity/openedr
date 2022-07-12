//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _RULES_H
#define _RULES_H

#include "nfdriver.h"
#include "tcpconn.h"

typedef struct _RULE_ENTRY
{
	LIST_ENTRY	entry;
	NF_RULE_EX	rule;
} RULE_ENTRY, *PRULE_ENTRY; 

enum RULES_MASK
{
	RM_NONE,
	RM_LOCAL_IPV6 = 8,
};

ULONG rule_getRulesMask();

void rule_add(PRULE_ENTRY pRule, BOOLEAN toHead);
void rule_remove_all();
BOOLEAN rule_add_temp(PNF_RULE pRule);
BOOLEAN rule_add_tempEx(PNF_RULE_EX pRule);
void rule_remove_all_temp();
void rule_set_temp();

NF_FILTERING_FLAG rule_findByConn(PCONNOBJ pConn);
NF_FILTERING_FLAG rule_findByAddr(PADDROBJ pAddr, NF_DIRECTION direction, sockaddr_gen * remoteAddr);


#endif