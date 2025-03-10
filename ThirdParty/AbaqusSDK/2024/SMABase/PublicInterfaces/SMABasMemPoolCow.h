//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasMemPoolCow_h
#define SMABasMemPoolCow_h

//
// Standalone, self-sufficient memory pool, managing all allocations for all COW containers.
// 

// Begin local includes
#include <SMABasMemPool.h>
#include <omi_Singleton.h>
// End local includes




class  SMABasMemPoolCow: 
          public SMABasMemPool,
          public omi_Singleton<SMABasMemPoolCow>
{

public:

  SMABasMemPoolCow()  :
    SMABasMemPool(1001,"cow_containers",0) 
  { }

  ~SMABasMemPoolCow() { }

};

#endif // SMABasMemPoolCow_h
