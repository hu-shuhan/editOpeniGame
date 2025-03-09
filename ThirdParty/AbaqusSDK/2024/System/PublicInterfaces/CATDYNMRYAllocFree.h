#ifndef CATDYNMRYAllocFree_H
#define CATDYNMRYAllocFree_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
//============================================================================
// COPYRIGHT DASSAULT SYSTEMES 1997
//============================================================================
//
// CATDYNMRYAllocFree:
// The global functions are provided as the only published interface
// to memory management.
// Classes using the Memory Management have not to care about
// implementation details
// (to protect them from changes...)
//
//============================================================================
// 
// Usage:
// CATDYNMRYAlloc is used to implement the "operator new" of root classes,
// but might also be used for other small allocations
// (arrays, strings, or any thing else).
//
// CATDYNMRYFree is used to implement symetrically the "operator delete"
// of root classes, and to release any allocation.
//
//============================================================================
// Mar. 97 Initial coding    Ph.Baucher
//============================================================================

#define CATDYNMRY_NO_POOL
#include <stddef.h>
#include <stdlib.h>
#include "CO0LSTPV.h"
#ifdef _CAT_ANSI_STREAMS
/** @c++ansi aew 2004-08-02.20:05:12 [Replace forward declaration of standard streams with iosfwd.h] **/
 #include "iosfwd.h" 
#else //!_CAT_ANSI_STREAMS 
class ostream;
#endif //_CAT_ANSI_STREAMS

// ALLOC
#ifdef CATDYNMRY_NO_POOL
inline void* CATDYNMRYAlloc( size_t iRequiredSizeInBytes )
{ return malloc( iRequiredSizeInBytes ); }
#else
ExportedByCO0LSTPV void* CATDYNMRYAlloc( size_t iRequiredSizeInBytes );
#endif
//
// FREE
#ifdef CATDYNMRY_NO_POOL
inline void CATDYNMRYFree( void* iAddrMem )
{ free( iAddrMem );iAddrMem = NULL; }
#else
ExportedByCO0LSTPV void CATDYNMRYFree( void* iAddrMem );
#endif
//
// STATS
//ExportedByCO0LSTPV void CATDYNMRYStats( ostream& ioOS );
#endif
