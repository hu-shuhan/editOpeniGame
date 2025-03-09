//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef BLOCKED_BIT_VECTOR_H
#define BLOCKED_BIT_VECTOR_H


// Begin local Includes

#include <omi_types.h>
#include <omi_BlockedArray.h>

class omi_BlkBitVectTrav;

// generator for BitPerByte
//   int i, j, cnt;
//   for( i=0; i<256; i++ ) {
//      cnt = 0;
//      for( j=0; j<8; j++ ) if( i&(1<<j) ) cnt++;
//      printf( "%d, ", cnt );
//      if( (i&15) == 15 ) printf("\n");
//   }

// simple method to count the number of bits in a given int
// looks at each byte and does a table lookup based on the
// byte value
inline uint CountBits32( uint val )
{
   static const uint BitPerByte[256] = {
       0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
       1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
       2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
       3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
       4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

   uchar *ptr = (uchar*)&val;
   return (BitPerByte[*ptr] + BitPerByte[ptr[1]] +
           BitPerByte[ptr[2]] + BitPerByte[ptr[3]]);
}



///////////////////////////////////////////////////////////////////////////////
//
// Name      : class omi_BlkBitVect
//
// Description:
//
//     This class inherits a uint template of the omi_BlockedArray.
//
//     This class supports methods to build and manipulate bit vectors.  The
//     underlying implementation uses a blocked array structure so no 
//     reallocation is needed as vectors grow.  Also, storage for sparse bit 
//     vectors is reduced as only blocks containing bits are allocated.  
//     Users can determine  block size (power of 2) on instantiation of a 
//     omi_BlkBitVect.   Assignment and boolean assigment operators have 
//     been overloaded.
//
//
// Author:
//        
//     Phil Greene
//
// Creation date: 3-25-1999
//

class omi_BlkBitVect : public omi_BlockedArray<uint>
{
   private:
      uint bitCount;
      bool dirty;
      /// Scan all blocks and count the bits.  All bitless blocks are deleted.
      /// Bit count is set.
      void Recount(); //reset bitCount after a bulk operation

   public:
      /// Default constructor
      omi_BlkBitVect();
      /// Copy constructor
      omi_BlkBitVect( const omi_BlkBitVect& bvec );
      /// Destructor
      virtual ~omi_BlkBitVect();
      /// Dump the contents of the vector. Useful for debugging.
      virtual void DumpList() const;
      /// Sets bit ON.  Increments bitCount if bit was not already ON.
      void SetBit( uint index );
      /// Sets specified bit OFF.  Decrements bitCount if bit was ON.
      void UnsetBit( uint index );
      /// Turn on a range of bits
      void SetFromTo( uint first, uint last );
      /// Removes all allocated bit blocks and sets bitCount to 0
      void Clear();
      /// Assignment operator
      omi_BlkBitVect& operator=(const omi_BlkBitVect& bvec);
      /// Bitwise Intersection of the input bit vector with this.
      omi_BlkBitVect& operator&=(const omi_BlkBitVect& bvec);
      /// Bitwise Union of the input bit vector with this.
      omi_BlkBitVect& operator|=(const omi_BlkBitVect& bvec);
      /// Bitwise subtraction of the input bit vector with this.
      omi_BlkBitVect& operator-=(const omi_BlkBitVect& bvec);
      /// Bitwise exclusive union of the input bit vector with this.
      omi_BlkBitVect& operator^=(const omi_BlkBitVect& bvec);
      /// Compare specified bit vector with this one.
      /// Returns 0 if they are identical, 
      ///         1 if this is 'greater than' input bit vector
      ///        -1 if this is 'less than' input bit vector
      int Compare( const omi_BlkBitVect& bvec ) const;

      /// Returns true if input bit vector is fully contained in this; false otherwise.
      bool Contains(const omi_BlkBitVect& bvec) const;

      /// Bitwise Union of the input bit vector with this.
      inline omi_BlkBitVect& operator+=(const omi_BlkBitVect& bvec) {
         *this |= bvec; return *this;
      }
      /// Checks bit by bit for equality.
      bool operator==(const omi_BlkBitVect& bvec) const;

      /// Returns true if the input bit vector is different from this bit vector.
      bool operator!=(const omi_BlkBitVect& bvec) const {
          return !(*this == bvec);
      }
      /// Return true if the specified bit is set; false otherwise.
      bool operator[](uint index) const {
          return GetBit(index);
      }

      /// Returns true if the input bit vector has an overlap with this bit vector; false otherwise.
      bool HasOverlap(const omi_BlkBitVect& bvec) const;

      /// Return true if the specified bit is set; false otherwise.
      inline bool GetBit( uint index ) const {
	 uint* tmp = GetCell(index>>5);
	 uint cell = (tmp != 0) ? *tmp : 0;
         return ( (cell & (1<<(index&31))) != 0);
      }
      /// Return the total number of bits set.
      inline uint BitCount() const {
          if ( dirty )
          {
              omi_BlkBitVect *ncthis = const_cast<omi_BlkBitVect *>(this);
              ncthis->Recount();
          }
          return bitCount; 
      }

      /// Set bits identified by input mask to a specific cell 
      inline void SetCellBits( uint index, uint mask ) {
         uint* tmp = GetCell(index);
         uint cell = (tmp != 0) ? *tmp : 0;
         bitCount -= CountBits32(cell);
         bitCount += CountBits32(mask);
         SetCell(index,mask);
      }

      /// Add bits identified by input mask to a specific cell
      inline void AddCellBits( uint index, uint mask ) {
         uint* tmp = GetCell(index);
         uint cell = (tmp != 0) ? *tmp : 0;
         bitCount -= CountBits32(cell);
         cell |= mask;
         bitCount += CountBits32(cell);
         SetCell(index,cell);
      }

      /// Return the amount of occupied memory in bytes
      virtual size_t Footprint() const; 

      /// @nodoc
      friend void SetBitTraverseData( const omi_BlkBitVect *flist, 
                                      omi_BlkBitVectTrav *trav );

};




#endif
