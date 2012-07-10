
#include <iostream>

#include "global.h"
#include "dialog.h"
#include "net.h"

int
main(int argc, char **argv) {
	const char *host_name;
	bool verbose = false;
	int inconsistent_count;
#ifdef WIN32
	WSADATA wsaData;
	int res;

	res = WSAStartup(MAKEWORD(2,2), &wsaData);
	if ( res != 0 ) {
		std::cerr << "unable to initialize Winsock" << std::endl;
		return ES_INITWINSOCK;
	}
#endif

	argc--; argv++;

	WinMTRDialog *dlg = new WinMTRDialog();
	while ( argc > 0 && (**argv == '-' || **argv == '/') ) {
		int nprobes;

		switch (argv[0][1]) {

		case 'q':
			nprobes = atoi(2+argv[0]);
			dlg->SetMaxAttempts(nprobes);
			break;

		case 'h':
			std::cout << "WMTRcmd " << WMTRCMD_VERSION << std::endl;
			std::cout << std::endl;
			std::cout << "Portable, command line only version of WinMTR." << std::endl;
			std::cout << "Use:  wmtrcmd  hostname" << std::endl;
			std::cout << "This program comes without any warranty, and is licenced" << std::endl;
			std::cout << "under the GNU Public License v2 ." << std::endl;
			return 0;

		case 'v':
			verbose = !verbose;
			break;

		default:
			std::cerr << "unknown option " << argv[0] << std::endl;
			return ES_UNKOPTION;
		}
		argc--; argv++;
	}
	if ( argc > 0 )
		host_name = *argv;
	else {
		std::cerr << "missing hostname or IP address" << std::endl;
		return ES_BAD_HOSTNAME;
	}
	int rc = dlg->restart(host_name);
	if ( rc != 0 )
		return rc;
	if ( verbose ) {
		inconsistent_count = dlg->DisplayRedraw();
	} else {
		inconsistent_count = dlg->ShowTraceTable();
	}
	return (inconsistent_count > 0) ? ES_INCONSIST : ES_NORMAL;
}

#ifdef WIN32
int gettimeofday(struct timeval *t, struct timezone *timezone) { 
	struct _timeb timebuffer;
	_ftime( &timebuffer );
	t->tv_sec=timebuffer.time;
	t->tv_usec=1000*timebuffer.millitm;
	return 0;
}
#endif
