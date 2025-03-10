/* -*-c++-*- */
// COPYRIGHT DASSAULT SYSTEMES	1999
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

#ifndef CATSysSimpleHashTable_H_
#define CATSysSimpleHashTable_H_

#include "CATSysTS.h"

typedef int (*PFCompare)(void*, void*) ;
typedef unsigned int (*PFHash)(void*) ;

ExportedByCATSysTS unsigned int CATSimpleHashIt(void * iObj) ;
ExportedByCATSysTS int CATSimpleHashCompare(void * iObj1, void * iObj2);

/**
 * HashTable implementation.
 */
class ExportedByCATSysTS CATSysSimpleHashTable {

	public:
		/**
		 * Constructs a CATSysSimpleHashTable.
		 * @param iSizeEstimate
		 * The initial estimated size of the hash table.
		 * @param iPFH
		 * The given hash function.
		 * @param iPFC
		 * The given elemts comparison function.
		 */
		CATSysSimpleHashTable(
					int iSizeEstimate = 0,
					PFHash iPFH = CATSimpleHashIt,
					PFCompare iPFC = CATSimpleHashCompare);

		/**
		 * Destructor.
		 */
		~CATSysSimpleHashTable() ;
  
		//--------------------------------
		// Direct handling of elements.
		//------------------------------
  
		/**
		 * Inserts the given element.
		 * Returns 0 if an element, equal to the given one, already exists in the hashtable,
		 * Returns 1 if the given element has been inserted successfully.
		 * Returns -1 in case of failure.
		 */
		int Insert(void* iObj);
  
		/**
		 * Inserts the given element and returns the element stored in the hashtable.
		 * If an element, equal to the given one, already exists in the hashtable, this method
		 * returns it, otherwise it returns the given element. Returns NULL in case of failure.
		 */
		void* InsertAndReturn(void* iObj);
  
		/**
		 * Returns the element stored next to the given element.
		 */
		void * Next(void* iFrom) const;

		/**
		 * Returns the next element corresponding to the same key of the given object.
		 */
		void * NextWithKey(void* iFrom) const;

		/**
		 * Returns the element stored in the hash table to the specified index.
		 */
		void * Get(int iPos) const;
  
		//-------------------
		// Hashed access.
		//-------------------
		
		/**
		 * Returns a pointer on associated object or NULL if object not found.
		 */
		void * Locate(void*) const;

		/**
		 * Returns a pointer on the BUCKET containing the associated object 
		 * or NULL if object not found.
		 */
		void * LocateBucket(void*) const;

		/**
		 * Returns a pointer on the first object corresponding to the given key,
		 * or NULL if not found.
		 */
		void * KeyLocate(unsigned int iKey) const;
  
		/**
		 * Removes the associated object from the hash table.
		 * Returns the removed object if successful, NULL otherwise.
		 */
		void * Remove(void*);

		/**
		 * Removes all the elements of the hash table.
		 */
		void RemoveAll();

		/**
		 * Returns the number of stored elements in the hash table.
		 */
		int Size() const;

		/**
		 * Print stats for the hash table.
		 */
		void PrintStats() const ;
  
		/**
		 * Reallocates the hashtable with the specified size.
		 * If no size is specified, the dimension of the hashtable is doubled.
		 * Returns 0 if successful, -1 in case of failure.
		 */
		int ReAllocate(int new_dim = 0) ;
		
	private:

		/*
		 * Instances of this classes are not copiable. I declare but don't define these, so that
		 * the compiler won't generate a default version of them for me, and also so that no one can
		 * use them by mistake.
		 */
		CATSysSimpleHashTable(const CATSysSimpleHashTable&);
		CATSysSimpleHashTable& operator=(const CATSysSimpleHashTable&);
	
		PFCompare _PFC; // Comparison function
		PFHash _PFH; // Hash function
		void ** _AllocatedZone; // address of the allocated memory  
		int _DimArray; // dimension of the hashtab (maximum number of bucket to be stored in the Hashtable)
		int _NbObjStocke; // number of bucket stored in the hashtable

		static int UpToNiceModulo(int);

		inline void ** NextEmptyBucket() const  // Address of the first free bucket
		{ return _AllocatedZone + _DimArray + 2 * _NbObjStocke; }
};

#endif // CATSysSimpleHashTable_H_


