#ifndef CATHashCodeCollec_h
#define CATHashCodeCollec_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATCollecRoot.h"
#include "CO0LSTPV.h"

class CATHashCodeIter ;


/**
 * Base class for hash-tables and hash-dictionary.
 */
class	ExportedByCO0LSTPV	CATHashCodeCollec	: public CATCollecRoot
{	protected :
/**
 * Constructs a empty instance.
 */
		CATHashCodeCollec ()	{}
		~CATHashCodeCollec ();

/**
 * Gets a better dimension for an hash table.
 * @param iN The number of items in the hash table.
 * @return The optimized value.
 */
		static
		  unsigned int	UpToNiceModulo	( unsigned int	iN );


	private :

/**
 * to go from item to item in the hash-code collection.
 */
		virtual
		  void*		NextItem( int&		ioBucket
					, int&		ioPosition )	const	= 0 ;

/**
 * to allow iterator objects to use the above private virtual function
 */
	friend
	  class CATHashCodeIter ;
};

#endif		/* CATHashCodeCollec_h */
