//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: omu_ObjVarray.h
// 
// Author: Uttam Narsu
// 
// Creation date: Mon Mar 11 1996
//
// Purpose: A very thin templated (adds typing) wrapper over the omu_BaseVarray
//          class which stores void*'s.
//
// Note: the usage is to declare omu_ObjVarray<CLASS>, *not* omu_ObjVarray<CLASS*>.
//
// This file contains the definition of the omu_ObjVarray class.
//

#ifndef omu_ObjVarray_h
#define omu_ObjVarray_h

//
// Forward Declarations
//

template <class T>
class omu_ObjVarray;

//
// Includes
//
#include <assert.h>

// Begin local includes

#include <omu_BaseVarray.h>

//
// Constants
//

//
// Class definition
//

template <class T>
class omu_ObjVarray : public omu_BaseVarray
{
public:

    omu_ObjVarray();
    omu_ObjVarray(uint initialSize, double gr=2.0);

    // Remove(Length-1); return voidValue_ if empty    
    T* Pop()
    { 
	return (T*) pop();
    }

    // False iff no valid value found.
    bool Get(uint i, T ** vp) const
    {
	return get(i, (omu_Ptr*) vp);
    }

    // Succeeds iff -- do --
    bool Set(uint i, T* v)
    {
	return set(i, (omu_Ptr)v);
    }
 
    // Add one past end
    bool Add(T* v)
    {
	return add((omu_Ptr)v);
    }

    // Subsequent elements shifted up
    bool Insert(uint i, T* v)
    {
	return insert(i, (omu_Ptr)v);
    }

    // Fails iff no valid value found.
    bool Remove(uint i, T ** vp = 0)
    {
	return remove(i, (omu_Ptr*) vp);
    }

    // 0 when empty
    uint Length() const
    {
	return length();
    }

    // Length becomes 0
    void ClearAll()
    {
	clearAll();
    }

    // Return Length() on failure.
    uint FindFirst(const T*& v) const
    {
	return findFirst((const omu_Ptr&)v);
    }

    // Return Length() on failure.
    uint FindLast(const T*& v) const
    {
	return findLast((const omu_Ptr&)v);
    }

    // Caller must delete array afterwards
    T* GetAll() const
    {
	return (T*) getAll();
    }

    // Add if necessary
    T*& operator[](uint i)
    {
	return (T*&)(omu_BaseVarray::operator[](i));
    }

    // Return voidvalue if invalid
    const T* operator[](uint i) const
    {
	return (const T*)(omu_BaseVarray::operator[](i));
    }


private:

    // Copy constructor and assignment operator declared private.
    omu_ObjVarray(const omu_ObjVarray<T>&);
    omu_ObjVarray<T>& operator=(const omu_ObjVarray<T>&);
};

#define OMU_OBJVARRAY_DECL(TYPE, NAME)\
typedef omu_ObjVarray<TYPE> NAME;

#endif  // #ifdef omu_ObjVarray_h
