//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- mode: c++ -*- //
#ifndef SMABasSrmForwarder_h
#define SMABasSrmForwarder_h

/// This class resides in SMABase (very low level of ABAQUS), and forwards calls
/// to the SRM (which resides in SMAAmm). This allows us to call SRM from anywhere
/// in ABAQUS. Specifically, this is used in Low-Level Pool-based allocators
/// residing in SMABasMem and servicing standard containers (cow, omi and generics).
/// These allocators need to operate with or without the SRM system available.

// Begin local includes
#include <mem_AllocationOperators.h>
#include <SMABasSrmResourceEnums.h>
#include <SMABasCamPool.h>
// End local includes

class  SMABasSrmForwarder : public mem_AllocationOperators
{
  typedef void (*funcPtrVoid)();
  typedef void (*funcPtrChar)(const char* phaseName); 
  typedef SMABasCamPool* (*funcPtrGetPool)(srm_ResourceEnums resourceId); 

 public:
   SMABasSrmForwarder();
  ~SMABasSrmForwarder();

  static void            MemorySummary();
  static void            MemorySummary(const char * phaseName);
  static SMABasCamPool*  GetPool(srm_ResourceEnums resourceId);
  static void            Initialize();

protected:

  static void            SetPtrMemorySummary(funcPtrVoid);       
  static void            SetPtrMemorySummary(funcPtrChar); 
  static void            SetPtrGetPool(funcPtrGetPool); 

private:

  static void           (*pMemorySummary) ();         /// function pointer to MemorySummary() in SRM
  static void           (*pMemorySummaryPhase) (const char* phaseName); ///  MemorySummary(phaseName)
  static SMABasCamPool* (*pGetPool) (srm_ResourceEnums resourceId); /// function pointer to GetPool() in SRM
};


#endif // SMABasSrmForwarder_h
