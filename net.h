//*****************************************************************************
// FILE:            net.h
// derived from WinMTRNet.h (WinMTR 0.8)
//
//    Copyleft (C) 2000-2002, Vasile Laurentiu Stanimir ( stanimir@cr.nivis.com )
//    Modified 2009, Steve Harvey (sgh@vex.net)
//
// DESCRIPTION:
//
//  In some ways, this code is still a black box.
//
// NOTES:
//
//
//*****************************************************************************

#ifndef WINMTRNET_H_
#define WINMTRNET_H_


#include "dialog.h"

//! per RFC 792, p14
struct ICMPHeader {
	__int8 type;
	__int8 code;
	__int16 checksum;
	__int16 id;
	__int16 sequence;
};

//! characteristics common to a specific hop count in a repeated traceroute
struct s_nethost {
	__int32 addr;				//!< the last known IP address (0=unknown)
	int inconsist;				//!< set if the IP address has ever been inconsistent
						//!< \todo { make inconsist a bool }
	int xmit;				//!< number of outgoing packets already sent with the associated TTL
	int returned;				//!< number of outgoing packets which elicited a response
	unsigned long total;			//!< total of Round Trip Times for all replies in microseconds
	int last;				//!< Round Trip Time for last received reply in microseconds
	int best;				//!< lowest Round Trip Time for all received replies
	int worst;				//!< highest Round Trip Time for all received replies
	int transit;				//!< 1=in transit, 0=reply received
						//!< \todo { make transit a bool }
	int saved[SAVED_PINGS];			//!< most recent saved RTT is last in the array, -1=pending reply, -2=never sent
	char name[255];				//!< hostname or dotted-quad if not resolvable, or ICMP code or empty
};

//! a collection of these relates incoming reply packets to what was sent
struct s_sequence {
	int index;				//!< is the hop number (TTL-1) starting from 0
	int transit;				//!< 1=in transit, 0=reply received
						//!< \todo { make transit a bool }
	int saved_seq;				//!< number of outgoing packets already sent with the associated TTL

	struct timeval time;			//!< when the outgoing packet was sent
};

/** Network operations
  */
class WinMTRNet {

public:

	//! initialization 
	//! \return -1 if unable to set up socket, otherwise 0
	int		Preopen(WinMTRDialog *wp);

	//! reset all saved state and set local and remote addresses
	//! \param address is where we are tracerouting to
	//! \param laddress is our local IP address
	//! \return always 0
	//! \todo { make this void }
	int		Open(int address, int laddress);

	//! (re)-initialize all saved state
	void		Reset();

	//! close open socket(s)
	void		Close();

	//! return the file descriptor for the receive socket. \sa PingThread
	int		GetWaitFd();

	//! invoked when a packet is available on the receive socket
	void		ProcessReturn();

	// the following get bits of info for display purposes
	int		GetLast(int at);
	int		GetMax();
	int		GetAddr(int at);
	const char*	GetConsistency(int at);
	char*		FmtAddr(int at, char *n);
	char*		GetName(int at, char *n);
	void		SetName(int at, char *n);
	int		GetPercent(int at);
	int		GetBest(int at);
	int		GetWorst(int at);
	int		GetAvg(int at);


	//! send one ping probe in the current batch
	//! \return 1 if we are at the end of the batch
	int		SendBatch();

	//! mark all entries in host table as completed (no longer in transit)
	void		EndTransit();

	//! calculate Deltatime? as the WaitTime divided by numhosts, in microseconds
	int		CalcDeltatime();

	int		GetReturned(int at);
	int		GetXmit(int at);
	int		GetTransit(int at);

	int*		GetSavedPings(int at);

	int		GetNumHosts() { return numhosts; };
	BOOL		FullLoop() { return full_loop; };

	//! reserve a free entry at the end of the RTT saved table
	//! \param at is the hop count (TTL-1) starting at 0
	void		SaveXmit(int at);

	//! save a Round Trip Time
	//! \param at is the 0-based hop count (varies fastest)
	//! \param seq is which batch of pings is being done (varies slowest)
	//! \param ms is the Round Trip Time in microseconds
	void		SaveReturn(int at, int seq, int ms);


	float WaitTime;

	WinMTRDialog *wmtrdlg;

private:

	unsigned short Checksum(unsigned short  *lpBuf, int nLen) ;

	//! Allocate a new entry in the sequence table.
	//! \param index is the hop number (TTL-1) starting at zero
	//! \return the index in the sequence table. \sa sequence
	int GetNewSequence(int index);

	//! send a single probe packet
	//! \param index is the hop number (TTL-1) starting at zero
	void SendQuery(int index);

	//! process an ICMP reply which may be an echo reply, time exceeded, or an unreachable
	//! \param seq is the sequence number of the corresponding outgoing packet
	//! \param addr is the IP addr
	//! \param now is the time of the packet receipt
	void ProcessPing(int seq, __int32 addr, struct timeval now);

	//! wrapper for socket close
	void skt_close(int skt);

	int packetsize;				//!< set by Preopen, used by SendQuery

	struct s_nethost host[MaxHost];		//!< one per possible hop

	struct s_sequence sequence[MaxSequence]; //!< one per outgoing ping

	int sendsock;
	int recvsock;

	struct sockaddr_in remoteaddress;
	struct sockaddr_in localaddress;

	int batch_at;				//!< current hop number (TTL-1) in the current batch
						//!< \todo { rename batch_at }
	int numhosts;				//!< presumed number of hops in traceroute. Initially 10, then refined

	BOOL full_loop;				//!< set when the number of hops in the traceroute is first determined
};

#endif	// ifndef WINMTRNET_H_
