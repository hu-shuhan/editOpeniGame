//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
//
// File Name: omu_BaseVarray.h
// 
// Author: Sudheer Apte
// 
// Creation date: Mon Mar 11 1996
//
// Purpose:
//
// This file contains the definition of the omu_BaseVarray class.
//

#ifndef omu_BaseVarray_h
#define omu_BaseVarray_h

//
// Forward Declarations
//

class omu_BaseVarray;

//
// Includes
//
#include <assert.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <mem_C_Allocation.h>
#include <omi_types.h>
#include <omu_types.h>

//
// Constants
//

//
// Class definition
//

// 
// Variable-length arrays of void*. Used as a base class by
// omu_ObjArray which adds a thin layer of typing (via templates)
// over this class.
//

class omu_BaseVarray  : public mem_AllocationOperators
{

public:
    
    void setGrowthRatio(double gr) {growthRatio_= gr;} // Set growth ratio
    bool setSize(uint size_in);  // Grow the Varray to this size
    uint size() const { return size_; }
    double growthRatio() const { return growthRatio_; }

protected:

    enum { DEF_ARRAY_SIZE = 16 };

    // The constructor and destructor now don't do any allocation
    // or deallocation. Those duties are now handled by the
    // derived, typed tables. This is because when constructing a
    // table from a stream, one needs to be able to read the size_
    // and chunkSize_ first, and then allocate that size array.
    omu_BaseVarray()
	: length_(0), data_(0), size_(0), growthRatio_(2.0),
	  chunkSize_(DEF_ARRAY_SIZE), voidValue_(0)
    { }

    omu_BaseVarray(uint initialSize, double gr=2.0)
	: length_(0), size_(0), data_(0),
	  chunkSize_(initialSize), voidValue_(0), growthRatio_(gr)
    { }

    // TODO: ownership issue. If the varray should call delete on
    // all the T*'s stored here, move this code to omu_ObjVarray,
    // and then call. Ask Sudhir.
    virtual ~omu_BaseVarray()
    {
	mem_Free(data_); data_ = 0;
	size_ = length_ = 0;
    }

    omu_Ptr& operator[](uint i);              // Add if necessary
    const omu_Ptr&  operator[](uint i) const  // Return voidvalue if invalid
    {
	assert (i < length_);
	return data_[i];
    }

    omu_Ptr pop();                    // Remove(Length-1) else voidvalue if empty

    bool get(uint i, omu_Ptr *vp) const
    {
     if (i >= length_)
	return false;
     if (vp == 0)
	return true;
    *vp = data_[i];
    return true; 
    }
    bool set(uint i, omu_Ptr v);
    bool add(omu_Ptr v);                       // Add one past end
    bool insert(uint i, omu_Ptr v);          // Subsequent elements shifted up
    bool remove(uint i, omu_Ptr *vp = 0);

    const uint& length() const               // 0 when empty
    { return length_; }

    void clearAll()                            // Length becomes 0
    { length_ = 0; }

    uint findFirst(const omu_Ptr& v) const;  // Return Length() on failure.
    uint findLast(const omu_Ptr& v) const;   // Return Length() on failure.

    omu_Ptr* getAll() const;          // Caller must delete array afterwards

//
// data members. Not private because omu_BaseVarray and
// omu_Varray<T> are really one interface/implementation pair.
//  
    uint      chunkSize_;    // Initial size
    double   growthRatio_;  // Grow by this factor every time.
    omu_Ptr  voidValue_;    // Add this value by default

    uint   length_;
    omu_Ptr *data_;
    uint   size_;

private:
    bool grow();

    // Copy constructor and assignment operator declared private.
    omu_BaseVarray(const omu_BaseVarray&);
    omu_BaseVarray& operator=(const omu_BaseVarray&);
};


#endif  // #ifdef omu_BaseVarray_h
