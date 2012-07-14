//*****************************************************************************
// FILE:            net.cpp
// derived from WinMTRNet.cpp (WinMTR 0.8)
//
//    Copyleft (C) 2000-2002, Vasile Laurentiu Stanimir ( stanimir@cr.nivis.com )
//    Modified 2009, Steve Harvey (sgh@vex.net)
//
//*****************************************************************************
#include "global.h"
#include "net.h"

#include <iostream>

void DnsResolverThread(void *p);

int WinMTRNet::CalcDeltatime() {
	float waittime = WaitTime / numhosts;
	return (int)(1000000 * waittime);
}


unsigned short WinMTRNet::Checksum(unsigned short  *lpBuf, int nLen) {
	register long lSum = 0L;

	while (nLen > 0) {
		lSum += *(lpBuf++);
		nLen -= 2;
	}

	lSum = (lSum & 0xffff) + (lSum>>16);
	lSum += (lSum >> 16);
	return (unsigned short)(~lSum);
}

/** Allocate a new entry in the sequence table.  
  */
int WinMTRNet::GetNewSequence(int index) {
	static unsigned int next_sequence = 0;
	int seq;

	seq = next_sequence++;

	if (next_sequence >= MaxSequence)
		next_sequence = 0;

	sequence[seq].index = index;
	sequence[seq].transit = 1;
	sequence[seq].saved_seq = ++host[index].xmit;

	memset(&sequence[seq].time, 0, sizeof(sequence[seq].time));
	host[index].transit = 1;

	SaveXmit(index);
	return seq;
}


/** Send a single ICMP ping probe packet.  The packetsize is forced into the
  * acceptable range.  The sequence number of the packet is set to the index
  * of the entry in the sequence table in which is stored the current time.
  * The ICMP ID is set to the current process ID.
  */
void WinMTRNet::SendQuery(int index) {
	char packet[MAXPACKET];
	struct ICMPHeader *icmp;

        struct timespec time_in_ns;

	if (packetsize < MINPACKET) packetsize = MINPACKET;
	if (packetsize > MAXPACKET) packetsize = MAXPACKET;

	// FIXME: use a locally scoped copy of packetsize from here on (BUG)
	packetsize -=  IP_HEADER_LENGTH;
	memset(packet, 0, packetsize);
	icmp = (struct ICMPHeader *)packet;
	int ttl = index+1;

	if (setsockopt(sendsock, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl)) < 0) {
		wmtrdlg->errMsg("error setsockopt");
		exit(ES_SETSOCKOPT_FAIL);
	}


	icmp->type = ICMP_ECHO;

	icmp->id = getpid();
	icmp->sequence = GetNewSequence(index);
	icmp->checksum = Checksum((unsigned short*)icmp, packetsize);

	// anto reverted this
	gettimeofday (&(sequence[icmp->sequence].time), NULL);

        //clock_gettime (CLOCK_MONOTONIC, &time_in_ns);

        sequence[icmp->sequence].time.tv_sec = time_in_ns.tv_sec;
        sequence[icmp->sequence].time.tv_usec = time_in_ns.tv_nsec / 1000;
        

	sendto(sendsock, packet, packetsize , 0,(struct sockaddr *)&remoteaddress,
	       sizeof(remoteaddress));
}


void WinMTRNet::ProcessPing(int seq, __int32 addr, struct timeval now) {
	int index;
	int totusec;

	if (seq < 0 || seq >= MaxSequence)
		return;

	// ignore duplicate packets
	if (!sequence[seq].transit)
		return;

	sequence[seq].transit = 0;
	index = sequence[seq].index;

	totusec =	(now.tv_sec  - sequence[seq].time.tv_sec) * 1000000 +
	          (now.tv_usec - sequence[seq].time.tv_usec);

	if (host[index].addr == 0) {
		host[index].addr = addr;
	} else if ( host[index].addr != addr ) {
		host[index].inconsist++;
	}

	if (host[index].returned <= 0) {
		host[index].best = host[index].worst = totusec;
	}

	host[index].last = totusec;

	if (totusec < host[index].best)
		host[index].best = totusec;

	if (totusec > host[index].worst)
		host[index].worst = totusec;

	host[index].total += totusec;
	host[index].returned++;
	host[index].transit = 0;
	SaveReturn(index, sequence[seq].saved_seq, totusec);
}



void WinMTRNet::ProcessReturn() {
	char packet[2048];
	struct sockaddr_in fromaddr;
#if defined (linux) || defined(__APPLE__)
	socklen_t fromaddrsize;
#else
	int fromaddrsize;
#endif
	int num;
	struct ICMPHeader *header;
	struct timeval now;

        struct timespec time_in_ns;

	// anto reverted this, in order to remove the definition error and test
	gettimeofday(&now, NULL);

        // ANTO commented this
        //clock_gettime (CLOCK_MONOTONIC, &time_in_ns);

        now.tv_sec = time_in_ns.tv_sec;
        now.tv_usec = time_in_ns.tv_nsec / 1000;


	fromaddrsize = sizeof(fromaddr);
	num = recvfrom(recvsock, packet, 2048, 0,
	               (struct sockaddr *)&fromaddr, &fromaddrsize);

	if (num < IP_HEADER_LENGTH + sizeof(struct ICMPHeader))
		return;

	header = (struct ICMPHeader *)(packet + IP_HEADER_LENGTH);

	if (header->type == ICMP_ECHOREPLY) {
		if (header->id != (__int16)getpid())
			return;

		ProcessPing(header->sequence, fromaddr.sin_addr.s_addr, now);
	} else if (header->type == ICMP_TIME_EXCEEDED) {
		if (num < IP_HEADER_LENGTH + sizeof(struct ICMPHeader) +
		                IP_HEADER_LENGTH + sizeof(struct ICMPHeader))
			return;

		header = (struct ICMPHeader *)(packet + IP_HEADER_LENGTH +
		                               sizeof(struct ICMPHeader) + IP_HEADER_LENGTH);

		if (header->id != (__int16)getpid())
			return;

		ProcessPing(header->sequence, fromaddr.sin_addr.s_addr, now);
	} else if (header->type == ICMP_HOST_UNREACHABLE) {
		int c = header->code;
		header = (struct ICMPHeader *)(packet + IP_HEADER_LENGTH +
		                               sizeof(struct ICMPHeader) + IP_HEADER_LENGTH);

		if (header->id != (__int16)getpid())
			return;

		int index = sequence[header->sequence].index;

		// still want the router issuing these...  host[index].addr = 0;
		ProcessPing(header->sequence, fromaddr.sin_addr.s_addr, now);

		switch (c) {

			case 0 :
				SetName(index,"Network Unreachable");
				break;

			case 1 :
				SetName(index,"Host Unreachable");
				break;

			case 2:
				SetName(index,"Protocol unreachable");
				break;

			case 3:
				SetName(index,"Port unreachable");
				break;

			case 4:
				SetName(index,"Fragmentation needed but the DF bit was set");
				break;

			case 5:
				SetName(index,"Source route failed");
				break;

			case 6:
				SetName(index,"Destination network unknown");
				break;

			case 7:
				SetName(index,"Destination host unknown");
				break;

			case 8:
				SetName(index,"Source host isolated");
				break;

			case 9:
				SetName(index,"Destination network administratively prohibited");
				break;

			case 10:
				SetName(index,"Destination host administratively prohibited");
				break;

			case 11:
				SetName(index,"Network unreachable for this type of service");
				break;

			case 12:
				SetName(index,"Host unreachable for this type of service");
				break;

			case 13:
				SetName(index,"Communication administratively prohibited by filtering");
				break;

			case 14:
				SetName(index,"Host precedence violation");
				break;

			case 15:
				SetName(index,"Precedence cutoff in effect ");
				break;

			default:
				char s[1024];
				sprintf(s, "ICMP_HOST_UNREACHABLE(%d)", c);
				SetName(index, s);
				break;
		}
	}
}


int WinMTRNet::GetAddr(int at) {
	return ntohl(host[at].addr);
}

const char * WinMTRNet::GetConsistency(int at) {
	return host[at].inconsist ? "i" : "";
}

char * WinMTRNet::FmtAddr(int at, char *n) {
	int addr = GetAddr(at);
	sprintf(n, "%d.%d.%d.%d",
	        (addr >> 24) & 0xff,
	        (addr >> 16) & 0xff,
	        (addr >> 8) & 0xff,
	        addr & 0xff
	       );

	if (addr==0)
		strcpy(n,"*");
	return n;
}


char * WinMTRNet::GetName(int at, char *n) {
	if (!strcmp(host[at].name, "")) {
		FmtAddr(at, n);
	} else {
		strcpy(n, host[at].name);
	}

	return n;
}


void WinMTRNet::SetName(int at, char *n) {
	strncpy(host[at].name, n, 255);
}

int WinMTRNet::GetPercent(int at) {
	if ((host[at].xmit - host[at].transit) == 0)
		return 0;

	return 100 - (100 * host[at].returned / (host[at].xmit - host[at].transit));
}


int WinMTRNet::GetLast(int at) {
	return host[at].last;
}


int WinMTRNet::GetBest(int at) {
	return host[at].best;
}


int WinMTRNet::GetWorst(int at) {
	return host[at].worst;
}


int WinMTRNet::GetAvg(int at) {
	if (host[at].returned == 0)
		return 0;

	return host[at].total / host[at].returned;
}


int WinMTRNet::GetMax() {
	int at;
	int max;

	max = 0;
	for (at = 0; at < MaxHost-2; at++) {
		if ((unsigned)host[at].addr == remoteaddress.sin_addr.s_addr) {
			return at + 1;
		} else if (host[at].addr != 0) {
			max = at + 2;
		}
	}
	return max;
}


int WinMTRNet::GetReturned(int at) {
	return host[at].returned;
}


int WinMTRNet::GetXmit(int at) {
	return host[at].xmit;
}


int WinMTRNet::GetTransit(int at) {
	return host[at].transit;
}


void WinMTRNet::EndTransit() {
	int at;

	for (at = 0; at < MaxHost; at++) {
		host[at].transit = 0;
	}
}


/** Send a ping in the current batch, and then decide if we are at
  * the end of the batch.  Each batch consists of a run of pings
  * with ascending Time To Live (TTL) values, and goes until there
  * is heuristic evidence that the destination host is going to be reached,
  * or that a suitably large run of non-responses will have been
  * encountered, or the TTL value has reached the maximum limit.
  */
int WinMTRNet::SendBatch() {
	int n_unknown, i;

	SendQuery(batch_at);
	n_unknown = 0;
	for (i=0;i<batch_at;i++) {
		if (host[i].addr == 0)
			n_unknown++;

		if ((unsigned)host[i].addr == remoteaddress.sin_addr.s_addr) {
			full_loop = TRUE;
			n_unknown = 100; /* Make sure we drop into "we should restart" */
		}
	}

	if (((unsigned)host[batch_at].addr == remoteaddress.sin_addr.s_addr) ||
	                (n_unknown > MAX_UNKNOWN_HOSTS) ||
	                (batch_at >= MaxHost-2)) {
		full_loop = TRUE;
		numhosts = batch_at+1;
		batch_at = 0;
		return 1;
	}

	batch_at++;
	return 0;
}


int WinMTRNet::Preopen(WinMTRDialog *wp) {
	wmtrdlg = wp;

	packetsize = wmtrdlg->pingsize;
	WaitTime = (float)wmtrdlg->interval ;

	batch_at = 0;
	numhosts = 10;

	sendsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (sendsock < 0)
		return -1;

#ifdef SEPARATE_SR_SOCKETS
	recvsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(recvsock < 0)
		return -1;
#else
	recvsock = sendsock;
#endif

	return 0;
}


int WinMTRNet::Open(int addr, int laddr) {
	Reset();

	remoteaddress.sin_family = AF_INET;
	remoteaddress.sin_addr.s_addr = addr;

	localaddress.sin_family = AF_INET;
	localaddress.sin_addr.s_addr = laddr;

#ifdef WantDNSResolution
	if (wmtrdlg->useDNS)
		_beginthread(DnsResolverThread, 0, this);
#endif

	return 0;
}


void WinMTRNet::Reset() {
	int at;
	int i;

	batch_at = 0;
	numhosts = 10;
	full_loop = FALSE;

	for (at = 0; at < MaxHost; at++) {
		strcpy(host[at].name,"");
		host[at].addr = 0;
		host[at].inconsist = 0;
		host[at].xmit = 0;
		host[at].transit = 0;
		host[at].returned = 0;
		host[at].total = 0;
		host[at].best = 0;
		host[at].worst = 0;

		for (i=0; i<SAVED_PINGS; i++) {
			host[at].saved[i] = -2;	/* unsent */
		}
	}

	for (at = 0; at < MaxSequence; at++) {
		sequence[at].transit = 0;
	}
}


void WinMTRNet::Close() {
	//std::cout << "net: closing socket " << sendsock << std::endl;
	skt_close(sendsock);
#ifdef SEPARATE_SR_SOCKETS
	skt_close(recvsock);
#endif
}


void WinMTRNet::skt_close(int skt) {
#ifdef WIN32
	closesocket(skt);
#else
	close(skt);
#endif
}

int WinMTRNet::GetWaitFd() {
	return recvsock;
}


int* WinMTRNet::GetSavedPings(int at) {
	return host[at].saved;
}


void WinMTRNet::SaveXmit(int at) {
	int tmp[SAVED_PINGS];
	memcpy(tmp, &host[at].saved[1], (SAVED_PINGS-1)*sizeof(int));
	memcpy(host[at].saved, tmp, (SAVED_PINGS-1)*sizeof(int));
	host[at].saved[SAVED_PINGS-1] = -1;
}


void WinMTRNet::SaveReturn(int at, int seq, int ms) {
	int idx;
	idx = SAVED_PINGS - (host[at].xmit - seq) - 1;

	if (idx < 0) {
		return;
	}

	host[at].saved[idx] = ms;
}


#ifdef WantDNSResolution
void DnsResolverThread(void *p) {
	WinMTRNet* wn = (WinMTRNet *)(p);
	int resolved[MaxHost];

	for (int i=0;i<MaxHost;i++)
		resolved[i]=0;

	int all = 0;

	while (wn->wmtrdlg->start && !(wn->FullLoop() && all)) {
		Sleep(100);
		all = 0;

		for (int i=0 ;i<wn->GetMax();i++) {

			if (!resolved[i]) {
				all = 1;

				struct hostent *phent ;

				char buf[20];
				int addr = wn->GetAddr(i);

				if (addr != 0) {
					sprintf(buf, "%d.%d.%d.%d",
					        (addr >> 24) & 0xff,
					        (addr >> 16) & 0xff,
					        (addr >> 8) & 0xff,
					        addr & 0xff
					       );

					int haddr = htonl(addr);
					phent = gethostbyaddr((const char*)&haddr, sizeof(int), AF_INET);

					if (phent) {
						wn->SetName(i, phent->h_name);
						resolved[i] = 1;
					} else {
						wn->SetName(i, buf);
					}
				}
			}
		}
	}

	_endthread();
}
#endif
