//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef OMIBLKBITVECTTRAV_H
#define OMIBLKBITVECTTRAV_H


// Begin local Includes
#include <mem_AllocationOperatorsTiny.h>
#include <omi_BlkBitVect.h>


///////////////////////////////////////////////////////////////////////////////
//
// Name      : omi_BlkBitVectTrav class
//
// Description:
//
//     The omi_BlkBitVectTrav class provides that basis for traversals of 
//     omi_BlkBitVects. It is a friend of the omi_BlkBitVect in that its 
//     implementation works on the actual omi_BlockedArray data entities.  
//     When a omi_BlkBitVectTrav object is instantiated for a given omi_BlkBitVect, 
//     the omi_BlkBitVectTrav obtains only pointers to the omi_BlkBitVect data 
//     and does not copy the list data.  This allows an application to modify 
//     a omi_BlkBitVect while it is being traversed with the caviat
//     that the current data block of the traversal cannot be freed.
//
// Author:
//        
//     Phil Greene
//
// Creation date: 3-25-1999
//

class omi_BlkBitVectTrav  : public mem_AllocationOperatorsTiny
{
   protected:
      int  blockCount;
      uint  blockSize;
      uint  blockOrder;
      uint  blockModulus;
      uint  blockBitCount;
      const uint  **blockPointers;
      int  currentBlock;
      const uint  *currentCell;
      const uint  *lowBlockPicket;
      const uint  *highBlockPicket;
      uint  currentBitMask;
      uint  currentIndex;
      void Initomi_BlkBitVect();

   public:
      omi_BlkBitVectTrav( const omi_BlkBitVect *reflist );

      omi_BlkBitVectTrav( const omi_BlkBitVectTrav& trav );

      virtual ~omi_BlkBitVectTrav();

      bool First( uint& index );
      bool Last( uint& index );
      bool SetCurrent(uint &index, bool forward=true);

      bool Next( uint& index );
      bool Prev( uint& index );

      omi_BlkBitVectTrav& operator=( const omi_BlkBitVectTrav& trav );

      inline bool GetBit( uint index ) const {
         uint mask = 1<<(index&31);
         index>>=5;
         uint offset = index>>blockOrder;
         const uint *block = offset < blockCount ?
                             blockPointers[offset] : 0;
         return (block != 0 ) ? (bool)((block[index & blockModulus]&mask)!=0)  : false;
      }
          
       inline uint Index() const { return currentIndex; }

       inline bool More() const { return (currentBlock<blockCount); }

       inline bool Current( uint& index ) const { 
	  index = currentIndex;
	  return More();
       }


   friend void SetBitTraverseData( const omi_BlkBitVect *flist, 
                                omi_BlkBitVectTrav *trav );
};

#endif
