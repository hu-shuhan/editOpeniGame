//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasAllocationTracking_h
#define SMABasAllocationTracking_h

//
// Special New and Delete used for Memory Allocation Tracking.
//

// Begin local includes
#include <SMABasMemPool.h>
// End local includes

extern SMABasMemPool  operatorNewPool;

void* mem_trackingNew(size_t size, const char* file);
void* mem_trackingNewNoThrow(size_t size, const char* file);
void  mem_trackingDelete(void* ptr, const char* file);

#endif // SMABasAllocationTracking_h
