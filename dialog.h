//*****************************************************************************
// FILE:            dialog.h
// derived from WinMTRDialog.h (WinMTR 0.8)
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

#ifndef WINMTRDIALOG_H_
#define WINMTRDIALOG_H_

/** pseudo-dialog.  It was a real dialog in the original WinMTR. 
  */
class WinMTRDialog {

public:
	WinMTRDialog();

	int InitMTRNet();

	//! show tabular traceroute data in original WinMTR format
	int DisplayRedraw();

	//! show the traceroute table in IXmaps TrGen canonical format
	int ShowTraceTable();

	//! set to enable traceroute
	bool start;

	//! wait time for a batch of pings, nominally 1.0 seconds
	double interval;

	//! size of the outgoing ping probe IPv4+ICMP+payload
	int pingsize;
	bool useDNS;

	//! number of ping batches to run
	int max_att;

	void SetHostName(const char *host);
	void SetInterval(float i);
	void SetPingSize(int ps);
	void SetUseDNS(BOOL udns);

	//! set the number of probes for each hop
	void SetMaxAttempts(int nprobes);

	//! display an error message on standard output
	void errMsg(const char *msg);

	int restart(const char *Hostname);

protected:

	char defaulthostname[1000];

};

#endif // ifndef WINMTRDIALOG_H_
