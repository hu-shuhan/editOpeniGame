#ifndef CATHashCodeIter_h
#define CATHashCodeIter_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 1999


#include "CATHashCodeCollec.h"
#include "CO0LSTPV.h"

class CATHashTable;
class CATHashDico;
class CATHashDicoS;

class ExportedByCO0LSTPV CATHashCodeIter : public CATCollecRoot { 
	public :

	/**
         * @nodoc
	 * Constructor.
	 * @param iHCollec 
         *   The @href CATHashTable for the iterator.
	 */
	CATHashCodeIter ( const CATHashTable& iHCollec ); 

	/**
         * @nodoc
	 * Constructor.
	 * @param iHCollec 
         *   The @href CATHashDico for the iterator.
	 */
	CATHashCodeIter ( const CATHashDico & iHCollec );

	/**
         * @nodoc
	 * Constructs an iterator.
	 * @param iHCollec 
         *   The @href CATHashDicoS for the iterator.
	 */
	CATHashCodeIter ( const CATHashDicoS& iHCollec ); 

	/**
	 * Destructor.
	 */
	~CATHashCodeIter ();

	/**
	 * Postfix increment operator.
	 * @return The next element of the collection.
	 */
	void*  operator++( int ) { 
		return _HCollec->NextItem( _Bucket, _Position ); 
	}

	/**
	 * Reset the iterator to the begining of the collection.
	 */
	void  Beginning(); 

	private :
		/** @nodoc 
		 *the hash-code collection to work on.
		 */
		const CATHashCodeCollec* _HCollec;

		/** @nodoc 
		 * the bucket designates the current entry in the hash-code collection.
		 */
		int _Bucket;

		/** @nodoc 
		 * the position designates the current item in the bucket.
		 */
		int _Position;

		friend class CATHashTable;
		friend class CATHashDico;
		friend class CATHashDicoS;
};

#endif  /* CATHashCodeIter_h */
