//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "gvars.h"
#include "rules.h"
#include "tcpconn.h"
#include "mempools.h"

static ULONG g_rulesMask = RM_NONE;

ULONG rule_getRulesMask()
{
    KIRQL irql;
	ULONG mask;

    sl_lock(&g_vars.slRules, &irql);	
	mask = g_rulesMask;
    sl_unlock(&g_vars.slRules, irql);	

	return mask;
}

/**
 *  Add RULE to linked list
 */
void rule_add(PRULE_ENTRY pRule, BOOLEAN toHead)
{
    KIRQL irql;

    sl_lock(&g_vars.slRules, &irql);	

	if (toHead)
	{
		InsertHeadList(&g_vars.lRules, &pRule->entry);
	} else
	{
		InsertTailList(&g_vars.lRules, &pRule->entry);
	}

	if (pRule->rule.protocol == IPPROTO_TCP)
	{
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
	}

	sl_unlock(&g_vars.slRules, irql);	
}


/**
 *	Remove all rules from list
 */
void rule_remove_all()
{
    KIRQL irql;

    sl_lock(&g_vars.slRules, &irql);	

	while (!IsListEmpty(&g_vars.lRules))
	{
		PRULE_ENTRY pRule = (PRULE_ENTRY)RemoveHeadList(&g_vars.lRules);
		mp_free(pRule, 1);
	}

	g_rulesMask = RM_NONE;

	sl_unlock(&g_vars.slRules, irql);	
}

/**
 *  Add NF_RULE to temp linked list
 */
BOOLEAN rule_add_temp(PNF_RULE pRule)
{
    KIRQL irql;
	PRULE_ENTRY pRuleEntry;

	pRuleEntry = (PRULE_ENTRY)mp_alloc(sizeof(RULE_ENTRY));
	if (!pRuleEntry)
		return FALSE;

	memset(&pRuleEntry->rule, 0, sizeof(NF_RULE_EX));

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE));

    sl_lock(&g_vars.slRules, &irql);	

	InsertTailList(&g_vars.lTempRules, &pRuleEntry->entry);

	sl_unlock(&g_vars.slRules, irql);	

	return TRUE;
}

/**
 *  Add NF_RULE_EX to temp linked list
 */
BOOLEAN rule_add_tempEx(PNF_RULE_EX pRule)
{
    KIRQL irql;
	PRULE_ENTRY pRuleEntry;

	pRuleEntry = (PRULE_ENTRY)mp_alloc(sizeof(RULE_ENTRY));
	if (!pRuleEntry)
		return FALSE;

	memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE_EX));

    sl_lock(&g_vars.slRules, &irql);	

	InsertTailList(&g_vars.lTempRules, &pRuleEntry->entry);

	sl_unlock(&g_vars.slRules, irql);	

	return TRUE;
}

/**
 *	Remove all rules from temp list
 */
void rule_remove_all_temp()
{
    KIRQL irql;
	PRULE_ENTRY pRule;

    sl_lock(&g_vars.slRules, &irql);	

	while (!IsListEmpty(&g_vars.lTempRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_vars.lTempRules);
		mp_free(pRule, 1);
	}

	sl_unlock(&g_vars.slRules, irql);	
}

/**
 *  Assign temp rules list as current
 */
void rule_set_temp()
{
    KIRQL irql;
	PRULE_ENTRY pRule;

    sl_lock(&g_vars.slRules, &irql);	

	while (!IsListEmpty(&g_vars.lRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_vars.lRules);
		mp_free(pRule, 1);
	}

	g_rulesMask = RM_NONE;

	while (!IsListEmpty(&g_vars.lTempRules))
	{
		pRule = (PRULE_ENTRY)RemoveHeadList(&g_vars.lTempRules);

		InsertTailList(&g_vars.lRules, &pRule->entry);

		if (pRule->rule.protocol == IPPROTO_TCP)
		{
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
		}
	}

	sl_unlock(&g_vars.slRules, irql);	
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
static BOOLEAN rule_isEqualIpAddresses(USHORT family,
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

BOOLEAN rule_nameMatches(const wchar_t * mask, size_t maskLen, const wchar_t * name, size_t nameLen)
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
				if (rule_nameMatches(s, maskLen, p, nameLen))
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
* Search the list of rules and return the flag of a first rule that matches pConn
* @return NF_FILTERING_FLAG 
* @param pConn 
**/
NF_FILTERING_FLAG rule_findByConn(PCONNOBJ pConn)
{
	PRULE_ENTRY	pRuleEntry;
	PNF_RULE_EX	pRule;
	ULONG processId = 0;
	BOOLEAN matched = FALSE;
	sockaddr_gen * pLocalAddress = NULL;
	sockaddr_gen * pRemoteAddress = NULL;
	gen_addr * pRuleLocalIpAddress = NULL;
	gen_addr * pRuleLocalIpAddressMask = NULL;
	gen_addr * pRuleRemoteIpAddress = NULL;
	gen_addr * pRuleRemoteIpAddressMask = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	size_t processNameLen = 0;
    KIRQL irql;

#ifdef _DEMO
	static unsigned int counter = -1;
	if (counter == 0)
	{
		return NF_ALLOW;
	}		
	counter--;
#endif

	if ((pConn->pAddr == NULL) || (g_vars.proxyAttached == 0))
	{
		return NF_ALLOW;
	}
	
	processId = pConn->processId;

	if (processId == *(ULONG*)&g_vars.proxyPid)
	{
		return NF_ALLOW;
	}

	if (pConn->pAddr->processName[0] != 0)
	{
		processNameLen = wcslen((wchar_t*)&pConn->pAddr->processName);
	}

	pLocalAddress = (sockaddr_gen*)pConn->localAddr;
	pRemoteAddress = (sockaddr_gen*)pConn->remoteAddr;

	if (!(rule_getRulesMask() & RM_LOCAL_IPV6))
	{
		if (pRemoteAddress->AddressIn.sin_family == AF_INET6)
		{
			unsigned char loopbackAddr[NF_MAX_IP_ADDRESS_LENGTH] = { 0 };
			unsigned char * ipAddress = (pConn->direction == NF_D_IN)? 
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

    sl_lock(&g_vars.slRules, &irql);	

	if (IsListEmpty(&g_vars.lRules))
	{
		sl_unlock(&g_vars.slRules, irql);	
		return NF_ALLOW;
	}

	pRuleEntry = (PRULE_ENTRY)g_vars.lRules.Flink;

	while (pRuleEntry != (PRULE_ENTRY)&g_vars.lRules)
	{
		pRule = &pRuleEntry->rule;

		if ((pRule->processId != 0) && (pRule->processId != processId))
		{
			goto next_rule;
		}

		if ((pRule->protocol != 0) && (pRule->protocol != IPPROTO_TCP))
		{
			goto next_rule;
		}

		if ((pRule->direction != 0) && !(pRule->direction & pConn->direction))
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
				
				if (!rule_isEqualIpAddresses(
						AF_INET,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				if (!rule_isEqualIpAddresses(
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
				
				if (!rule_isEqualIpAddresses(
						AF_INET6,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				if (!rule_isEqualIpAddresses(
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

			if (!rule_nameMatches(
					(wchar_t*)&pRule->processName + len - 1, len, 
					(wchar_t*)&pConn->pAddr->processName + processNameLen - 1, processNameLen))
			{
				goto next_rule;
			}
		}

		flag = pRule->filteringFlag;

		sl_unlock(&g_vars.slRules, irql);	

		if (!(flag & NF_BLOCK))
		{
			unsigned short port;

			if (pRemoteAddress->AddressIn.sin_family == AF_INET)
			{
				port = (pConn->direction == NF_D_IN)? 
					pLocalAddress->AddressIn.sin_port :
					pRemoteAddress->AddressIn.sin_port;
			} else
			{
				port = (pConn->direction == NF_D_IN)? 
					pLocalAddress->AddressIn6.sin6_port :
					pRemoteAddress->AddressIn6.sin6_port;
			}

			// Do not filter NetBT packets
			switch (ntohs(port))
			{
				case 445:
				case 139:
				case 137:
				case 135:
					return NF_ALLOW;
			}
		}

		return flag;

next_rule:
		
		pRuleEntry = (PRULE_ENTRY)pRuleEntry->entry.Flink;
	}

	sl_unlock(&g_vars.slRules, irql);	

	return NF_ALLOW;
}

/**
* Search the list of rules and return the flag of a first rule that matches specified parameters
* @return NF_FILTERING_FLAG 
* @param pAddr 
* @param direction 
* @param remoteAddr 
**/
NF_FILTERING_FLAG rule_findByAddr(PADDROBJ pAddr, NF_DIRECTION direction, sockaddr_gen * remoteAddr)
{
	PRULE_ENTRY	pRuleEntry;
	PNF_RULE_EX	pRule;
	ULONG processId = 0;
	BOOLEAN matched = FALSE;
	sockaddr_gen * pLocalAddress = NULL;
	sockaddr_gen * pRemoteAddress = NULL;
	gen_addr * pRuleLocalIpAddress = NULL;
	gen_addr * pRuleLocalIpAddressMask = NULL;
	gen_addr * pRuleRemoteIpAddress = NULL;
	gen_addr * pRuleRemoteIpAddressMask = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	size_t processNameLen = 0;
    KIRQL irql;

#ifdef _DEMO
	static unsigned int counter = -1;
	if (counter == 0)
	{
		return NF_ALLOW;
	}		
	counter--;
#endif

	if ((pAddr == NULL) || (g_vars.proxyAttached == 0))
	{
		return NF_ALLOW;
	}
	
	processId = pAddr->processId;

	if (processId == *(ULONG*)&g_vars.proxyPid)
	{
		return NF_ALLOW;
	}

	if (pAddr->processName[0] != 0)
	{
		processNameLen = wcslen((wchar_t*)&pAddr->processName);
	}

	pLocalAddress = (sockaddr_gen*)pAddr->localAddr;
	pRemoteAddress = (sockaddr_gen*)remoteAddr;

    sl_lock(&g_vars.slRules, &irql);	

	if (IsListEmpty(&g_vars.lRules))
	{
		sl_unlock(&g_vars.slRules, irql);	
		return NF_ALLOW;
	}

	pRuleEntry = (PRULE_ENTRY)g_vars.lRules.Flink;

	while (pRuleEntry != (PRULE_ENTRY)&g_vars.lRules)
	{
		pRule = &pRuleEntry->rule;

		if ((pRule->processId != 0) && (pRule->processId != processId))
		{
			goto next_rule;
		}

		if ((pRule->protocol != 0) && (pRule->protocol != IPPROTO_UDP))
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
				
				if (!rule_isEqualIpAddresses(
						AF_INET,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn.sin_addr
						))
				{
					goto next_rule;
				}

				if (!rule_isEqualIpAddresses(
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
				
				if (!rule_isEqualIpAddresses(
						AF_INET6,
						pRuleLocalIpAddress,
						pRuleLocalIpAddressMask,
						(gen_addr*)&pLocalAddress->AddressIn6.sin6_addr
						))
				{
					goto next_rule;
				}

				if (!rule_isEqualIpAddresses(
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

			if (!rule_nameMatches(
					(wchar_t*)&pRule->processName + len - 1, len, 
					(wchar_t*)&pAddr->processName + processNameLen - 1, processNameLen))
			{
				goto next_rule;
			}
		}

		flag = pRule->filteringFlag;

		sl_unlock(&g_vars.slRules, irql);	

		return flag;

next_rule:
		
		pRuleEntry = (PRULE_ENTRY)pRuleEntry->entry.Flink;
	}

	sl_unlock(&g_vars.slRules, irql);	

	return NF_ALLOW;
}

