#ifndef CATConstantForThreads_h
#define CATConstantForThreads_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATDataType.h"
#include "JS0MT.h"
#ifndef _WINDOWS_SOURCE
//#include <stdlib.h>
#include <pthread.h>
#endif
//CATMutex
//=============================================================================
#define CATMutexSuccessful 0
#define CATMutexFailure    1

// CATThreads
//=============================================================================
#define CATThreadsCreated      0x0001
#define CATThreadsInitializing 0x0002
#define CATThreadsRunning      0x0004
#define CATThreadsWaiting      0x0008
#define CATThreadsEnded        0x0010
#define CATThreadsJoinable     0x0020

#define CATThreadsSuccessful 0
#define CATThreadsFailure    -1
#define CATThreadsRCUnavailable    1

typedef  CATLONG32  CATThreadsReturnCode;

#if defined(_DARWIN_SOURCE) || defined(_LINUX_SOURCE) || defined(_IOS_SOURCE)
typedef pthread_t CATThreadsId;
#else // All Os apart MacOS
typedef CATULONG32  CATThreadsId;
#endif

 

#endif
