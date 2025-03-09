#ifndef CATCollecRoot_h
#define CATCollecRoot_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include <stddef.h>
#include "CATDYNMRYAllocFree.h"
#include "CO0LSTPV.h"

/**
* Base class for collection.
* <b>Role:</b>Do not use. 
*/
class ExportedByCO0LSTPV CATCollecRoot {
	public :
		/** @nodoc */
		inline CATCollecRoot ();

		/** @nodoc */
		~CATCollecRoot ();

#ifndef CATDYNMRY_NO_POOL
		/** @nodoc */
		inline void* operator new( size_t iSize );

		/** @nodoc */
		inline void  operator delete( void* iAddr );
#endif

	protected :
		/** @nodoc */
		inline CATCollecRoot( const CATCollecRoot& );

		/** @nodoc */
		inline void operator=( const CATCollecRoot& );
};

/**
 * @nodoc
 */
inline CATCollecRoot::CATCollecRoot ()
{
}
#ifndef CATDYNMRY_NO_POOL
inline void* CATCollecRoot::operator new( size_t iSize )
{
  return CATDYNMRYAlloc( iSize ); 
} 
inline void CATCollecRoot:: operator delete( void* iAddr )
{
  CATDYNMRYFree( iAddr ); 
} 
#endif
inline CATCollecRoot::CATCollecRoot( const CATCollecRoot& )
{
} 
inline void CATCollecRoot::operator=( const CATCollecRoot& )
{
}

#endif
