//*****************************************************************************
// FILE:            global.h
// derived from WinMTRGlobal.h (WinMTR 0.8)
//
//    Copyleft (C) 2000-2002, Vasile Laurentiu Stanimir ( stanimir@cr.nivis.com )
//    Modified 2009, Steve Harvey (sgh@vex.net)
//
// DESCRIPTION:
//
//
// NOTES:
//
//
//*****************************************************************************

#ifndef GLOBAL_H_
#define GLOBAL_H_

#define VC_EXTRALEAN

#if defined (linux) || defined(__APPLE__)
typedef unsigned char __int8;
typedef unsigned short __int16;
typedef unsigned int __int32;
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifndef BOOL
#define BOOL bool
#define TRUE 1
#define FALSE 0
#endif

#if defined (linux) || defined(__APPLE__)
#include <unistd.h>
#define Sleep(x)	usleep((x)*1000)
#include <sys/time.h>
#else
#include <process.h>
#include <io.h>
#endif
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>

#ifdef WIN32
//#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "WS2_32.Lib")
#endif

// exit status codes
#define ES_NORMAL		0	// no errors
#define ES_INCONSIST		1	// traceroute got inconsistent host addrs
#define ES_INITWINSOCK		2	// Win32: unable to initialize WinSock
#define ES_UNKOPTION		3	// unknown command line option
#define ES_RAWSOCK_CANT		4	// can't get raw socket (PreOpen)
#define ES_HOSTNAME_CANT	5	// can't resolve hostname
#define ES_LOCALHOST_CANT	6	// can't resolve localhost
#define ES_RAWSOCK_FAIL		7	// can't get raw socket (Open)
#define ES_SETSOCKOPT_FAIL	8	// setsockopt failed
#define ES_BAD_HOSTNAME		9	// missing or invalid hostname

#define WMTRCMD_VERSION	"0.1"
#define WMTRCMD_LICENSE	"GPL - GNU Public License"
#define WMTRCMD_ORIG_COPYRIGHT "Copyleft @2000-2002, Vasile Laurentiu Stanimir (stanimir@cr.nivis.com)"
#define WMTRCMD_COPYRIGHT "Copyright 2009 Steve Harvey (sgh@vex.net)"

#define DEFAULT_PING_SIZE	64
#define DEFAULT_INTERVAL	1.0
#define DEFAULT_DNS		TRUE
#define DEFAULT_MAX_ATT		5

#define SAVED_PINGS 100
#define MaxHost 256			//! maximum+1 number of hops, sizes host table
//#define MaxSequence 65536
#define MaxSequence 32767		//! maximum sequence number for ICMP headers and sizes sequence table
//#define MaxSequence 5

#define MAXPACKET 4096			//! maximum out-going packet size: IPv4 header + ICMP + payload
#define MINPACKET 64			//! minimum out-going packet size: IPv4 header + ICMP + payload

#define MaxTransit 4


#define ICMP_ECHO		8
#define ICMP_ECHOREPLY		0

#define ICMP_TSTAMP		13
#define ICMP_TSTAMPREPLY	14

#define ICMP_TIME_EXCEEDED	11

#define ICMP_HOST_UNREACHABLE 3

#define MAX_UNKNOWN_HOSTS 10

#define IP_HEADER_LENGTH   20


#define MTR_NR_COLS 9

const char MTR_COLS[ MTR_NR_COLS ][10] = {
	"Hostname",
	"Nr",
	"Loss %",
	"Sent",
	"Recv",
	"Best",
	"Avrg",
	"Worst",
	"Last"
};

const int MTR_COL_LENGTH[ MTR_NR_COLS ] = {
	190, 30, 50, 40, 40, 50, 50, 50, 50
};

#if defined (linux) || defined(__APPLE__)
// extern "C" {
//	int gettimeofday(struct timeval* tv, struct timezone *tz);
// }
typedef int clockid_t;
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

// anto commented all the declarations of gettimeofday: assumng that it i defined already
extern "C" {
	// Anto reverted
//	int gettimeofday(struct timeval* tv, struct timezone *tz);
	//int clock_gettime (clockid_t clk_id, struct timespec *tp);
}

#else
// ANTO reverted
//int gettimeofday(struct timeval* tv, struct timezone *tz);

//int clock_gettime (clockid_t clk_id, struct timespec *tp);
#endif


#endif // ifndef GLOBAL_H_
