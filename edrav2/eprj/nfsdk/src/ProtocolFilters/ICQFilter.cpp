//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdafx.h"
#include "ICQFilter.h"
#include "ProxySession.h"
#include "auxutil.h"

using namespace ProtocolFilters;

#define MAX_CMD_BUF_LEN 65536 + sizeof(OSCAR_FLAP)
#define MAX_RES_BUF_LEN 65536 + sizeof(OSCAR_FLAP)

const GUID GUID_AIM_FileTransfer =
{0x43134609,0x7F4C,0xD111,{0x82,0x22,0x44,0x45,0x53,0x54,0x00,0x00}};

ICQFilter::ICQFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags)
{
	m_pSession->getSessionInfo(&m_si);
	m_state = STATE_NONE;
	setActive(false);
}

ICQFilter::~ICQFilter(void)
{
}

eFilterResult ICQFilter::updateCmdBuffer(const char * buf, int len)
{
	if ((m_cmdBuffer.size() + len) > MAX_CMD_BUF_LEN)
		return FR_DELETE_ME;

	m_cmdBuffer.append(buf, len);	

	OSCAR_FLAP * pFlap = (OSCAR_FLAP *)m_cmdBuffer.buffer();

	for (;;) 
	{
		if (pFlap->id != 0x2a)
			return FR_DELETE_ME;

		if (m_cmdBuffer.size() < sizeof(OSCAR_FLAP)-1 ||
			m_cmdBuffer.size() < (ntohs(pFlap->len) + sizeof(OSCAR_FLAP) - 1))
		{
			return FR_PASSTHROUGH;
		}

		pFlap = (OSCAR_FLAP *)(pFlap->data + ntohs(pFlap->len));
		
		if ((unsigned long)((char*)pFlap - m_cmdBuffer.buffer()) >= m_cmdBuffer.size())
			break;
	}

	return FR_DATA_EATEN;
}

eFilterResult ICQFilter::updateResBuffer(const char * buf, int len)
{
	if ((m_resBuffer.size() + len) > MAX_RES_BUF_LEN)
		return FR_DELETE_ME;

	m_resBuffer.append(buf, len);	

	OSCAR_FLAP * pFlap = (OSCAR_FLAP *)m_resBuffer.buffer();

	for (;;) 
	{
		if (pFlap->id != 0x2a)
			return FR_DATA_EATEN;

		if (m_resBuffer.size() < sizeof(OSCAR_FLAP)-1 ||
			m_resBuffer.size() < (ntohs(pFlap->len) + sizeof(OSCAR_FLAP) - 1))
		{
			return FR_PASSTHROUGH;
		}

		pFlap = (OSCAR_FLAP *)(pFlap->data + ntohs(pFlap->len));
		
		if ((unsigned long)((char*)pFlap - m_resBuffer.buffer()) >= m_resBuffer.size())
			break;
	}

	return FR_DATA_EATEN;
}

inline OSCAR_TLV * getTLV(char * p, char * pEnd)
{
	if (!p || p >= pEnd)
		return NULL;

	size_t len = pEnd - p;

	if (len < (sizeof(OSCAR_TLV) - 1))
		return NULL;

	return (OSCAR_TLV*)p;
}

inline OSCAR_TLV * nextTLV(OSCAR_TLV * p, char * pEnd)
{
	if (!p || (char*)p >= pEnd)
		return NULL;

	size_t len = pEnd - (char*)p;

	if (len < (sizeof(OSCAR_TLV) - 1))
		return NULL;

	return getTLV(p->data + ntohs(p->len), pEnd);
}

eFilterResult ICQFilter::filterOutgoingMessage(char * p, char * pFlap, char * pEnd, PFEvents * pHandler)
{
	eFilterResult res = FR_PASSTHROUGH;

	if (!p || (p >= pEnd) || (pFlap >= pEnd))
		return FR_PASSTHROUGH;

	OSCAR_CHAT_MESSAGE_HDR * pHdr = (OSCAR_CHAT_MESSAGE_HDR *)p;
	
	p += sizeof(OSCAR_CHAT_MESSAGE_HDR);
	if (p >= pEnd)
		return FR_PASSTHROUGH;

	unsigned char nameLen = *p;
	if ((p + 1 + nameLen) >= pEnd)
		return FR_PASSTHROUGH;
	
	std::string toUin(p + 1, nameLen);

	p = p + 1 + nameLen;

	OSCAR_TLV * ptlv = getTLV(p, pEnd);

	if (!ptlv)
		return FR_PASSTHROUGH;

	PFObjectImpl obj(OT_ICQ_CHAT_MESSAGE_OUTGOING, ICQS_MAX);

	if (m_flags & FF_READ_ONLY_OUT)
	{
		obj.setReadOnly(true);
	}

	PFStream * pStream = obj.getStream(ICQS_RAW);
	if (pStream)
	{
		pStream->reset();
		pStream->write(pFlap, (tStreamSize)(pEnd - pFlap));
	}

	pStream = obj.getStream(ICQS_USER_UIN);
	if (pStream)
	{
		pStream->reset();
		pStream->write(m_user.c_str(), (tStreamSize)m_user.length());
	}

	pStream = obj.getStream(ICQS_CONTACT_UIN);
	if (pStream)
	{
		pStream->reset();
		pStream->write(toUin.c_str(), (tStreamSize)toUin.length());
	}

	switch (ntohs(pHdr->msgChannel))
	{
	case 1: // Channel 1
		if (ptlv->type == ntohs(0x6))
		{
			ptlv = nextTLV(ptlv, pEnd);
			if (!ptlv)
				break;
		}

		if (ptlv->type == ntohs(0x2))
		{
			ptlv = getTLV(ptlv->data, pEnd);
			if (!ptlv)
				break;

			ptlv = nextTLV(ptlv, pEnd);
			if (!ptlv)
				break;

			if (ntohs(ptlv->len) <= sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR))
				break;

			OSCAR_CHAT_MESSAGE_TEXT_HDR * pTextHdr = (OSCAR_CHAT_MESSAGE_TEXT_HDR*)ptlv->data;
			
			if (ntohs(pTextHdr->charsetId) == 2)
			{
				// Unicode
				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					int t = ICQTF_UNICODE;
					pStream->write(&t, sizeof(t));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					pStream->reset();

					int textLen = (ntohs(ptlv->len) - sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR))/sizeof(wchar_t);
					wchar_t * pChar = (wchar_t *)(ptlv->data + sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR));
					wchar_t c;
					for (int i=0; i<textLen; i++, pChar++)
					{
						c = ntohs(*pChar);
						pStream->write(&c, sizeof(c));
					}
				}
			} else
			{
				// Single byte encoding
				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					int t = ICQTF_ANSI;
					pStream->write(&t, sizeof(t));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					pStream->reset();

					int textLen = (ntohs(ptlv->len) - sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR));
					char * pChar = (ptlv->data + sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR));
					char c;
					for (int i=0; i<textLen; i++, pChar++)
					{
						c = *pChar;
						pStream->write(&c, sizeof(c));
					}
				}
			}

			res = FR_DATA_EATEN;
		}
		break;

	case 2:  // Channel 2
		if (ptlv->type == ntohs(0x5))
		{
			if (ntohs(ptlv->len) <= sizeof(OSCAR_CHAT_MESSAGE_CH2_HDR))
				break;

			OSCAR_CHAT_MESSAGE_CH2_HDR * pCh2Hdr = (OSCAR_CHAT_MESSAGE_CH2_HDR*)ptlv->data;

			ptlv = getTLV(ptlv->data + sizeof(OSCAR_CHAT_MESSAGE_CH2_HDR), pEnd);
			while (ptlv)
			{
				if (ptlv->type == 0x1127)
					break;
				ptlv = nextTLV(ptlv, pEnd);
			}

			if (!ptlv)
				break;

			if (IsEqualGUID(pCh2Hdr->capabilityGuid, GUID_AIM_FileTransfer))
			{
				// This is a file transfer request
				OSCAR_FILE_TRANSFER_HDR * pFTHdr = (OSCAR_FILE_TRANSFER_HDR*)ptlv->data;

				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					int t = ICQTF_FILE_TRANSFER;
					pStream->write(&t, sizeof(t));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					pStream->reset();

					std::vector<char> sBuf(_MAX_PATH*2);

					_snprintf(&sBuf[0], sBuf.size(), ICQ_FILE_COUNT ": %d\r\n", ntohs(pFTHdr->fileCount));
					pStream->write(&sBuf[0], (tStreamSize)strlen(&sBuf[0]));

					_snprintf(&sBuf[0], sBuf.size(), ICQ_TOTAL_BYTES ": %d\r\n", ntohl(pFTHdr->totalBytes));
					pStream->write(&sBuf[0], (tStreamSize)strlen(&sBuf[0]));

					_snprintf(&sBuf[0], sBuf.size(), ICQ_FILE_NAME ": %s\r\n", pFTHdr->fileName);
					pStream->write(&sBuf[0], (tStreamSize)strlen(&sBuf[0]));

					pStream->write("\r\n", 2);
				}

				res = FR_DATA_EATEN;

				break;
			}

			OSCAR_CHAT_MESSAGE_CH2_EXT_HDR * pCh2ExtHdr = (OSCAR_CHAT_MESSAGE_CH2_EXT_HDR*)ptlv->data;

			if ((ptlv->data + pCh2ExtHdr->len + 2) >= pEnd)
				break;

			bool zerosInPluginGuid = true;
			for (int i=0; i<(int)sizeof(pCh2ExtHdr->pluginGuid)/sizeof(pCh2ExtHdr->pluginGuid[0]); i++)
			{
				if (pCh2ExtHdr->pluginGuid[i] != 0)
				{
					zerosInPluginGuid = false;
					break;
				}
			}

			if (!zerosInPluginGuid)
				break;

			char * p = ptlv->data + pCh2ExtHdr->len + 2;

			if ((p + *(unsigned short*)p + 2) >= pEnd)
				break;

			p = p + *(unsigned short*)p + 2;

			OSCAR_CHAT_MESSAGE_CH2_TEXT * pCh2Text = (OSCAR_CHAT_MESSAGE_CH2_TEXT*)p;

			if ((p + sizeof(OSCAR_CHAT_MESSAGE_CH2_TEXT)) >= pEnd)
				break;
			
			p = p + sizeof(OSCAR_CHAT_MESSAGE_CH2_TEXT);

			pStream = obj.getStream(ICQS_TEXT_FORMAT);
			if (pStream)
			{
				pStream->reset();

				int t = ICQTF_ANSI;

				// Check text extension guid if it exists
				if (pCh2Text->msgType == 1 &&
					(htons(ptlv->len) - (((p - ptlv->data) + pCh2Text->len))) >= (int)sizeof(OSCAR_CHAT_MESSAGE_CH2_FORMAT))
				{
					OSCAR_CHAT_MESSAGE_CH2_FORMAT * pf = (OSCAR_CHAT_MESSAGE_CH2_FORMAT *)(p + pCh2Text->len);
					std::string guid(pf->szGuid, pf->guidLen);
					if (guid == SZ_GUID_AIM_SUPPORTS_UTF8)
					{
						t = ICQTF_UTF8;
					}
				}

				pStream->write(&t, sizeof(t));
			}

			pStream = obj.getStream(ICQS_TEXT);
			if (pStream)
			{
				pStream->reset();

				// Single byte encoding
				int textLen = pCh2Text->len;
				char * pChar = p;
				char c;
				for (int i=0; i<textLen; i++, pChar++)
				{
					c = *pChar;
					pStream->write(&c, sizeof(c));
				}
			}

			res = FR_DATA_EATEN;
		}
		break;
	
	case 4: // Channel 4
/*
		if (ptlv->type == ntohs(0x5))
		{
			if (ntohs(ptlv->len) <= sizeof(OSCAR_CHAT_MESSAGE_CH2_HDR))
				return FR_PASSTHROUGH;

			OSCAR_CHAT_MESSAGE_CH4_HDR * pCh4Hdr = (OSCAR_CHAT_MESSAGE_CH4_HDR*)ptlv->data;

			if ((p + sizeof(OSCAR_CHAT_MESSAGE_CH4_HDR)) >= pEnd)
				return FR_PASSTHROUGH;
			
			p = p + sizeof(OSCAR_CHAT_MESSAGE_CH4_HDR);

			{
				PFObjectImpl obj(OT_ICQ_CHAT_MESSAGE_OUTGOING, ICQS_MAX);

				if (m_flags & FF_READ_ONLY_OUT)
				{
					obj.setReadOnly(true);
				}

				PFStream * pStream = obj.getStream(ICQS_RAW);
				if (pStream)
				{
					pStream->reset();
					pStream->write(m_cmdBuffer.buffer(), m_cmdBuffer.size());
				}

				pStream = obj.getStream(ICQS_USER_UIN);
				if (pStream)
				{
					pStream->reset();
					pStream->write(m_user.c_str(), (tStreamSize)m_user.length());
				}

				pStream = obj.getStream(ICQS_CONTACT_UIN);
				if (pStream)
				{
					pStream->reset();
					pStream->write(toUin.c_str(), (tStreamSize)toUin.length());
				}

				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					pStream->write(pCh4Hdr, sizeof(OSCAR_CHAT_MESSAGE_CH4_HDR));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					pStream->reset();

					// Single byte encoding
					int textLen = pCh4Hdr->len;
					char * pChar = p;
					char c;
					for (int i=0; i<textLen; i++, pChar++)
					{
						c = *pChar;
						pStream->write(&c, sizeof(c));
					}
				}

				try {
					pHandler->dataAvailable(m_pSession->getId(), &obj);
				} catch (...)
				{
				}

				return FR_DATA_EATEN;
			}
		}
*/		break;
	}

	if (res == FR_DATA_EATEN)
	{
		try {
			pHandler->dataAvailable(m_pSession->getId(), &obj);
		} catch (...)
		{
		}
	}

	return res;
}

eFilterResult ICQFilter::filterIncomingMessage(char * p, char * pFlap, char * pEnd, PFEvents * pHandler)
{
	eFilterResult res = FR_PASSTHROUGH;

	if (!p || (p >= pEnd) || (pFlap >= pEnd))
		return FR_PASSTHROUGH;

	OSCAR_CHAT_MESSAGE_HDR * pHdr = (OSCAR_CHAT_MESSAGE_HDR *)p;
	
	p += sizeof(OSCAR_CHAT_MESSAGE_HDR);
	if (p >= pEnd)
		return FR_PASSTHROUGH;

	unsigned char nameLen = *p;
	if ((p + 1 + nameLen) >= pEnd)
		return FR_PASSTHROUGH;
	
	std::string fromUin(p + 1, nameLen);

	p = p + 1 + nameLen;

	OSCAR_CHAT_IN_MESSAGE_HDR * pInHdr = (OSCAR_CHAT_IN_MESSAGE_HDR*) p;
	p += sizeof(OSCAR_CHAT_IN_MESSAGE_HDR);
	if (p >= pEnd)
		return FR_PASSTHROUGH;

	OSCAR_TLV * ptlv = getTLV(p, pEnd);

	if (!ptlv)
		return FR_PASSTHROUGH;

	// Skip fixed part TLVs
	for (int i=0; i<ntohs(pInHdr->nTlvs); i++)
	{
		ptlv = nextTLV(ptlv, pEnd);
		if (!ptlv)
			return FR_PASSTHROUGH;
	}

	PFObjectImpl obj(OT_ICQ_CHAT_MESSAGE_INCOMING, ICQS_MAX);

	if (m_flags & FF_READ_ONLY_IN)
	{
		obj.setReadOnly(true);
	}

	PFStream * pStream = obj.getStream(ICQS_RAW);
	if (pStream)
	{
		pStream->reset();
		pStream->write(pFlap, (tStreamSize)(pEnd - pFlap));
	}

	pStream = obj.getStream(ICQS_USER_UIN);
	if (pStream)
	{
		pStream->reset();
		pStream->write(m_user.c_str(), (tStreamSize)m_user.length());
	}

	pStream = obj.getStream(ICQS_CONTACT_UIN);
	if (pStream)
	{
		pStream->reset();
		pStream->write(fromUin.c_str(), (tStreamSize)fromUin.length());
	}

	switch (ntohs(pHdr->msgChannel))
	{
	case 1: // Channel 1
		if (ptlv->type == ntohs(0x2))
		{
			ptlv = getTLV(ptlv->data, pEnd);
			if (!ptlv)
				break;

			ptlv = nextTLV(ptlv, pEnd);
			if (!ptlv)
				break;

			if (ntohs(ptlv->len) <= sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR))
				break;

			OSCAR_CHAT_MESSAGE_TEXT_HDR * pTextHdr = (OSCAR_CHAT_MESSAGE_TEXT_HDR*)ptlv->data;
			
			if (ntohs(pTextHdr->charsetId) == 2)
			{
				// Unicode
				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					int t = ICQTF_UNICODE;
					pStream->write(&t, sizeof(t));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					int textLen = (ntohs(ptlv->len) - sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR))/sizeof(wchar_t);
					wchar_t * pChar = (wchar_t *)(ptlv->data + sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR));
					wchar_t c;
					for (int i=0; i<textLen; i++, pChar++)
					{
						c = ntohs(*pChar);
						pStream->write(&c, sizeof(c));
					}
				}
			} else
			{
				// Single byte encoding

				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					int t = ICQTF_ANSI;
					pStream->write(&t, sizeof(t));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					int textLen = (ntohs(ptlv->len) - sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR));
					char * pChar = (ptlv->data + sizeof(OSCAR_CHAT_MESSAGE_TEXT_HDR));
					char c;
					for (int i=0; i<textLen; i++, pChar++)
					{
						c = *pChar;
						pStream->write(&c, sizeof(c));
					}
				}
			}

			res = FR_DATA_EATEN;
		}
		break;

	case 2:  // Channel 2
		if (ptlv->type == ntohs(0x5))
		{
			if (ntohs(ptlv->len) <= sizeof(OSCAR_CHAT_MESSAGE_CH2_HDR))
				break;

			OSCAR_CHAT_MESSAGE_CH2_HDR * pCh2Hdr = (OSCAR_CHAT_MESSAGE_CH2_HDR*)ptlv->data;

			ptlv = getTLV(ptlv->data + sizeof(OSCAR_CHAT_MESSAGE_CH2_HDR), pEnd);
			while (ptlv)
			{
				if (ptlv->type == 0x1127)
					break;
				ptlv = nextTLV(ptlv, pEnd);
			}

			if (!ptlv)
				break;

			if (IsEqualGUID(pCh2Hdr->capabilityGuid, GUID_AIM_FileTransfer))
			{
				// This is a file transfer request
				OSCAR_FILE_TRANSFER_HDR * pFTHdr = (OSCAR_FILE_TRANSFER_HDR*)ptlv->data;

				pStream = obj.getStream(ICQS_TEXT_FORMAT);
				if (pStream)
				{
					pStream->reset();
					int t = ICQTF_FILE_TRANSFER;
					pStream->write(&t, sizeof(t));
				}

				pStream = obj.getStream(ICQS_TEXT);
				if (pStream)
				{
					pStream->reset();

					std::vector<char> sBuf(_MAX_PATH*2);

					_snprintf(&sBuf[0], sBuf.size(), ICQ_FILE_COUNT ": %d\r\n", ntohs(pFTHdr->fileCount));
					pStream->write(&sBuf[0], (tStreamSize)strlen(&sBuf[0]));

					_snprintf(&sBuf[0], sBuf.size(), ICQ_TOTAL_BYTES ": %d\r\n", ntohl(pFTHdr->totalBytes));
					pStream->write(&sBuf[0], (tStreamSize)strlen(&sBuf[0]));

					_snprintf(&sBuf[0], sBuf.size(), ICQ_FILE_NAME ": %s\r\n", pFTHdr->fileName);
					pStream->write(&sBuf[0], (tStreamSize)strlen(&sBuf[0]));

					pStream->write("\r\n", 2);
				}

				res = FR_DATA_EATEN;

				break;
			}

			OSCAR_CHAT_MESSAGE_CH2_EXT_HDR * pCh2ExtHdr = (OSCAR_CHAT_MESSAGE_CH2_EXT_HDR*)ptlv->data;

			if ((ptlv->data + pCh2ExtHdr->len + 2) >= pEnd)
				break;

			bool zerosInPluginGuid = true;
			for (int i=0; i<(int)sizeof(pCh2ExtHdr->pluginGuid)/sizeof(pCh2ExtHdr->pluginGuid[0]); i++)
			{
				if (pCh2ExtHdr->pluginGuid[i] != 0)
				{
					zerosInPluginGuid = false;
					break;
				}
			}

			if (!zerosInPluginGuid)
				break;

			char * p = ptlv->data + pCh2ExtHdr->len + 2;

			if ((p + *(unsigned short*)p + 2) >= pEnd)
				break;

			p = p + *(unsigned short*)p + 2;

			OSCAR_CHAT_MESSAGE_CH2_TEXT * pCh2Text = (OSCAR_CHAT_MESSAGE_CH2_TEXT*)p;

			if ((p + sizeof(OSCAR_CHAT_MESSAGE_CH2_TEXT)) >= pEnd)
				break;
			
			p = p + sizeof(OSCAR_CHAT_MESSAGE_CH2_TEXT);

			pStream = obj.getStream(ICQS_TEXT_FORMAT);
			if (pStream)
			{
				pStream->reset();

				int t = ICQTF_ANSI;

				// Check text extension guid if it exists
				if (pCh2Text->msgType == 1 &&
					(htons(ptlv->len) - (((p - ptlv->data) + pCh2Text->len))) >= (int)sizeof(OSCAR_CHAT_MESSAGE_CH2_FORMAT))
				{
					OSCAR_CHAT_MESSAGE_CH2_FORMAT * pf = (OSCAR_CHAT_MESSAGE_CH2_FORMAT *)(p + pCh2Text->len);
					std::string guid(pf->szGuid, pf->guidLen);
					if (guid == SZ_GUID_AIM_SUPPORTS_UTF8)
					{
						t = ICQTF_UTF8;
					}
				}

				pStream->write(&t, sizeof(t));
			}

			pStream = obj.getStream(ICQS_TEXT);
			if (pStream)
			{
				// Single byte encoding
				int textLen = pCh2Text->len;
				char * pChar = p;
				char c;
				for (int i=0; i<textLen; i++, pChar++)
				{
					c = *pChar;
					pStream->write(&c, sizeof(c));
				}
			}
	
			res = FR_DATA_EATEN;
		}
		break;

	case 4:
		break;
	}
	
	if (res == FR_DATA_EATEN)
	{
		try {
			pHandler->dataAvailable(m_pSession->getId(), &obj);
		} catch (...)
		{
		}
	}

	return res;
}

eFilterResult ICQFilter::filterIncomingOfflineMessage(char * p, char * pFlap, char * pEnd, PFEvents * pHandler)
{
	eFilterResult res = FR_PASSTHROUGH;

	if (!p || (p >= pEnd) || (pFlap >= pEnd))
		return FR_PASSTHROUGH;

	OSCAR_TLV * ptlv = getTLV(p, pEnd);

	if (!ptlv)
		return FR_PASSTHROUGH;

	if (ptlv->data[6] != 0x41 ||
		ptlv->data[7] != 0 ||
		ptlv->len < 25)
		return FR_PASSTHROUGH;

	std::string fromUin(12, ' ');
	unsigned int nFromUin = *(unsigned int*)(ptlv->data+10);
	_snprintf((char*)fromUin.c_str(), fromUin.size(), "%u", nFromUin);
	
	PFObjectImpl obj(OT_ICQ_CHAT_MESSAGE_INCOMING, ICQS_MAX);

	if (m_flags & FF_READ_ONLY_IN)
	{
		obj.setReadOnly(true);
	}

	PFStream * pStream = obj.getStream(ICQS_RAW);
	if (pStream)
	{
		pStream->reset();
		pStream->write(pFlap, (tStreamSize)(pEnd - pFlap));
	}

	pStream = obj.getStream(ICQS_USER_UIN);
	if (pStream)
	{
		pStream->reset();
		pStream->write(m_user.c_str(), (tStreamSize)m_user.length());
	}

	pStream = obj.getStream(ICQS_CONTACT_UIN);
	if (pStream)
	{
		pStream->reset();
		pStream->write(fromUin.c_str(), (tStreamSize)fromUin.length());
	}

	pStream = obj.getStream(ICQS_TEXT_FORMAT);
	if (pStream)
	{
		pStream->reset();
		int t = ICQTF_ANSI;
		pStream->write(&t, sizeof(t));
	}

	pStream = obj.getStream(ICQS_TEXT);
	if (pStream)
	{
		// Single byte encoding
		int textLen = *(unsigned short*)(ptlv->data+22);
		char * pChar = ptlv->data+24;
		char c;
		for (int i=0; i<textLen; i++, pChar++)
		{
			c = *pChar;
			pStream->write(&c, sizeof(c));
		}
	}

	res = FR_DATA_EATEN;

	if (res == FR_DATA_EATEN)
	{
		try {
			pHandler->dataAvailable(m_pSession->getId(), &obj);
		} catch (...)
		{
		}
	}

	return res;
}

eFilterResult ICQFilter::filterSNAC(OSCAR_FLAP * pFlap, ePacketDirection pd, PFEvents * pHandler)
{
	if (pFlap->channel != FC_SNAC)
	{
		return FR_PASSTHROUGH;
	}

	char * pEnd = (char *)pFlap + (ntohs(pFlap->len) + sizeof(OSCAR_FLAP) - 1);
	
	OSCAR_SNAC * pSnac = (OSCAR_SNAC *)pFlap->data;

	if (pd == PD_SEND)
	{
		// Outgoing chat message
		if (pSnac->familyId == ntohs(0x4) &&
			pSnac->subtypeId == ntohs(0x6))
		{
			return filterOutgoingMessage(pSnac->data, (char*)pFlap, pEnd, pHandler);
		} else
		// Login request
		if (pSnac->familyId == ntohs(0x17) &&
			pSnac->subtypeId == ntohs(0x6))
		{
			OSCAR_TLV * ptlv = getTLV(pSnac->data, pEnd);

			if (!ptlv)
				return FR_PASSTHROUGH;

			if (ptlv->type == ntohs(1))
			{
				m_user = std::string(ptlv->data, ntohs(ptlv->len));

				PFObjectImpl obj(OT_ICQ_LOGIN, 2);

				if (m_flags & FF_READ_ONLY_OUT)
				{
					obj.setReadOnly(true);
				}

				PFStream * pStream = obj.getStream(ICQS_RAW);
				if (pStream)
				{
					pStream->reset();
					pStream->write(pFlap, (tStreamSize)(pEnd - (char*)pFlap));
				}

				pStream = obj.getStream(ICQS_USER_UIN);
				if (pStream)
				{
					pStream->reset();
					pStream->write(m_user.c_str(), (tStreamSize)m_user.length());
				}

				try {
					pHandler->dataAvailable(m_pSession->getId(), &obj);
				} catch (...)
				{
				}

				return FR_DATA_EATEN;
			}
		}
	} else
	if (pd == PD_RECEIVE)
	{
		// Status response with user UIN
		if (pSnac->familyId == ntohs(0x1) &&
			pSnac->subtypeId == ntohs(0xf))
		{
			int textLen = *(pSnac->data);

			if (textLen == 0 ||
				(pSnac->data + 1 + textLen) >= pEnd)
				return FR_PASSTHROUGH;

			m_user = std::string(pSnac->data+1, textLen);
		} else
		// Incoming chat message
		if (pSnac->familyId == ntohs(0x4) &&
			pSnac->subtypeId == ntohs(0x7))
		{
			return filterIncomingMessage(pSnac->data, (char*)pFlap, pEnd, pHandler);
		} else
		// Incoming offline message
		if (pSnac->familyId == ntohs(0x15) &&
			pSnac->subtypeId == ntohs(0x3))
		{
			return filterIncomingOfflineMessage(pSnac->data, (char*)pFlap, pEnd, pHandler);
		} 
	}

	return FR_PASSTHROUGH;
}

eFilterResult ICQFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	eFilterResult res = FR_DELETE_ME;

	DbgPrint("ICQFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);

	if (m_si.tcpConnInfo.direction != NFAPI_NS NF_D_OUT)
		return FR_DELETE_ME;

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (!pHandler)
		return FR_DELETE_ME;

	if (len > 0)
	{
		switch (m_state)
		{
		case STATE_NONE:
			{
				if (pd == PD_RECEIVE)
				{
					res = updateResBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;
					if (m_resBuffer.buffer()[0] != 0x2a)
					{
						return FR_DELETE_ME;
					}
					m_resBuffer.reset();
					res = FR_PASSTHROUGH;
					m_state = STATE_SESSION;

					setActive(true);
				} 
				
				break;
			}

		case STATE_SESSION:
			{
				if (pd == PD_SEND)
				{
					res = updateCmdBuffer(buf, len);
					if (res != FR_DATA_EATEN)
					{
						if (res == FR_PASSTHROUGH)
						{
							return (m_flags & FF_READ_ONLY_OUT)? res : FR_DATA_EATEN;
						}
						return res;
					}

					OSCAR_FLAP * pFlap = (OSCAR_FLAP *)m_cmdBuffer.buffer();
					OSCAR_FLAP * pFlapNext = NULL;

					for (;;)
					{	
						if (pFlap->id != 0x2a)
						{
							res = FR_PASSTHROUGH;
						} else
						if (pFlap->channel == FC_SNAC)
						{
							res = filterSNAC(pFlap, pd, pHandler);
						} else
						{
							res = FR_PASSTHROUGH;
						}

						if (res == FR_PASSTHROUGH)
						{
							PFObjectImpl obj(OT_ICQ_REQUEST, 1);

							if (m_flags & FF_READ_ONLY_OUT)
							{
								obj.setReadOnly(true);
							}

							PFStream * pStream = obj.getStream(ICQS_RAW);
							if (pStream)
							{
								pStream->reset();
								
								unsigned long flapLen = (pFlap->id != 0x2a)? 
									m_cmdBuffer.size() - (unsigned long)((char*)pFlap - m_cmdBuffer.buffer()) :
									sizeof(OSCAR_FLAP) - 1 + ntohs(pFlap->len);
									
								pStream->write(pFlap, flapLen);
							}

							try {
								pHandler->dataAvailable(m_pSession->getId(), &obj);
							} catch (...)
							{
							}
						}

						if (pFlap->id != 0x2a)
						{
							m_cmdBuffer.reset();
							break;
						}

						pFlap = (OSCAR_FLAP *)(pFlap->data + ntohs(pFlap->len));
						if ((unsigned long)((char*)pFlap - m_cmdBuffer.buffer()) >= m_cmdBuffer.size())
						{
							m_cmdBuffer.reset();
							break;
						}

						pFlapNext = (OSCAR_FLAP *)(pFlap->data + ntohs(pFlap->len));
						unsigned long offset = (unsigned long)((char*)pFlapNext - m_cmdBuffer.buffer());
						
						if (offset > m_cmdBuffer.size())
						{
							offset = m_cmdBuffer.size() - (unsigned long)((char*)pFlap - m_cmdBuffer.buffer());
							memmove(m_cmdBuffer.buffer(), pFlap, offset);
							m_cmdBuffer.resize(offset);
							break;
						}
					}

					if (m_flags & FF_READ_ONLY_OUT)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);
					}

					return FR_DATA_EATEN;
				} else
				{
					res = updateResBuffer(buf, len);
					if (res != FR_DATA_EATEN)
					{
						if (res == FR_PASSTHROUGH)
						{
							return (m_flags & FF_READ_ONLY_IN)? res : FR_DATA_EATEN;
						}
						return res;
					}

					OSCAR_FLAP * pFlap = (OSCAR_FLAP *)m_resBuffer.buffer();
					OSCAR_FLAP * pFlapNext = NULL;
					
					for (;;)
					{
						if (pFlap->id != 0x2a)
						{
							res = FR_PASSTHROUGH;
						} else
						if (pFlap->channel == FC_SNAC)
						{
							res = filterSNAC(pFlap, pd, pHandler);
						} else
						{
							res = FR_PASSTHROUGH;
						}

						if (res == FR_PASSTHROUGH)
						{
							PFObjectImpl obj(OT_ICQ_RESPONSE, 1);

							if (m_flags & FF_READ_ONLY_IN)
							{
								obj.setReadOnly(true);
							}

							PFStream * pStream = obj.getStream(ICQS_RAW);
							if (pStream)
							{
								pStream->reset();
								
								unsigned long flapLen = (pFlap->id != 0x2a)? 
									m_resBuffer.size() - (unsigned long)((char*)pFlap - m_resBuffer.buffer()) :
									sizeof(OSCAR_FLAP) - 1 + ntohs(pFlap->len);

								pStream->write(pFlap, flapLen);
							}

							try {
								pHandler->dataAvailable(m_pSession->getId(), &obj);
							} catch (...)
							{
							}
						}

						if (pFlap->id != 0x2a)
						{
							m_resBuffer.reset();
							break;
						}

						pFlap = (OSCAR_FLAP *)(pFlap->data + ntohs(pFlap->len));
						if ((unsigned long)((char*)pFlap - m_resBuffer.buffer()) >= m_resBuffer.size())
						{
							m_resBuffer.reset();
							break;
						}

						pFlapNext = (OSCAR_FLAP *)(pFlap->data + ntohs(pFlap->len));
						unsigned long offset = (unsigned long)((char*)pFlapNext - m_resBuffer.buffer());
						
						if (offset > m_resBuffer.size())
						{
							offset = m_resBuffer.size() - (unsigned long)((char*)pFlap - m_resBuffer.buffer());
							memmove(m_resBuffer.buffer(), pFlap, offset);
							m_resBuffer.resize(offset);
							break;
						}
					}

					if (m_flags & FF_READ_ONLY_IN)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);
					}

					return FR_DATA_EATEN;
				}
				
				break;
			}
		}		

		return res;
	}

	if (len == 0)
	{
		if (pHandler)
		{
			PFObjectImpl obj((pd == PD_SEND)? OT_TCP_DISCONNECT_LOCAL : OT_TCP_DISCONNECT_REMOTE, 0);
			try {
				pHandler->dataAvailable(m_pSession->getId(), &obj);
			} catch (...)
			{
			}
			return FR_DATA_EATEN;
		}
	} 

	return FR_PASSTHROUGH;
}
		
eFilterResult ICQFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType ICQFilter::getType()
{
	return FT_ICQ;
}

PF_FilterCategory ICQFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool ICQFilter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	switch (object.getType())
	{
	case OT_ICQ_LOGIN:
	case OT_ICQ_CHAT_MESSAGE_OUTGOING:
	case OT_ICQ_REQUEST:
		pd = PD_SEND;
		break;
	case OT_ICQ_RESPONSE:
	case OT_ICQ_CHAT_MESSAGE_INCOMING:
		pd = PD_RECEIVE;
		break;
	default:
		return false;
	}

	m_pSession->tcpPostStream(this, pd, object.getStream(0));

	return true;
}

