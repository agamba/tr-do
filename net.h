#ifdef WIN32
#include "net90.h"
#define WinMTRNet WinMTRNet90
#else
#define WinMTRNet WinMTRNet80
#include "net80.h"
#endif

