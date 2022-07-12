//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "rules.h"
#include "devctrl.h"

typedef UNALIGNED union _gen_addr
{
    struct in_addr sin_addr;
    struct in6_addr sin6_addr;
} gen_addr;

typedef struct _RULE_ENTRY
{
	LIST_ENTRY	entry;
	NF_RULE_EX		rule;
} RULE_ENTRY, *PRULE_ENTRY; 

static NPAGED_LOOKASIDE_LIST g_rulesLAList;
static LIST_ENTRY g_lRules;
static KSPIN_LOCK g_slRules;
static BOOLEAN	  g_initialized = FALSE;

static ULONG	g_rulesMask;

static LIST_ENTRY g_lTempRules;

typedef struct _BINDING_RULE_ENTRY
{
	LIST_ENTRY			entry;
	NF_BINDING_RULE		rule;
} BINDING_RULE_ENTRY, *PBINDING_RULE_ENTRY; 

static NPAGED_LOOKASIDE_LIST g_bindingRulesLAList;
static LIST_ENTRY g_lBindingRules;

NTSTATUS rules_init()
{
	InitializeListHead(&g_lRules);
	KeInitializeSpinLock(&g_slRules);

    ExInitializeNPagedLookasideList( &g_rulesLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( RULE_ENTRY ),
                                     MEM_TAG,
                                     0 );

	InitializeListHead(&g_lTempRules);

	InitializeListHead(&g_lBindingRules);

    ExInitializeNPagedLookasideList( &g_bindingRulesLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( BINDING_RULE_ENTRY ),
                                     MEM_TAG,
                                     0 );

	g_initialized = TRUE;

	g_rulesMask = RM_NONE;

	return STATUS_SUCCESS;
}

void rules_free()
{
	KdPrint((DPREFIX"rules_free\n"));

	if (g_initialized)
	{
		rules_remove_all();
		rules_remove_all_temp();
		ExDeleteNPagedLookasideList( &g_rulesLAList );
		ExDeleteNPagedLookasideList( &g_bindingRulesLAList );
		g_initialized = FALSE;
	}
}

ULONG rules_getRulesMask()
{
    KLOCK_QUEUE_HANDLE lh;
	ULONG mask;

    sl_lock(&g_slRules, &lh);	
	mask = g_rulesMask;
	sl_unlock(&lh);	

	return mask;
}

/**
 *  Add RULE to linked list
 */
void rules_add(PNF_RULE pRule, BOOLEAN toHead)
{
    KLOCK_QUEUE_HANDLE lh;
	PRULE_ENTRY pRuleEntry;

	pRuleEntry = ExAllocateFromNPagedLookasideList( &g_rulesLAList );
	if (!pRuleEntry)
		return;

	memset(&pRuleEntry->rule, 0, sizeof(NF_RULE_EX));

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE));

    sl_lock(&g_slRules, &lh);	

	if (toHead)
	{
		InsertHeadList(&g_lRules, &pRuleEntry->entry);
	} else
	{
		InsertTailList(&g_lRules, &pRuleEntry->entry);
	}

	if (pRule->protocol == IPPROTO_TCP)
	{
		g_rulesMask |= RM_TCP;

		if (pRule->ip_family == AF_INET6)
		{
			unsigned char loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH] = { 0 };

			loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH-1] = 1;

			if (memcmp(pRule->localIpAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
			{
				g_rulesMask |= RM_LOCAL_IPV6;
			} else
			if (memcmp(pRule->remoteIpAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
			{
				g_rulesMask |= RM_LOCAL_IPV6;
			} 
		}

	} else
	if (pRule->protocol == IPPROTO_UDP)
	{
		g_rulesMask |= RM_UDP;
	} 

	if (pRule->filteringFlag & NF_FILTER_AS_IP_PACKETS)
	{
		g_rulesMask |= RM_IP;
	}

    sl_unlock(&lh);	
}

/**
 *  Add RULE to linked list
 */
void rules_addEx(PNF_RULE_EX pRule, BOOLEAN toHead)
{
    KLOCK_QUEUE_HANDLE lh;
	PRULE_ENTRY pRuleEntry;

	pRuleEntry = ExAllocateFromNPagedLookasideList( &g_rulesLAList );
	if (!pRuleEntry)
		return;

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE_EX));

    sl_lock(&g_slRules, &lh);	

	if (toHead)
	{
		InsertHeadList(&g_lRules, &pRuleEntry->entry);
	} else
	{
		InsertTailList(&g_lRules, &pRuleEntry->entry);
	}

	if (pRule->protocol == IPPROTO_TCP)
	{
		g_rulesMask |= RM_TCP;

		if (pRule->ip_family == AF_INET6)
		{
			unsigned char loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH] = { 0 };

			loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH-1] = 1;

			if (memcmp(pRule->localIpAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
			{
				g_rulesMask |= RM_LOCAL_IPV6;
			} else
			if (memcmp(pRule->remoteIpAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
			{
				g_rulesMask |= RM_LOCAL_IPV6;
			} 
		}

	} else
	if (pRule->protocol == IPPROTO_UDP)
	{
		g_rulesMask |= RM_UDP;
	} 

	if (pRule->filteringFlag & NF_FILTER_AS_IP_PACKETS)
	{
		g_rulesMask |= RM_IP;
	}

    sl_unlock(&lh);	
}

/**
 *	Remove all rules from list
 */
void rules_remove_all()
{
	PRULE_ENTRY pRule;
    KLOCK_QUEUE_HANDLE lh;

    sl_lock(&g_slRules, &lh);	

	while (!IsListEmpty(&g_lRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_lRules);
		ExFreeToNPagedLookasideList( &g_rulesLAList, pRule );
	}

	g_rulesMask = RM_NONE;

	sl_unlock(&lh);	

	rules_bindingRemove_all();
}


/**
 *  Add RULE to temp linked list
 */
BOOLEAN rules_add_temp(PNF_RULE pRule)
{
    KLOCK_QUEUE_HANDLE lh;
	PRULE_ENTRY pRuleEntry;

	pRuleEntry = ExAllocateFromNPagedLookasideList( &g_rulesLAList );
	if (!pRuleEntry)
		return FALSE;

	memset(&pRuleEntry->rule, 0, sizeof(NF_RULE_EX));

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE));

    sl_lock(&g_slRules, &lh);	

	InsertTailList(&g_lTempRules, &pRuleEntry->entry);

    sl_unlock(&lh);	

	return TRUE;
}

/**
 *  Add RULE to temp linked list
 */
BOOLEAN rules_add_tempEx(PNF_RULE_EX pRule)
{
    KLOCK_QUEUE_HANDLE lh;
	PRULE_ENTRY pRuleEntry;

	pRuleEntry = ExAllocateFromNPagedLookasideList( &g_rulesLAList );
	if (!pRuleEntry)
		return FALSE;

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE_EX));

    sl_lock(&g_slRules, &lh);	

	InsertTailList(&g_lTempRules, &pRuleEntry->entry);

    sl_unlock(&lh);	

	return TRUE;
}

/**
 *	Remove all rules from temp list
 */
void rules_remove_all_temp()
{
	PRULE_ENTRY pRule;
    KLOCK_QUEUE_HANDLE lh;

    sl_lock(&g_slRules, &lh);	

	while (!IsListEmpty(&g_lTempRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_lTempRules);
		ExFreeToNPagedLookasideList( &g_rulesLAList, pRule );
	}

	sl_unlock(&lh);	
}

/**
 *  Assign temp rules list as current
 */
void rules_set_temp()
{
    KLOCK_QUEUE_HANDLE lh;
	PRULE_ENTRY pRule;

    sl_lock(&g_slRules, &lh);	

	while (!IsListEmpty(&g_lRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_lRules);
		ExFreeToNPagedLookasideList( &g_rulesLAList, pRule );
	}

	g_rulesMask = RM_NONE;

	while (!IsListEmpty(&g_lTempRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_lTempRules);

		InsertTailList(&g_lRules, &pRule->entry);

		if (pRule->rule.protocol == IPPROTO_TCP)
		{
			g_rulesMask |= RM_TCP;

			if (pRule->rule.ip_family == AF_INET6)
			{
				unsigned char loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH] = { 0 };

				loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH-1] = 1;

				if (memcmp(pRule->rule.localIpAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
				{
					g_rulesMask |= RM_LOCAL_IPV6;
				} else
				if (memcmp(pRule->rule.remoteIpAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
				{
					g_rulesMask |= RM_LOCAL_IPV6;
				} 
			}
		} else
		if (pRule->rule.protocol == IPPROTO_UDP)
		{
			g_rulesMask |= RM_UDP;
		} 

		if (pRule->rule.filteringFlag & NF_FILTER_AS_IP_PACKETS)
		{
			g_rulesMask |= RM_IP;
		}
	}

    sl_unlock(&lh);	
}

static BOOLEAN isZeroBuffer(const unsigned char * buf, int len)
{
	int i;
	for (i=0; i<len; i++)
	{
		if (buf[i] != 0)
			return FALSE;
	}
	return TRUE;
}

/**
*	Returns TRUE if pAddress matches pRuleAddress/pRuleAddressMask
*/
static BOOLEAN rules_isEqualIpAddresses(USHORT family,
								gen_addr * pRuleAddress, 
								gen_addr * pRuleAddressMask,
								gen_addr * pAddress)
{
	switch (family)
	{
	case AF_INET:
		if (!pRuleAddress->sin_addr.S_un.S_addr)
			return TRUE;

		if (pRuleAddressMask->sin_addr.S_un.S_addr)
		{
			return (pAddress->sin_addr.S_un.S_addr & pRuleAddressMask->sin_addr.S_un.S_addr) ==
				(pRuleAddress->sin_addr.S_un.S_addr & pRuleAddressMask->sin_addr.S_un.S_addr);
		} else
		{
			return pAddress->sin_addr.S_un.S_addr == pRuleAddress->sin_addr.S_un.S_addr;
		}
		break;

	case AF_INET6:
		{
			int i;

			if (isZeroBuffer((unsigned char *)&pRuleAddress->sin6_addr, sizeof(pRuleAddress->sin6_addr)))
				return TRUE;

			if (!isZeroBuffer((unsigned char *)&pRuleAddressMask->sin6_addr, sizeof(pRuleAddressMask->sin6_addr)))
			{
				for (i=0; i<8; i++)
				{
					if ((pRuleAddress->sin6_addr.u.Word[i] & pRuleAddressMask->sin6_addr.u.Word[i]) !=
						(pAddress->sin6_addr.u.Word[i] & pRuleAddressMask->sin6_addr.u.Word[i]))
					{
						return FALSE;
					}
				}

				return TRUE;
			} else
			{
				for (i=0; i<8; i++)
				{
					if (pRuleAddress->sin6_addr.u.Word[i] != pAddress->sin6_addr.u.Word[i])
					{
						return FALSE;
					}
				}

				return TRUE;
			}
		}
		break;

	default:
		break;
	}

	return FALSE;
}

BOOLEAN rules_nameMatches(const wchar_t * mask, size_t maskLen, const wchar_t * name, size_t nameLen)
{
	const wchar_t *p, *s;

	s = mask;
	p = name;

	while (maskLen > 0 && nameLen > 0)
	{
		if (*s == L'*')
		{
			s--;
			maskLen--;
			while (nameLen > 0)
			{
				if (rules_nameMatches(s, maskLen, p, nameLen))
				{
					return TRUE;
				}
				p--;
				nameLen--;
			}
			return (maskLen == 0);
		}

		if (*s != *p)
			return FALSE;

		s--;
		maskLen--;

		p--;
		nameLen--;
	}

	if (maskLen == 0)
	{
		return TRUE;
	}

	if (maskLen == 1 && nameLen == 0 && *s == L'*')
	{
		return TRUE;
	}

	return FALSE;
}


/**
* Search the list of rules and return the flag of a first rule that matches pCtx
* @return NF_FILTERING_FLAG 
* @param pCtx 
**/
NF_FILTERING_FLAG rules_findByTcpCtx(PTCPCTX pCtx)
{
	PRULE_ENTRY	pRuleEntry;
	PNF_RULE_EX	pRule;
	sockaddr_gen * pLocalAddress = NULL;
	sockaddr_gen * pRemoteAddress = NULL;
	gen_addr * pRuleLocalIpAddress = NULL;
	gen_addr * pRuleLocalIpAddressMask = NULL;
	gen_addr * pRuleRemoteIpAddress = NULL;
	gen_addr * pRuleRemoteIpAddressMask = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	size_t processNameLen = 0;
    KLOCK_QUEUE_HANDLE lh;

#ifdef _DEMO
	static unsigned int counter = (unsigned int)-1;
	if (counter == 0)
	{
		return NF_ALLOW;
	}		
	counter--;
#endif

	if (!devctrl_isProxyAttached() ||
		(pCtx->processId == devctrl_getProxyPid()))
	{
		return NF_ALLOW;
	}

	// EDR_FIX: Checks that need to collect events for the process
	if(cmdedr_isProcessInWhiteList(pCtx->processId))
		return NF_ALLOW;
	
	pLocalAddress = (sockaddr_gen*)pCtx->localAddr;
	pRemoteAddress = (sockaddr_gen*)pCtx->remoteAddr;

	if (!(rules_getRulesMask() & RM_LOCAL_IPV6))
	{
		if (pRemoteAddress->AddressIn.sin_family == AF_INET6)
		{
			unsigned char loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH] = { 0 };
			unsigned char * ipAddress = (pCtx->direction == NF_D_IN)? 
				pLocalAddress->AddressIn6.sin6_addr.u.Byte :
				pRemoteAddress->AddressIn6.sin6_addr.u.Byte;

			loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH-1] = 1;

			// Do not filter local IPv6 connections
			if (memcmp(ipAddress, loopbackAddr, NF_MAX_IP_ADDRESS_LENGTH) == 0)
			{
				return NF_ALLOW;
			}
		}
	}

	if (pCtx->processName[0] != 0)
	{
		processNameLen = wcslen((wchar_t*)&pCtx->processName);
	}

	sl_lock(&g_slRules, &lh);	

	if (IsListEmpty(&g_lRules))
	{
		sl_unlock(&lh);	
		return NF_ALLOW;
	}

	pRuleEntry = (PRULE_ENTRY)g_lRules.Flink;

	while (pRuleEntry != (PRULE_ENTRY)&g_lRules)
	{
		pRule = &pRuleEntry->rule;

		if (pRule->filteringFlag & NF_FILTER_AS_IP_PACKETS)
		{
			goto next_rule;
		}

		if ((pRule->processId != 0) && (pRule->processId != pCtx->processId))
		{
			goto next_rule;
		}

		if ((pRule->protocol != 0) && (pRule->protocol != IPPROTO_TCP))
		{
			goto next_rule;
		}

		if ((pRule->direction != 0) && !(pRule->direction & pCtx->direction))
		{
			goto next_rule;
		}

		if ((pRule->localPort != 0) && (pRule->localPort != pLocalAddress->AddressIn.sin_port))
		{
			goto next_rule;
		}

		if ((pRule->remotePort != 0) && (pRule->remotePort != pRemoteAddress->AddressIn.sin_port))
		{
			goto next_rule;
		}

		pRuleLocalIpAddress = (gen_addr*)pRule->localIpAddress;
		pRuleRemoteIpAddress = (gen_addr*)pRule->remoteIpAddress;
		pRuleLocalIpAddressMask = (gen_addr*)pRule->localIpAddressMask;
		pRuleRemoteIpAddressMask = (gen_addr*)pRule->remoteIpAddressMask;
		
		if (pRule->ip_family != 0)
		{
			if (pRule->ip_family != pLocalAddress->AddressIn.sin_family)
				goto next_rule;

			switch (pRule->ip_family)
			{
			case AF_INET:
				
				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleRemoteIpAddress,
						pRuleRemoteIpAddressMask,
						(gen_addr*)&pRemoteAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				break;

			case AF_INET6:
				
				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleRemoteIpAddress,
						pRuleRemoteIpAddressMask,
						(gen_addr*)&pRemoteAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				break;

			default:
				break;
			}
		}
		
		if (pRule->processName[0] != 0)
		{
			size_t len;
			
			len = wcslen((wchar_t*)&pRule->processName);

			if (!rules_nameMatches(
					(wchar_t*)&pRule->processName + len - 1, len, 
					(wchar_t*)&pCtx->processName + processNameLen - 1, processNameLen))
			{
				goto next_rule;
			}
		}

		flag = pRule->filteringFlag;

		sl_unlock(&lh);	


		return flag;

next_rule:
		
		pRuleEntry = (PRULE_ENTRY)pRuleEntry->entry.Flink;
	}

	sl_unlock(&lh);	

	return NF_ALLOW;
}


NF_FILTERING_FLAG rules_findByUdpInfo(PUDPCTX pCtx, char * remoteAddress, UCHAR direction)
{
	PRULE_ENTRY	pRuleEntry;
	PNF_RULE_EX	pRule;
	sockaddr_gen * pLocalAddress = NULL;
	sockaddr_gen * pRemoteAddress = NULL;
	gen_addr * pRuleLocalIpAddress = NULL;
	gen_addr * pRuleLocalIpAddressMask = NULL;
	gen_addr * pRuleRemoteIpAddress = NULL;
	gen_addr * pRuleRemoteIpAddressMask = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	size_t processNameLen = 0;
    KLOCK_QUEUE_HANDLE lh;

#ifdef _DEMO
	static unsigned int counter = (unsigned int)-1;
	if (counter == 0)
	{
		return NF_ALLOW;
	}		
	counter--;
#endif

	if (!devctrl_isProxyAttached() ||
		(pCtx->processId == devctrl_getProxyPid()))
	{
		return NF_ALLOW;
	}
	
	pLocalAddress = (sockaddr_gen*)pCtx->localAddr;
	pRemoteAddress = (sockaddr_gen*)remoteAddress;

	if (pCtx->processName[0] != 0)
	{
		processNameLen = wcslen((wchar_t*)&pCtx->processName);
	}

	sl_lock(&g_slRules, &lh);	

	if (IsListEmpty(&g_lRules))
	{
		sl_unlock(&lh);	
		return NF_ALLOW;
	}

	pRuleEntry = (PRULE_ENTRY)g_lRules.Flink;

	while (pRuleEntry != (PRULE_ENTRY)&g_lRules)
	{
		pRule = &pRuleEntry->rule;

		if (pRule->filteringFlag & NF_FILTER_AS_IP_PACKETS)
		{
			goto next_rule;
		}

		if ((pRule->protocol != 0) && (pRule->protocol != IPPROTO_UDP))
		{
			goto next_rule;
		}

		if ((pRule->processId != 0) && (pRule->processId != pCtx->processId))
		{
			goto next_rule;
		}

		if ((pRule->direction != 0) && !(pRule->direction & direction))
		{
			goto next_rule;
		}

		if ((pRule->localPort != 0) && (pRule->localPort != pLocalAddress->AddressIn.sin_port))
		{
			goto next_rule;
		}

		if ((pRule->remotePort != 0) && (pRule->remotePort != pRemoteAddress->AddressIn.sin_port))
		{
			goto next_rule;
		}

		pRuleLocalIpAddress = (gen_addr*)pRule->localIpAddress;
		pRuleRemoteIpAddress = (gen_addr*)pRule->remoteIpAddress;
		pRuleLocalIpAddressMask = (gen_addr*)pRule->localIpAddressMask;
		pRuleRemoteIpAddressMask = (gen_addr*)pRule->remoteIpAddressMask;
		
		if (pRule->ip_family != 0)
		{
			if (pRule->ip_family != pLocalAddress->AddressIn.sin_family)
				goto next_rule;

			switch (pRule->ip_family)
			{
			case AF_INET:
				
				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleRemoteIpAddress,
						pRuleRemoteIpAddressMask,
						(gen_addr*)&pRemoteAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				break;

			case AF_INET6:
				
				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleRemoteIpAddress,
						pRuleRemoteIpAddressMask,
						(gen_addr*)&pRemoteAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				break;

			default:
				break;
			}
		}
		
		if (pRule->processName[0] != 0)
		{
			size_t len;
			
			len = wcslen((wchar_t*)&pRule->processName);

			if (!rules_nameMatches(
					(wchar_t*)&pRule->processName + len - 1, len, 
					(wchar_t*)&pCtx->processName + processNameLen - 1, processNameLen))
			{
				goto next_rule;
			}
		}

		flag = pRule->filteringFlag;

		sl_unlock(&lh);	

		return flag;

next_rule:
		
		pRuleEntry = (PRULE_ENTRY)pRuleEntry->entry.Flink;
	}

	sl_unlock(&lh);	

	return NF_ALLOW;
}


NF_FILTERING_FLAG rules_findByIPInfo(PPACKET_INFO pPacketInfo)
{
	PRULE_ENTRY	pRuleEntry;
	PNF_RULE_EX	pRule;
	sockaddr_gen * pLocalAddress = NULL;
	sockaddr_gen * pRemoteAddress = NULL;
	gen_addr * pRuleLocalIpAddress = NULL;
	gen_addr * pRuleLocalIpAddressMask = NULL;
	gen_addr * pRuleRemoteIpAddress = NULL;
	gen_addr * pRuleRemoteIpAddressMask = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
    KLOCK_QUEUE_HANDLE lh;

#ifdef _DEMO
	static unsigned int counter = (unsigned int)-1;
	if (counter == 0)
	{
		return NF_ALLOW;
	}		
	counter--;
#endif

	if (!devctrl_isProxyAttached())
	{
		return NF_ALLOW;
	}
	
	pLocalAddress = &pPacketInfo->localAddress;
	pRemoteAddress = &pPacketInfo->remoteAddress;

    sl_lock(&g_slRules, &lh);	

	if (IsListEmpty(&g_lRules))
	{
		sl_unlock(&lh);	
		return NF_ALLOW;
	}

	pRuleEntry = (PRULE_ENTRY)g_lRules.Flink;

	while (pRuleEntry != (PRULE_ENTRY)&g_lRules)
	{
		pRule = &pRuleEntry->rule;

		if (!(pRule->filteringFlag & NF_FILTER_AS_IP_PACKETS))
		{
			goto next_rule;
		}

		if ((pRule->direction != 0) && !(pRule->direction & pPacketInfo->direction))
		{
			goto next_rule;
		}

		if ((pRule->protocol != 0) && (pRule->protocol != pPacketInfo->protocol))
		{
			goto next_rule;
		}

		if ((pRule->localPort != 0) || (pRule->remotePort != 0))
		{
			if (pPacketInfo->protocol == IPPROTO_TCP ||
				pPacketInfo->protocol == IPPROTO_UDP)
			{
				if ((pRule->localPort != 0) && (pRule->localPort != pLocalAddress->AddressIn.sin_port))
				{
					goto next_rule;
				}

				if ((pRule->remotePort != 0) && (pRule->remotePort != pRemoteAddress->AddressIn.sin_port))
				{
					goto next_rule;
				}
			} else
			{
				goto next_rule;
			}
		}

		pRuleLocalIpAddress = (gen_addr*)pRule->localIpAddress;
		pRuleRemoteIpAddress = (gen_addr*)pRule->remoteIpAddress;
		pRuleLocalIpAddressMask = (gen_addr*)pRule->localIpAddressMask;
		pRuleRemoteIpAddressMask = (gen_addr*)pRule->remoteIpAddressMask;
		
		if (pRule->ip_family != 0)
		{
			if (pRule->ip_family != pLocalAddress->AddressIn.sin_family)
				goto next_rule;

			switch (pRule->ip_family)
			{
			case AF_INET:
				
				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleRemoteIpAddress,
						pRuleRemoteIpAddressMask,
						(gen_addr*)&pRemoteAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				break;

			case AF_INET6:
				
				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleRemoteIpAddress,
						pRuleRemoteIpAddressMask,
						(gen_addr*)&pRemoteAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				break;

			default:
				break;
			}
		}
		
		flag = pRule->filteringFlag;

		sl_unlock(&lh);	

		return flag;

next_rule:
		
		pRuleEntry = (PRULE_ENTRY)pRuleEntry->entry.Flink;
	}

	sl_unlock(&lh);	

	return NF_ALLOW;
}


/**
 *  Add binding rule to linked list
 */
void rules_bindingAdd(PNF_BINDING_RULE pRule, BOOLEAN toHead)
{
    KLOCK_QUEUE_HANDLE lh;
	PBINDING_RULE_ENTRY pRuleEntry;

	pRuleEntry = (PBINDING_RULE_ENTRY)ExAllocateFromNPagedLookasideList( &g_bindingRulesLAList );
	if (!pRuleEntry)
		return;

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_BINDING_RULE));

    sl_lock(&g_slRules, &lh);	

	if (toHead)
	{
		InsertHeadList(&g_lBindingRules, &pRuleEntry->entry);
	} else
	{
		InsertTailList(&g_lBindingRules, &pRuleEntry->entry);
	}

    sl_unlock(&lh);	
}

/**
 *	Remove all binding rules from list
 */
void rules_bindingRemove_all()
{
	PBINDING_RULE_ENTRY pRule;
    KLOCK_QUEUE_HANDLE lh;

    sl_lock(&g_slRules, &lh);	

	while (!IsListEmpty(&g_lBindingRules))
	{
		pRule = (PBINDING_RULE_ENTRY)RemoveHeadList(&g_lBindingRules);
		ExFreeToNPagedLookasideList( &g_bindingRulesLAList, pRule );
	}

	sl_unlock(&lh);	
}

NF_FILTERING_FLAG rules_findByBindInfo(PNF_BINDING_RULE pBindInfo)
{
	PBINDING_RULE_ENTRY	pRuleEntry;
	PNF_BINDING_RULE	pRule;
	gen_addr * pLocalAddress = NULL;
	gen_addr * pRuleLocalIpAddress = NULL;
	gen_addr * pRuleLocalIpAddressMask = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	size_t processNameLen = 0;
    KLOCK_QUEUE_HANDLE lh;

#ifdef _DEMO
	static unsigned int counter = (unsigned int)-1;
	if (counter == 0)
	{
		return NF_ALLOW;
	}		
	counter--;
#endif

	if (!devctrl_isProxyAttached())
	{
		return NF_ALLOW;
	}
	
	pLocalAddress = (gen_addr *)&pBindInfo->localIpAddress;

	if (pBindInfo->processName[0] != 0)
	{
		processNameLen = wcslen((wchar_t*)&pBindInfo->processName);
	}

    sl_lock(&g_slRules, &lh);	

	if (IsListEmpty(&g_lBindingRules))
	{
		sl_unlock(&lh);	
		return NF_ALLOW;
	}

	pRuleEntry = (PBINDING_RULE_ENTRY)g_lBindingRules.Flink;

	while (pRuleEntry != (PBINDING_RULE_ENTRY)&g_lBindingRules)
	{
		pRule = &pRuleEntry->rule;

		if ((pRule->protocol != 0) && (pRule->protocol != pBindInfo->protocol))
		{
			goto next_rule;
		}

		if ((pRule->processId != 0) && (pRule->processId != pBindInfo->processId))
		{
			goto next_rule;
		}

		if ((pRule->localPort != 0) && (pRule->localPort != pBindInfo->localPort))
		{
			goto next_rule;
		}

		pRuleLocalIpAddress = (gen_addr*)pRule->localIpAddress;
		pRuleLocalIpAddressMask = (gen_addr*)pRule->localIpAddressMask;
		
		if (pRule->ip_family != 0)
		{
			if (pRule->ip_family != pBindInfo->ip_family)
				goto next_rule;

			switch (pRule->ip_family)
			{
			case AF_INET:
				
				if (!rules_isEqualIpAddresses(
						AF_INET,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						pLocalAddress
						))
				{
					goto next_rule;
				}

				break;

			case AF_INET6:
				
				if (!rules_isEqualIpAddresses(
						AF_INET6,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						pLocalAddress
						))
				{
					goto next_rule;
				}

				break;

			default:
				break;
			}
		}

		if (pRule->processName[0] != 0)
		{
			size_t len;
			
			len = wcslen((wchar_t*)&pRule->processName);

			if (!rules_nameMatches(
					(wchar_t*)&pRule->processName + len - 1, len, 
					(wchar_t*)&pBindInfo->processName + processNameLen - 1, processNameLen))
			{
				goto next_rule;
			}
		}

		flag = pRule->filteringFlag;

		if (flag != NF_ALLOW)
		{
			memcpy(pBindInfo->newLocalIpAddress, pRule->newLocalIpAddress, NF_MAX_IP_ADDRESS_LENGTH);
			pBindInfo->newLocalPort = pRule->newLocalPort;
		}

		sl_unlock(&lh);	

		return flag;

next_rule:
		
		pRuleEntry = (PBINDING_RULE_ENTRY)pRuleEntry->entry.Flink;
	}

	sl_unlock(&lh);	

	return NF_ALLOW;
}
