/**
*	The sample filters all non-loopback IP packets in pass-through mode,
*	and prints the information about all called events to standard console output.
**/

#include "stdafx.h"
#include <crtdbg.h>
#include "nfapi.h"
#include <time.h>

using namespace nfapi;

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

// Forward declarations
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);
void dump_ip(const char * buf, int len, PNF_IP_PACKET_OPTIONS options);
FILE * pcap_open(const char * fileName);
void pcap_write(FILE * fp, const char * buf, int len);

static FILE * g_pcapFile = NULL;


//
//	API events handler
//
class EventHandler : public NF_EventHandler
{
	virtual void threadStart()
	{
		printf("threadStart\n");
		fflush(stdout);

		// Initialize thread specific stuff
	}

	virtual void threadEnd()
	{
		printf("threadEnd\n");

		// Uninitialize thread specific stuff
	}
	
	//
	// TCP events
	//

	virtual void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printf("tcpConnectRequest id=%I64u\n", id);
	}

	virtual void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printConnInfo(true, id, pConnInfo);
		fflush(stdout);
	}

	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printConnInfo(false, id, pConnInfo);
		fflush(stdout);
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		printf("tcpReceive id=%I64u len=%d\n", id, len);
		fflush(stdout);

		// Send the packet to application
		nf_tcpPostReceive(id, buf, len);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		printf("tcpSend id=%I64u len=%d\n", id, len);
		fflush(stdout);

		// Send the packet to server
		nf_tcpPostSend(id, buf, len);
	}

	virtual void tcpCanReceive(ENDPOINT_ID id)
	{
		printf("tcpCanReceive id=%I64d\n", id);
		fflush(stdout);
	}

	virtual void tcpCanSend(ENDPOINT_ID id)
	{
		printf("tcpCanSend id=%I64d\n", id);
		fflush(stdout);
	}
	
	//
	// UDP events
	//

	virtual void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		printAddrInfo(true, id, pConnInfo);
		fflush(stdout);
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
		printf("udpConnectRequest id=%I64u\n", id);
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		printAddrInfo(false, id, pConnInfo);
		fflush(stdout);
	}

	virtual void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{	
		char remoteAddr[MAX_PATH];
		DWORD dwLen;
		
		dwLen = sizeof(remoteAddr);
		WSAAddressToString((sockaddr*)remoteAddress, 
				(((sockaddr*)remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);

		printf("udpReceive id=%I64u len=%d remoteAddress=%s flags=%x optionsLen=%d\n", id, len, remoteAddr, options->flags, options->optionsLength);
		fflush(stdout);

		// Send the packet to application
		nf_udpPostReceive(id, remoteAddress, buf, len, options);
	}

	virtual void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		char remoteAddr[MAX_PATH];
		DWORD dwLen;
		
		dwLen = sizeof(remoteAddr);
		WSAAddressToString((sockaddr*)remoteAddress, 
				(((sockaddr*)remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);

		printf("udpSend id=%I64u len=%d remoteAddress=%s flags=%x optionsLen=%d\n", id, len, remoteAddr, options->flags, options->optionsLength);
		fflush(stdout);

		// Send the packet to server
		nf_udpPostSend(id, remoteAddress, buf, len, options);
	}

	virtual void udpCanReceive(ENDPOINT_ID id)
	{
		printf("udpCanReceive id=%I64d\n", id);
		fflush(stdout);
	}

	virtual void udpCanSend(ENDPOINT_ID id)
	{
		printf("udpCanSend id=%I64d\n", id);
		fflush(stdout);
	}
};

//
//	API events handler for IP packets
//
class IPEventHandler : public NF_IPEventHandler
{
	virtual void ipReceive(const char * buf, int len, PNF_IP_PACKET_OPTIONS options)
	{	
		printf("ipReceive len=%d\n", len);

		if (g_pcapFile)
		{
			pcap_write(g_pcapFile, buf, len);
		} else
		{
			dump_ip(buf, len, options);
		}

		nf_ipPostReceive(buf, len, options);
	}

	virtual void ipSend(const char * buf, int len, PNF_IP_PACKET_OPTIONS options)
	{
		printf("ipSend len=%d\n", len);

		if (g_pcapFile)
		{
			pcap_write(g_pcapFile, buf, len);
		} else
		{
			dump_ip(buf, len, options);
		}

		nf_ipPostSend(buf, len, options);
	}
};

int main(int argc, char* argv[])
{
	EventHandler eh;
	IPEventHandler ipeh;
	NF_RULE rule;
	WSADATA wsaData;

	// This call is required for WSAAddressToString
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

	if (argc > 1)
	{
		g_pcapFile = pcap_open(argv[1]);
		if (!g_pcapFile)
		{
			printf("Unable to create pcap file %s\n", argv[1]);
			return -1;
		}

		printf("Saving the output to pcap file %s\n", argv[1]);
	}

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, &eh) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver\n");
		return -1;
	}

	printf("Press any key to stop...\n\n");

	// Specify the object for handling NF_IPEventHandler events
	nf_setIPEventHandler(&ipeh);

	// Filter all IP packets
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_FILTER_AS_IP_PACKETS;
	nf_addRule(&rule, TRUE);

	// Wait for any key
	getchar();

	// Free the library
	nf_free();

	::WSACleanup();

	if (g_pcapFile)
	{
		fclose(g_pcapFile);
	}

	return 0;
}

/**
* Print the connection information
**/
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	char remoteAddr[MAX_PATH] = "";
	DWORD dwLen;
	sockaddr * pAddr;
	char processName[MAX_PATH] = "";
	
	pAddr = (sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				localAddr, 
				&dwLen);

	pAddr = (sockaddr*)pConnInfo->remoteAddress;
	dwLen = sizeof(remoteAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);
	
	if (connected)
	{
		if (!nf_getProcessName(pConnInfo->processId, processName, sizeof(processName)/sizeof(processName[0])))
		{
			processName[0] = '\0';
		}

		printf("tcpConnected id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s (conn.table size %d)\n\tProcess: %s\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr,
			nf_getConnCount(),
			processName);
	} else
	{
		printf("tcpClosed id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s (conn.table size %d)\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr,
			nf_getConnCount());
	}

}

/**
* Print the address information
**/
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	sockaddr * pAddr;
	DWORD dwLen;
	char processName[MAX_PATH] = "";
	
	pAddr = (sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				localAddr, 
				&dwLen);
		
	if (created)
	{
		if (!nf_getProcessName(pConnInfo->processId, processName, sizeof(processName)/sizeof(processName[0])))
		{
			processName[0] = '\0';
		}

		printf("udpCreated id=%I64u pid=%d local=%s\n\tProcess: %s\n",
			id,
			pConnInfo->processId, 
			localAddr, 
			processName);
	} else
	{
		printf("udpClosed id=%I64u pid=%d local=%s\n",
			id,
			pConnInfo->processId, 
			localAddr);
	}

}

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
typedef	unsigned char	__uint8_t;
typedef	unsigned short	__uint16_t;
typedef	unsigned int	__uint32_t;
typedef	unsigned char	u_int8_t;
typedef	unsigned short	u_int16_t;
typedef	unsigned int	u_int32_t;
typedef unsigned __int64  u_int64;

#pragma pack(1) 

struct ip {
	u_char	ip_vhl;			/* version << 4 | header length >> 2 */
	u_char	ip_tos;			/* type of service */
	u_short	ip_len;			/* total length */
	u_short	ip_id;			/* identification */
	u_short	ip_off;			/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_char	ip_ttl;			/* time to live */
	u_char	ip_p;			/* protocol */
	u_short	ip_sum;			/* checksum */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

#define	IP_MAKE_VHL(v, hl)	((v) << 4 | (hl))
#define	IP_VHL_HL(vhl)		((vhl) & 0x0f)
#define	IP_VHL_V(vhl)		((vhl) >> 4)
#define	IP_VHL_BORING		0x45
#define	IPVERSION	4

struct ip6_hdr {
	union {
		struct ip6_hdrctl {
			u_int32_t ip6_un1_flow;	/* 20 bits of flow-ID */
			u_int16_t ip6_un1_plen;	/* payload length */
			u_int8_t  ip6_un1_nxt;	/* next header */
			u_int8_t  ip6_un1_hlim;	/* hop limit */
		} ip6_un1;
		u_int8_t ip6_un2_vfc;	/* 4 bits version, top 4 bits class */
	} ip6_ctlun;
	struct in6_addr ip6_src;	/* source address */
	struct in6_addr ip6_dst;	/* destination address */
};

#define ip6_vfc		ip6_ctlun.ip6_un2_vfc
#define ip6_flow	ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen	ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt		ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim	ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops	ip6_ctlun.ip6_un1.ip6_un1_hlim

#define IPV6_VERSION		0x60
#define IPV6_VERSION_MASK	0xf0

typedef	u_long	tcp_seq;
typedef u_long	tcp_cc;			

typedef struct _TCP_HEADER 
{
	u_short	th_sport;		/* source port */
	u_short	th_dport;		/* destination port */
	tcp_seq	th_seq;			/* sequence number */
	tcp_seq	th_ack;			/* acknowledgement number */
	u_char	th_x2:4,			/* data offset */
		th_off:4;
	u_char	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG)

	u_short	th_win;			/* window */
	u_short	th_sum;			/* checksum */
	u_short	th_urp;			/* urgent pointer */
} TCP_HEADER, *PTCP_HEADER;

typedef struct UDP_HEADER_ 
{
    u_short srcPort;
    u_short destPort;
    u_short length;
    u_short checksum;
} UDP_HEADER, *PUDP_HEADER;

#pragma pack() 

void dump_ip(const char * buf, int len, PNF_IP_PACKET_OPTIONS options)
{
	struct ip * pIp;
	int i, dataSize, offset;
	int protocol;

	if (len < sizeof(struct ip))
		return;

	printf("interface %d\n", options->interfaceIndex);
	printf("subInterface %d\n", options->subInterfaceIndex);
	printf("ip_family %d\n", options->ip_family);
	printf("compartmentId %d\n", options->compartmentId);
	printf("header len %d\n", options->ipHeaderSize);

	pIp = (struct ip*)buf;
	if (IP_VHL_V(pIp->ip_vhl) != IPVERSION)
	{
		struct ip6_hdr * pIp6 = (struct ip6_hdr*)buf;
		
		if (pIp6->ip6_vfc != IPV6_VERSION)
		{
			printf("not IPv4 or IPv6\n");
			return;
		}

		printf("IPv6\n");

		protocol = pIp6->ip6_nxt;

		printf("protocol %d\n", pIp6->ip6_nxt);

		printf("src addr:");
		for (i=0; i<8; i++)
		{
			printf("%x:", ntohs(pIp6->ip6_src.u.Word[i]));
		}
		printf("\n");
	
		printf("dst addr:");
		for (i=0; i<8; i++)
		{
			printf("%x:", ntohs(pIp6->ip6_dst.u.Word[i]));
		}
		printf("\n");
	} else
	{
		printf("IPv4\n");

		protocol = pIp->ip_p;

		printf("protocol %d\n", pIp->ip_p);

		printf("src addr: %d.%d.%d.%d\n", 
			pIp->ip_src.S_un.S_un_b.s_b1,
			pIp->ip_src.S_un.S_un_b.s_b2,
			pIp->ip_src.S_un.S_un_b.s_b3,
			pIp->ip_src.S_un.S_un_b.s_b4
		);

		printf("dst addr: %d.%d.%d.%d\n", 
			pIp->ip_dst.S_un.S_un_b.s_b1,
			pIp->ip_dst.S_un.S_un_b.s_b2,
			pIp->ip_dst.S_un.S_un_b.s_b3,
			pIp->ip_dst.S_un.S_un_b.s_b4
		);
	}

	offset = options->ipHeaderSize;

	if (protocol == IPPROTO_TCP)
	{
		PTCP_HEADER pTcp = (PTCP_HEADER)(buf + offset);

		printf("src port: %d\n", ntohs(pTcp->th_sport));
		printf("dst port: %d\n", ntohs(pTcp->th_dport));
		printf("flags: 0x%x\n", pTcp->th_flags);

		offset += pTcp->th_off*4;
		
		dataSize = len - offset;
	
		if (dataSize > 0)
		{
			printf("TCP packet content part:\n");

			for (i=0; i<100 && i < dataSize; i++)
			{
				if (isprint((unsigned char)buf[offset+i]))
					printf("%c", buf[offset+i]);
				else
					printf(".");
			}
		}
	} else
	if (protocol == IPPROTO_UDP)
	{
		PUDP_HEADER pUdp = (PUDP_HEADER)(buf + offset);

		printf("src port: %d\n", ntohs(pUdp->srcPort));
		printf("dst port: %d\n", ntohs(pUdp->destPort));

		offset += sizeof(UDP_HEADER);
		
		dataSize = len - offset;	

		if (dataSize > 0)
		{
			printf("UDP packet content part:\n");

			for (i=0; i<100 && i < dataSize; i++)
			{
				if (isprint((unsigned char)buf[offset+i]))
					printf("%c", buf[offset+i]);
				else
					printf(".");
			}
		}
	}

	printf("\n=====================\n");
}

#define TCPDUMP_MAGIC		0xa1b2c3d4
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4
#define LINKTYPE_RAW		101

typedef unsigned int bpf_u_int32;
typedef int bpf_int32;

struct pcap_file_header {
	bpf_u_int32 magic;
	u_short version_major;
	u_short version_minor;
	bpf_int32 thiszone;	/* gmt to local correction */
	bpf_u_int32 sigfigs;	/* accuracy of timestamps */
	bpf_u_int32 snaplen;	/* max length saved portion of each pkt */
	bpf_u_int32 linktype;	/* data link type (LINKTYPE_*) */
};

struct pcap_timeval {
    bpf_int32 tv_sec;		/* seconds */
    bpf_int32 tv_usec;		/* microseconds */
};

struct pcap_sf_pkthdr 
{
    struct pcap_timeval ts;	/* time stamp */
    bpf_u_int32 caplen;		/* length of portion present */
    bpf_u_int32 len;		/* length this packet (off wire) */
};

FILE * pcap_open(const char * fileName)
{
	struct pcap_file_header hdr;
	FILE * fp;

	fp = fopen(fileName, "wb");
	if (!fp)
		return NULL;

	hdr.magic = TCPDUMP_MAGIC;
	hdr.version_major = PCAP_VERSION_MAJOR;
	hdr.version_minor = PCAP_VERSION_MINOR; 

	hdr.thiszone = 0;
	hdr.snaplen = 0;
	hdr.sigfigs = 0;
	hdr.linktype = LINKTYPE_RAW;

	if (fwrite((char *)&hdr, sizeof(hdr), 1, fp) != 1)
	{
		fclose(fp);
		return NULL;
	}

	return fp;
}

void pcap_write(FILE * fp, const char * buf, int len)
{
	struct pcap_sf_pkthdr sf_hdr;
	__time32_t t = _time32(NULL);

	sf_hdr.ts.tv_sec  = (bpf_int32)t;
	sf_hdr.ts.tv_usec = 0;
	sf_hdr.caplen     = len;
	sf_hdr.len        = len;

	(void)fwrite(&sf_hdr, sizeof(sf_hdr), 1, fp);
	(void)fwrite(buf, len, 1, fp);
}