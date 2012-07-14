//*****************************************************************************
// FILE:            dialog.cpp
// derived from WinMTRDialog.cpp (WinMTR 0.8)
//
//    Copyleft (C) 2000-2002, Vasile Laurentiu Stanimir ( stanimir@cr.nivis.com )
//    Modified 2009, Steve Harvey (sgh@vex.net)
//
//*****************************************************************************

#include "global.h"
#include "net.h"
#include "dialog.h"

#include <iostream>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static	 char THIS_FILE[] = __FILE__;
#endif


// WinMTRNet wmtrnet;

void PingThread(void *p);

static struct timeval intervaltime;

//! \todo { perhaps this should output to std::cerr }
void WinMTRDialog::errMsg(const char *msg) {
	std::cout << msg << std::endl;
}

WinMTRDialog::WinMTRDialog() {
	useDNS = DEFAULT_DNS;
	interval = DEFAULT_INTERVAL;
	pingsize = DEFAULT_PING_SIZE;
	max_att = DEFAULT_MAX_ATT;

	wmtrnet = new WinMTRNet(this);

	start = false;
	rep_mpls = false;
	rep_ttl = false;
	rep_icmp_status = false;
}

void WinMTRDialog::SetHostName(const char *host) {
	strncpy(defaulthostname, host, sizeof(defaulthostname)-1);
}


void WinMTRDialog::SetPingSize(int ps) {
	pingsize = ps;
}

void WinMTRDialog::SetInterval(float i) {
	interval = i;
}

void WinMTRDialog::SetUseDNS(BOOL udns) {
	useDNS = udns;
}

void WinMTRDialog::SetMPLS(bool do_mpls) {
	rep_mpls = do_mpls;
}

void WinMTRDialog::SetTTL(bool do_ttl) {
	rep_ttl = do_ttl;
}

void WinMTRDialog::SetICMPStatus(bool do_icmp_status) {
	rep_icmp_status = do_icmp_status;
}

void WinMTRDialog::SetMaxAttempts(int nprobes) {
	if ( nprobes > 0 && nprobes < 100 )
		max_att = nprobes;
}


int WinMTRDialog::restart(const char *host_name) {
	int rc = 0;
	SetHostName(host_name);
#ifdef WIN32
	rc = InitMTRNet();
	if ( rc == 0 ) {
		wmtrnet->DoTrace(traddr, max_att, pingsize, interval);
	}
#else
	start = !start;
	if (start) {
		int rc = InitMTRNet();
		if ( rc == 0 ) {
//#ifdef linux
#if 1
			PingThread(this);
#else
			_beginthread(PingThread, 0 , this);
#endif
		} else {
			start = false;
			return rc;
		}
	}


	if (!start)  {
		Sleep(1000);
	}
#endif
	return rc;
}


int WinMTRDialog::DisplayRedraw()
{
	char buf[255], nr_crt[255];
	int nh = wmtrnet->GetMax();
	int inconsist_count = 0;
	for(int i=0;i <nh ; i++) {

		wmtrnet->GetName(i, buf);
		if( (strcmp(buf,"*")==0) && wmtrnet->FullLoop())
			strcpy(buf,"No response from host");
		const char *cons = wmtrnet->GetConsistency(i);
		if ( *cons == 'i' )
			inconsist_count++;
		std::cout << buf << cons << " ";
		
		sprintf(nr_crt, "%d", i+1);
		std::cout << nr_crt << " ";

		sprintf(buf, "%d", wmtrnet->GetPercent(i));
		std::cout << buf << " ";

		sprintf(buf, "%d", wmtrnet->GetXmit(i));
		std::cout << buf << " ";

		sprintf(buf, "%d", wmtrnet->GetReturned(i));
		std::cout << buf << " ";

		sprintf(buf, "%d", wmtrnet->GetBest(i)/1000);
		std::cout << buf << " ";

		sprintf(buf, "%d", wmtrnet->GetAvg(i)/1000);
		std::cout << buf << " ";

		sprintf(buf, "%d", wmtrnet->GetWorst(i)/1000);
		std::cout << buf << " ";

		sprintf(buf, "%d", wmtrnet->GetLast(i)/1000);
		std::cout << buf << " |";

		int *v = wmtrnet->GetSavedRTTs(i);
		for (int j = wmtrnet->GetXmit(i); j > 0; j-- ) {
			std::cout << " " << v[SAVED_PINGS-j];
		}
		std::cout << std::endl;
   
	}

	return inconsist_count;
}


int WinMTRDialog::ShowTraceTable()
{
	char buf[255], nr_crt[255];
	int nh = wmtrnet->GetMax();
	int inconsist_count = 0;
	for(int i=0;i <nh ; i++) {

		wmtrnet->FmtAddr(i, buf);
		const char *cons = wmtrnet->GetConsistency(i);
		if ( *cons == 'i' )
			inconsist_count++;
		std::cout << buf << cons << " ";
		
		int *rv = wmtrnet->GetSavedRTTs(i);
		__int8 *tv = wmtrnet->GetSavedTTLs(i);
		MPLSStack *mv = wmtrnet->GetSavedMPLS(i);
#ifdef WIN32
		int xmit = wmtrnet->GetXmit(i);
		for (int j = 0; j < xmit; j++ ) {
			std::cout << " " << rv[j];
			if ( rep_ttl ) {
				std::cout << "/" << (int)tv[j];
			}
			if ( rep_icmp_status ) {
				std::cout << "/" << (int)mv[j].status;
			}
			if ( rep_mpls ) {
				mv[j].Format(buf, sizeof(buf));
				std::cout << " " << buf;
			}
		}
#else
		for (int j = wmtrnet->GetXmit(i); j > 0; j-- ) {
			std::cout << " " << rv[SAVED_PINGS-j];
			if ( rep_ttl ) {
				std::cout << "/" << (int)tv[SAVED_PINGS-j];
			}
			if ( rep_icmp_status ) {
				std::cout << "/" << (int)mv[SAVED_PINGS-j].status;
			}
			if ( rep_mpls ) {
				mv[SAVED_PINGS-j].Format(buf, sizeof(buf));
				std::cout << " " << buf;
			}
		}
#endif
		std::cout << std::endl;
   
	}

	return inconsist_count;
}

int WinMTRDialog::InitMTRNet() {
	struct hostent *host, *lhost;
	int net_preopen_result;
	//made member int traddr;
	//made member int localaddr;
	const char *Hostname = defaulthostname;
	//char buf[255];

	net_preopen_result = wmtrnet->Preopen(this);

	if (net_preopen_result != 0) {
		errMsg("Unable to get raw socket!");
		return ES_RAWSOCK_CANT;
	}

	if (Hostname == NULL)
		Hostname = "localhost";

	int isIP=1;

	const char *t = Hostname;

	while (*t) {
		if (!isdigit(*t) && *t!='.') {
			isIP=0;
			break;
		}

		t++;
	}

	if (!isIP) {
		// INSECURE:: sprintf(buf, "Resolving host %s...", Hostname);
		host = gethostbyname(Hostname);

		if (host == NULL) {
			errMsg("Unable to resolve hostname.");
			return ES_HOSTNAME_CANT;
		}

		traddr = *(int *)host->h_addr;
	} else
		traddr = inet_addr(Hostname);

	lhost = gethostbyname("localhost");
	if (lhost == NULL) {
		errMsg("Unable to get local IP address.");
		return ES_LOCALHOST_CANT;
	}

	localaddr = *(int *)lhost->h_addr;
	// INSECURE:: sprintf(buf, "Tracing route to %s...", Hostname);
	if (wmtrnet->Open(traddr, localaddr) != 0) {
		errMsg("Unable to get raw socket!");
		return ES_RAWSOCK_FAIL;
	}

	return 0;
}


#ifndef WIN32
void PingThread(void *p) {
	WinMTRDialog *wmtrdlg = (WinMTRDialog *)p;
	fd_set readfd;
	int maxfd;
	int netfd;
	int NumPing;
	struct timeval lasttime, thistime, selecttime;
	int dt;
	int anyset = 0;

	NumPing = 0;
	
	#ifdef WIN32
		gettimeofday(&lasttime, reinterpret_cast<struct timezone *>(0));
	#endif

	while (wmtrdlg->start) {
		dt = wmtrdlg->wmtrnet->CalcDeltatime();
		intervaltime.tv_sec  = dt / 1000000;
		intervaltime.tv_usec = dt % 1000000;

		FD_ZERO(&readfd);
		maxfd = 0;

		netfd = wmtrdlg->wmtrnet->GetWaitFd();
		FD_SET(netfd, &readfd);

		if (netfd >= maxfd)
			maxfd = netfd + 1;

		if (anyset) {
			selecttime.tv_sec = 0;
			selecttime.tv_usec = 0;
		} else {
			//wmtrdlg->DisplayRedraw();

			gettimeofday(&thistime, reinterpret_cast<struct timezone *>(0));

			if ((thistime.tv_sec > (lasttime.tv_sec + intervaltime.tv_sec)) ||
			                ((thistime.tv_sec == (lasttime.tv_sec + intervaltime.tv_sec)) &&
			                 (thistime.tv_usec >= (lasttime.tv_usec + intervaltime.tv_usec))
			                )
			   ) {
				lasttime = thistime;

				if ( NumPing < wmtrdlg->max_att ) {
	 				if (wmtrdlg->wmtrnet->SendBatch())
						NumPing++;
					//std::cout << "dlg: sent batch, NumPing=" << NumPing << std::endl;
				} else {
					wmtrdlg->start = !wmtrdlg->start;
				}
			}

			selecttime.tv_usec = (thistime.tv_usec - lasttime.tv_usec);

			selecttime.tv_sec = (thistime.tv_sec - lasttime.tv_sec);

			if (selecttime.tv_usec < 0) {
				--selecttime.tv_sec;
				selecttime.tv_usec += 1000000;
			}

			selecttime.tv_usec = intervaltime.tv_usec - selecttime.tv_usec;
			selecttime.tv_sec = intervaltime.tv_sec - selecttime.tv_sec;

			if (selecttime.tv_usec < 0) {
				--selecttime.tv_sec;
				selecttime.tv_usec += 1000000;
			}


		}

		select(maxfd, &readfd, NULL, NULL, &selecttime);

		anyset = 0;

		if (FD_ISSET(netfd, &readfd)) {
			wmtrdlg->wmtrnet->ProcessReturn();
			anyset = 1;
			//std::cout << "dlg: processed return" << std::endl;
		}
	}
	//wmtrdlg->DisplayRedraw();

	//std::cout << "dlg: finished" << std::endl;
	wmtrdlg->wmtrnet->EndTransit();
	//std::cout << "dlg: ended" << std::endl;
	wmtrdlg->wmtrnet->Close();
	//std::cout << "dlg: closed" << std::endl;

//#ifndef linux
#if 0
	_endthread();
#endif
}
#endif

#ifdef WIN32
//debug
#include <stdio.h>
void WinMTRDialog::ShowDebugBuf() {
	printf("%d\n", wmtrnet->data_len);
	for (int i=0; i<sizeof(wmtrnet->data); i++ ) {
		printf("%02x ", ((unsigned char *)wmtrnet->data)[i]);
		if ( i % 16 == 15 ) {
			putchar('\n');
		}
	}
}

void WinMTRDialog::SetDebugTTL(int ttl) {
	wmtrnet->sample_info_ttl = ttl;
}

void WinMTRDialog::SetDebugOfs(int ofs) {
	wmtrnet->sample_ofs = ofs;
}
#else
void WinMTRDialog::ShowDebugBuf() {
	;
}

void WinMTRDialog::SetDebugTTL(int ttl) {
	;
}

void WinMTRDialog::SetDebugOfs(int ofs) {
	;
}

#endif	// WIN32
