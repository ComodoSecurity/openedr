//
// 	NetFilterSDK 
// 	Copyright (C) 2014 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#pragma once

#include "threadpool.h"

namespace nfapi
{
	enum eEndpointType
	{
		ET_UNKNOWN,
		ET_TCP,
		ET_UDP
	};

	inline eEndpointType getEndpointType(int code)
	{
		switch (code)
		{
		case NF_TCP_CONNECTED:
		case NF_TCP_CONNECT_REQUEST:
		case NF_TCP_CLOSED:
		case NF_TCP_RECEIVE:
		case NF_TCP_SEND:
		case NF_TCP_CAN_RECEIVE:
		case NF_TCP_CAN_SEND:
		case NF_TCP_REINJECT:
		case NF_TCP_RECEIVE_PUSH:
			return ET_TCP;

		case NF_UDP_CREATED:
		case NF_UDP_CONNECT_REQUEST:
		case NF_UDP_CLOSED:
		case NF_UDP_RECEIVE:
		case NF_UDP_SEND:
		case NF_UDP_CAN_RECEIVE:
		case NF_UDP_CAN_SEND:
			return ET_UDP;
		}

		return ET_UNKNOWN;
	}

	inline int getEventFlag(int code)
	{
		return 1 << code;
	}

	inline bool isEventFlagEnabled(int flags, int code)
	{
		return (flags & (1 << code)) != 0;
	}
	
	bool nf_pushInEvent(ENDPOINT_ID id, int code);
	bool nf_pushOutEvent(ENDPOINT_ID id, int code);

	class NFEvent : public ThreadJob
	{
	public:

		NFEvent(eEndpointType et, ENDPOINT_ID id, int flags) 
		{
			m_et = et;
			m_id = id;
			m_flags = flags;
		}
		
		~NFEvent()
		{
		}

		virtual void execute()
		{
			if (m_et == ET_TCP)
			{
				PCONNOBJ pConn = NULL;
				PHASH_TABLE_ENTRY phte;
				PNF_DATA_ENTRY pDataEntry;
				
				AutoLock lock(g_cs);
				phte = ht_find_entry(g_phtConnById, m_id);
				if (!phte)
				{
					return;
				}
				
				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

				if (pConn->closed)
					return;

				if (isEventFlagEnabled(m_flags, NF_TCP_CONNECTED))
				{
					AutoUnlock unlock(g_cs);
					g_pEventHandler->tcpConnected(m_id, &pConn->connInfo);
				}

				if (isEventFlagEnabled(m_flags, NF_TCP_CONNECT_REQUEST))
				{
					{
						AutoUnlock unlock(g_cs);
						g_pEventHandler->tcpConnectRequest(m_id, &pConn->connInfo);
					}

					if (pConn->connInfo.filteringFlag & NF_PEND_CONNECT_REQUEST)
					{
						return;
					}

					PNF_DATA pDataCopy = (PNF_DATA)mp_alloc(sizeof(NF_DATA) - 1 + sizeof(pConn->connInfo));
					if (!pDataCopy)
					{
						return;
					}

					pDataCopy->id = m_id;
					pDataCopy->code = NF_TCP_CONNECT_REQUEST;
					pDataCopy->bufferSize = sizeof(pConn->connInfo);
					memcpy(pDataCopy->buffer, &pConn->connInfo, sizeof(pConn->connInfo));
					
					nf_postData(pDataCopy);

					mp_free(pDataCopy);

					if (pConn->connInfo.filteringFlag & NF_BLOCK)
					{
						nf_tcpClose(m_id);
						connobj_free(pConn);
						return;
					} else
					if (!(pConn->connInfo.filteringFlag & NF_FILTER))
					{
						connobj_free(pConn);
						return;
					}
				}

				{
					unsigned int receivesInBytes = pConn->receivesInBytes;

					while (!IsListEmpty(&pConn->receivesIn))
					{
						if (g_useNagle)
						if (!isEventFlagEnabled(m_flags, NF_TCP_RECEIVE_PUSH))
						{
							if (pConn->receivesIn.Flink == pConn->receivesIn.Blink)
							{
								g_timers.addTimer(pConn->id, 1);
								break;
							}
						}

						pDataEntry = (PNF_DATA_ENTRY)pConn->receivesIn.Flink;

						if (receivesInBytes == 0 && pDataEntry->data.bufferSize > 0)
							break;

						RemoveEntryList(&pDataEntry->entry);

						if (pConn->receivesInBytes >= pDataEntry->data.bufferSize)
						{
							pConn->receivesInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							pConn->receivesInBytes = 0;
						}

						if (receivesInBytes >= pDataEntry->data.bufferSize)
						{
							receivesInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							receivesInBytes = 0;
						}

//						if (!pConn->disconnectedIn)
						{
							if (pConn->disableFiltering)
							{
								AutoUnlock unlock(g_cs);
								nf_tcpPostReceive(pDataEntry->data.id, pDataEntry->data.buffer, pDataEntry->data.bufferSize);
							} else
							{
								AutoUnlock unlock(g_cs);
								g_pEventHandler->tcpReceive(pDataEntry->data.id, pDataEntry->data.buffer, pDataEntry->data.bufferSize);
							}
						}

						mp_free(pDataEntry);
					}
				} 

//				if (isEventFlagEnabled(m_flags, NF_TCP_SEND))
				{
					unsigned int sendsInBytes = pConn->sendsInBytes;

					while (!IsListEmpty(&pConn->sendsIn))
					{
						pDataEntry = (PNF_DATA_ENTRY)pConn->sendsIn.Flink;

						if (sendsInBytes == 0 && pDataEntry->data.bufferSize > 0)
							break;

						RemoveEntryList(&pDataEntry->entry);

						if (pConn->sendsInBytes >= pDataEntry->data.bufferSize)
						{
							pConn->sendsInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							pConn->sendsInBytes = 0;
						}

						if (sendsInBytes >= pDataEntry->data.bufferSize)
						{
							sendsInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							sendsInBytes = 0;
						}

//						if (!pConn->disconnectedOut)
						{
							if (pConn->disableFiltering)
							{
								AutoUnlock unlock(g_cs);
								nf_tcpPostSend(pDataEntry->data.id, pDataEntry->data.buffer, pDataEntry->data.bufferSize);
							} else
							{
								AutoUnlock unlock(g_cs);
								g_pEventHandler->tcpSend(pDataEntry->data.id, pDataEntry->data.buffer, pDataEntry->data.bufferSize);
							}
						}

						mp_free(pDataEntry);
					}
				}

				if (isEventFlagEnabled(m_flags, NF_TCP_CAN_RECEIVE))
				{
					if (!pConn->filteringDisabled)
					{
						if ((pConn->receivesOutBytes < NF_TCP_PACKET_BUF_SIZE * 32) && !pConn->canReceiveCalled)
//						if (IsListEmpty(&pConn->receivesOut) && !pConn->canReceiveCalled)
						{
							pConn->canReceiveCalled = TRUE;
							AutoUnlock unlock(g_cs);
							g_pEventHandler->tcpCanReceive(m_id);
						} 
					}
				}

				if (isEventFlagEnabled(m_flags, NF_TCP_CAN_SEND))
				{
					if (!pConn->filteringDisabled)
					{
						if (IsListEmpty(&pConn->sendsOut) && !pConn->canSendCalled)
						{
							pConn->canSendCalled = TRUE;
							AutoUnlock unlock(g_cs);
							g_pEventHandler->tcpCanSend(m_id);
						}
					}
				}

				// Resume the connection 
				if ((pConn->sendsOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pConn->receivesOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pConn->sendsInBytes <= MAX_QUEUE_BYTES) &&
					(pConn->receivesInBytes <= MAX_QUEUE_BYTES))
				{
					nf_tcpSetConnectionStateInternal(pConn, pConn->wantSuspend);
				}

				if (isEventFlagEnabled(m_flags, NF_TCP_CLOSED) && !pConn->closed)
				{
					pConn->closed = TRUE;

					{
						AutoUnlock unlock(g_cs);
						g_pEventHandler->tcpClosed(m_id, &pConn->connInfo);
					}

					nf_pushOutEvent(m_id, NF_TCP_CLOSED);
				}
			} else
			if (m_et == ET_UDP)
			{
				PADDROBJ pAddr = NULL;
				PHASH_TABLE_ENTRY phte;
				PNF_DATA_ENTRY pDataEntry;
				
				AutoLock lock(g_cs);
				phte = ht_find_entry(g_phtAddrById, m_id);
				if (!phte)
				{
					return;
				}
				
				pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

				if (isEventFlagEnabled(m_flags, NF_UDP_CREATED))
				{
					AutoUnlock unlock(g_cs);
					g_pEventHandler->udpCreated(m_id, &pAddr->connInfo);
				}

				if (isEventFlagEnabled(m_flags, NF_UDP_CONNECT_REQUEST))
				{
					{
						AutoUnlock unlock(g_cs);
						g_pEventHandler->udpConnectRequest(m_id, &pAddr->connRequest);
					}

					if (pAddr->connRequest.filteringFlag & NF_PEND_CONNECT_REQUEST)
					{
						return;
					}

					PNF_DATA pDataCopy = (PNF_DATA)mp_alloc(sizeof(NF_DATA) - 1 + sizeof(pAddr->connRequest));
					if (!pDataCopy)
					{
						return;
					}

					pDataCopy->id = m_id;
					pDataCopy->code = NF_UDP_CONNECT_REQUEST;
					pDataCopy->bufferSize = sizeof(pAddr->connRequest);
					memcpy(pDataCopy->buffer, &pAddr->connRequest, sizeof(pAddr->connRequest));
					
					nf_postData(pDataCopy);

					mp_free(pDataCopy);
				}

//				if (isEventFlagEnabled(m_flags, NF_UDP_RECEIVE))
				{
					char * pRemoteAddr; 
					PNF_UDP_OPTIONS pOptions;
					char * pPacketData; 
					unsigned long packetDataLen;
					unsigned int receivesInBytes = pAddr->receivesInBytes;

					while (!IsListEmpty(&pAddr->receivesIn))
					{
						pDataEntry = (PNF_DATA_ENTRY)pAddr->receivesIn.Flink;
						RemoveEntryList(&pDataEntry->entry);

						if (pAddr->receivesInBytes >= pDataEntry->data.bufferSize)
						{
							pAddr->receivesInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							pAddr->receivesInBytes = 0;
						}

						if (receivesInBytes >= pDataEntry->data.bufferSize)
						{
							receivesInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							receivesInBytes = 0;
						}

						pRemoteAddr = pDataEntry->data.buffer; 
						pOptions = (PNF_UDP_OPTIONS)(pDataEntry->data.buffer + NF_MAX_ADDRESS_LENGTH);
						pPacketData = pDataEntry->data.buffer + NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + pOptions->optionsLength; 
						packetDataLen = pDataEntry->data.bufferSize - (unsigned long)(pPacketData - pDataEntry->data.buffer);
				
						if (pAddr->filteringDisabled)
						{
							AutoUnlock unlock(g_cs);
							nf_udpPostReceive(pDataEntry->data.id, 
								(unsigned char*)pRemoteAddr,
								pPacketData, 
								packetDataLen,
								pOptions);
						} else
						{
							AutoUnlock unlock(g_cs);
							g_pEventHandler->udpReceive(pDataEntry->data.id,
								(unsigned char*)pRemoteAddr, 
								pPacketData, 
								packetDataLen,
								pOptions);
						}

						mp_free(pDataEntry);

						if (receivesInBytes == 0)
							break;
					}
				}

//				if (isEventFlagEnabled(m_flags, NF_UDP_SEND))
				{
					char * pRemoteAddr; 
					PNF_UDP_OPTIONS pOptions;
					char * pPacketData; 
					unsigned long packetDataLen;
					unsigned int sendsInBytes = pAddr->sendsInBytes;

					while (!IsListEmpty(&pAddr->sendsIn))
					{
						pDataEntry = (PNF_DATA_ENTRY)pAddr->sendsIn.Flink;
						RemoveEntryList(&pDataEntry->entry);

						if (pAddr->sendsInBytes >= pDataEntry->data.bufferSize)
						{
							pAddr->sendsInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							pAddr->sendsInBytes = 0;
						}

						if (sendsInBytes >= pDataEntry->data.bufferSize)
						{
							sendsInBytes -= pDataEntry->data.bufferSize;
						} else
						{
							sendsInBytes = 0;
						}

						pRemoteAddr = pDataEntry->data.buffer; 
						pOptions = (PNF_UDP_OPTIONS)(pDataEntry->data.buffer + NF_MAX_ADDRESS_LENGTH);
						pPacketData = pDataEntry->data.buffer + NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + pOptions->optionsLength; 
						packetDataLen = pDataEntry->data.bufferSize - (unsigned long)(pPacketData - pDataEntry->data.buffer);
				
						if (pAddr->filteringDisabled)
						{
							AutoUnlock unlock(g_cs);
							nf_udpPostSend(pDataEntry->data.id, 
								(unsigned char*)pRemoteAddr,
								pPacketData, 
								packetDataLen,
								pOptions);
						} else
						{
							AutoUnlock unlock(g_cs);
							g_pEventHandler->udpSend(pDataEntry->data.id,
								(unsigned char*)pRemoteAddr, 
								pPacketData, 
								packetDataLen,
								pOptions);
						}

						mp_free(pDataEntry);

						if (sendsInBytes == 0)
							break;
					}
				}

				if (isEventFlagEnabled(m_flags, NF_UDP_CAN_RECEIVE))
				{
					if (IsListEmpty(&pAddr->receivesOut))
					{
						AutoUnlock unlock(g_cs);
						g_pEventHandler->udpCanReceive(m_id);
					}
				}

				if (isEventFlagEnabled(m_flags, NF_UDP_CAN_SEND))
				{
					if (IsListEmpty(&pAddr->sendsOut))
					{
						AutoUnlock unlock(g_cs);
						g_pEventHandler->udpCanSend(m_id);
					}
				}

				// Resume the connection 
				if ((pAddr->sendsOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pAddr->receivesOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pAddr->sendsInBytes <= MAX_QUEUE_BYTES) &&
					(pAddr->receivesInBytes <= MAX_QUEUE_BYTES))
				{
					nf_udpSetConnectionStateInternal(pAddr, pAddr->wantSuspend);
				}

				if (isEventFlagEnabled(m_flags, NF_UDP_CLOSED))
				{
					{
						AutoUnlock unlock(g_cs);
						g_pEventHandler->udpClosed(m_id, &pAddr->connInfo);
					}

					nf_pushOutEvent(m_id, NF_UDP_CLOSED);
				}
			}
		}

		eEndpointType	m_et;
		ENDPOINT_ID		m_id;
		unsigned long	m_flags;
	};

	class NFEventOut : public ThreadJob
	{
	public:

		NFEventOut(eEndpointType et, ENDPOINT_ID id, int flags) 
		{
			m_et = et;
			m_id = id;
			m_flags = flags;
		}
		
		~NFEventOut()
		{
		}

		virtual void execute()
		{
			NF_STATUS status;

			if (m_et == ET_TCP)
			{
				PCONNOBJ pConn = NULL;
				PHASH_TABLE_ENTRY phte;
				PNF_DATA_ENTRY pDataEntry;

				if (isEventFlagEnabled(m_flags, NF_TCP_REINJECT))
				{
					NF_DATA data;
					data.id = m_id;
					data.code = NF_TCP_REINJECT;
					data.bufferSize = 0;
					nf_postData(&data);
				}

				AutoLock lock(g_cs);
				phte = ht_find_entry(g_phtConnById, m_id);
				if (!phte)
				{
					return;
				}

				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

				if (isEventFlagEnabled(m_flags, NF_TCP_CAN_RECEIVE))
				{
					unsigned int receivesOutBytes = pConn->receivesOutBytes;

					pConn->lastIoTime = GetTickCount();
				
					while (!IsListEmpty(&pConn->receivesOut))
					{
						pDataEntry = (PNF_DATA_ENTRY)pConn->receivesOut.Flink;

						if (receivesOutBytes == 0 && pDataEntry->data.bufferSize > 0)
							break;

						RemoveEntryList(&pDataEntry->entry);

						if (pConn->disconnectedIn)
						{
							mp_free(pDataEntry);
							continue;
						}

						{
							AutoUnlock unlock(g_cs);
							status = nf_postData(&pDataEntry->data);
						}

						if (status == NF_STATUS_SUCCESS)
						{
							if (pDataEntry->data.bufferSize == 0)
							{
								pConn->disconnectedIn = TRUE;
							}

							if (pConn->receivesOutBytes >= pDataEntry->data.bufferSize)
							{
								pConn->receivesOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								pConn->receivesOutBytes = 0;
							}

							if (receivesOutBytes >= pDataEntry->data.bufferSize)
							{
								receivesOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								receivesOutBytes = 0;
							}

							mp_free(pDataEntry);
						} else
						{
							InsertHeadList(&pConn->receivesOut, &pDataEntry->entry);
							break;
						}
					}

					if (!pConn->filteringDisabled)
					{
						if (IsListEmpty(&pConn->receivesOut))
						{
							nf_pushInEvent(m_id, NF_TCP_CAN_RECEIVE);
						} 
					}
				}

				if (isEventFlagEnabled(m_flags, NF_TCP_CAN_SEND))
				{
					unsigned int sendsOutBytes = pConn->sendsOutBytes;

					pConn->lastIoTime = GetTickCount();

					while (!IsListEmpty(&pConn->sendsOut))
					{
						pDataEntry = (PNF_DATA_ENTRY)pConn->sendsOut.Flink; 

						if (sendsOutBytes == 0 && pDataEntry->data.bufferSize > 0)
							break;

						RemoveEntryList(&pDataEntry->entry);

						if (pConn->disconnectedOut)
						{
							mp_free(pDataEntry);
							continue;
						}

						{
							AutoUnlock unlock(g_cs);
							status = nf_postData(&pDataEntry->data);
						}

						if (status == NF_STATUS_SUCCESS)
						{ 
							if (pDataEntry->data.bufferSize == 0)
							{
								pConn->disconnectedOut = TRUE;
							}

							if (pConn->sendsOutBytes >= pDataEntry->data.bufferSize)
							{
								pConn->sendsOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								pConn->sendsOutBytes = 0;
							}
							
							if (sendsOutBytes >= pDataEntry->data.bufferSize)
							{
								sendsOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								sendsOutBytes = 0;
							}
							
							mp_free(pDataEntry);

						} else
						{
							InsertHeadList(&pConn->sendsOut, &pDataEntry->entry);
							break;
						}
					}

					if (!pConn->filteringDisabled)
					{
						if (IsListEmpty(&pConn->sendsOut))
						{
							nf_pushInEvent(m_id, NF_TCP_CAN_SEND);
						}
					}
				}

				// Resume the connection 
				if ((pConn->sendsOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pConn->receivesOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pConn->sendsInBytes <= MAX_SEND_QUEUE_BYTES) &&
					(pConn->receivesInBytes <= MAX_QUEUE_BYTES))
				{
					nf_tcpSetConnectionStateInternal(pConn, pConn->wantSuspend);
				}

				if (isEventFlagEnabled(m_flags, NF_TCP_CLOSED))
				{
					connobj_free(pConn);
				}
			} else
			if (m_et == ET_UDP)
			{
				PADDROBJ pAddr = NULL;
				PHASH_TABLE_ENTRY phte;
				PNF_DATA_ENTRY pDataEntry;
				
				AutoLock lock(g_cs);
				phte = ht_find_entry(g_phtAddrById, m_id);
				if (!phte)
				{
					return;
				}
				
				pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

				if (isEventFlagEnabled(m_flags, NF_UDP_CAN_RECEIVE))
				{
					if (!IsListEmpty(&pAddr->receivesOut))
					{
						pDataEntry = (PNF_DATA_ENTRY)pAddr->receivesOut.Flink;

						{
							AutoUnlock unlock(g_cs);
							status = nf_postData(&pDataEntry->data);
						}

						if (status == NF_STATUS_SUCCESS)
						{
							if (pAddr->receivesOutBytes >= pDataEntry->data.bufferSize)
							{
								pAddr->receivesOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								pAddr->receivesOutBytes = 0;
							}
							RemoveEntryList(&pDataEntry->entry);
							mp_free(pDataEntry);

							pAddr->indicateReceiveInProgress = TRUE;
						}
					} else
					{
						pAddr->indicateReceiveInProgress = FALSE;
					}
					
					if (IsListEmpty(&pAddr->receivesOut))
					{
						nf_pushInEvent(m_id, NF_UDP_CAN_RECEIVE);
					}
				}

				if (isEventFlagEnabled(m_flags, NF_UDP_CAN_SEND))
				{
					unsigned int sendsOutBytes = pAddr->sendsOutBytes;

					while (!IsListEmpty(&pAddr->sendsOut))
					{
						pDataEntry = (PNF_DATA_ENTRY)pAddr->sendsOut.Flink;

						{
							AutoUnlock unlock(g_cs);
							status = nf_postData(&pDataEntry->data);
						}

						if (status == NF_STATUS_SUCCESS)
						{
							if (pAddr->sendsOutBytes >= pDataEntry->data.bufferSize)
							{
								pAddr->sendsOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								pAddr->sendsOutBytes = 0;
							}
							
							if (sendsOutBytes >= pDataEntry->data.bufferSize)
							{
								sendsOutBytes -= pDataEntry->data.bufferSize;
							} else
							{
								sendsOutBytes = 0;
							}
							
							RemoveEntryList(&pDataEntry->entry);
							mp_free(pDataEntry);

							if (sendsOutBytes == 0)
								break;
						} else
						{
							break;
						}
					}

					if (IsListEmpty(&pAddr->sendsOut))
					{
						nf_pushInEvent(m_id, NF_UDP_CAN_SEND);
					}
				}

				// Resume the connection 
				if ((pAddr->sendsOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pAddr->receivesOutBytes <= MAX_OUT_QUEUE_BYTES) &&
					(pAddr->sendsInBytes <= MAX_QUEUE_BYTES) &&
					(pAddr->receivesInBytes <= MAX_QUEUE_BYTES))
				{
					nf_udpSetConnectionStateInternal(pAddr, pAddr->wantSuspend);
				}

				if (isEventFlagEnabled(m_flags, NF_UDP_CLOSED))
				{
					addrobj_free(pAddr);
				}
			}
		}

		eEndpointType	m_et;
		ENDPOINT_ID		m_id;
		unsigned long	m_flags;
	};

	template <class EventType>
	class EventQueue : public ThreadJobSource
	{
	public:
		EventQueue()
		{
			m_nThreads = 1;
			m_pending = false;
		}

		~EventQueue()
		{
		}

		bool init(int nThreads)
		{
			m_nThreads = nThreads;
			m_pending = false;
			return m_pool.init(nThreads, this);
		}

		void free()
		{
			m_pool.free();
		}

		void suspend(bool suspend)
		{
			AutoLock lock(m_cs);
			m_pending = suspend;
		}

		bool push(PNF_DATA pData)
		{
			AutoLock lock(m_cs);

			eEndpointType et = getEndpointType(pData->code);

			if (et == ET_TCP)
			{
				tEventFlags::iterator it = m_tcpEventFlags.find(pData->id);
				if (it != m_tcpEventFlags.end())
				{
					it->second |= getEventFlag(pData->code);
				} else
				{
					m_tcpEventFlags[pData->id] = getEventFlag(pData->code);
					EventListItem eli = { pData->id, ET_TCP };
					m_eventList.push_back(eli);
				}
			} else
			if (et == ET_UDP)
			{
				tEventFlags::iterator it = m_udpEventFlags.find(pData->id);
				if (it != m_udpEventFlags.end())
				{
					it->second |= getEventFlag(pData->code);
				} else
				{
					m_udpEventFlags[pData->id] = getEventFlag(pData->code);
					EventListItem eli = { pData->id, ET_UDP };
					m_eventList.push_back(eli);
				}
			}

			return true;
		}

		void processEvents()
		{
			if (!m_pending)
			{
				m_pool.jobAvailable();
			}
		}

		void wait(size_t maxQueueSize)
		{
			for (;;)
			{
				{
					AutoLock lock(m_cs);
					if ((m_eventList.size() + m_busyTCPEndpoints.size() + m_busyUDPEndpoints.size()) <= maxQueueSize)
						return;
				}

				WaitForSingleObject(m_jobCompletedEvent, 10 * 1000);
			}
		}

		virtual ThreadJob * getNextJob()
		{
			AutoLock lock(m_cs);

			EventType * pEvent = NULL;
			tEventFlags::iterator itf;
			int flags = 0;

			// EDR_FIX: Fix compilation on vs2017
			for (typename tEventList::iterator it = m_eventList.begin(); it != m_eventList.end(); )
			{
				if (it->type == ET_TCP)
				{
					if (m_busyTCPEndpoints.find(it->id) != m_busyTCPEndpoints.end())
					{
						it++;
						continue;
					}

					itf = m_tcpEventFlags.find(it->id);
					if (itf == m_tcpEventFlags.end())
					{
						it = m_eventList.erase(it);
						continue;
					}
					flags = itf->second;
					m_tcpEventFlags.erase(itf);
					m_busyTCPEndpoints.insert(it->id);
				} else
				if (it->type == ET_UDP)
				{
					if (m_busyUDPEndpoints.find(it->id) != m_busyUDPEndpoints.end())
					{
						it++;
						continue;
					}

					itf = m_udpEventFlags.find(it->id);
					if (itf == m_udpEventFlags.end())
					{
						it = m_eventList.erase(it);
						continue;
					}

					flags = itf->second;
					m_udpEventFlags.erase(itf);
					m_busyUDPEndpoints.insert(it->id);
				}

				void * p = mp_alloc(sizeof(EventType));
				if (!p)
				{
					return NULL;
				}

				pEvent = new(p) EventType(it->type, it->id, flags);

				m_eventList.erase(it);

				return pEvent;
			}

			return NULL;
		}

		virtual void jobCompleted(ThreadJob * pJob)
		{
			AutoLock lock(m_cs);

			EventType * pEvent = (EventType*)pJob;

			if (pEvent->m_et == ET_TCP)
			{
				m_busyTCPEndpoints.erase(pEvent->m_id);
			} else
			if (pEvent->m_et == ET_UDP)
			{
				m_busyUDPEndpoints.erase(pEvent->m_id);
			}

			mp_free(pEvent);

			SetEvent(m_jobCompletedEvent);

			if (!m_pending && !m_eventList.empty())
			{
				m_pool.jobAvailable();
			}
		}

		virtual void threadStarted()
		{
			g_pEventHandler->threadStart();
		}

		virtual void threadStopped()
		{
			g_pEventHandler->threadEnd();
		}

	private:
		struct EventListItem
		{
			ENDPOINT_ID		id;
			eEndpointType	type;
		};

		typedef std::list<EventListItem> tEventList;
		tEventList m_eventList;
		
		typedef std::map<ENDPOINT_ID, int> tEventFlags;
		tEventFlags m_tcpEventFlags;
		tEventFlags m_udpEventFlags;
		
		typedef std::set<ENDPOINT_ID> tBusyEndpoints;
		tBusyEndpoints m_busyTCPEndpoints;
		tBusyEndpoints m_busyUDPEndpoints;
		
		AutoEventHandle m_jobCompletedEvent;
		bool m_pending;
		
		ThreadPool m_pool;

		int m_nThreads;
		
		AutoCriticalSection m_cs;
	};

}