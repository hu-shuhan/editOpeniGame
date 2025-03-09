//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef cow_Copy_h
#define cow_Copy_h

// Below are the set of cow_Copy templates. Each provides a Copy method
// which creates a new copy of a value.

// The cow_Direct template simply invokes the copy constructor of the
// target class.
//

// Begin local includes
#include <mem_AllocationOperators.h>

template <class Type> struct cow_Direct  : public mem_AllocationOperators
{
    static Type* Copy(const Type &value);
};

// The cow_Virtual template calls the Copy method of the target class.
// This method should be declared virtual, and it should be defined as
// follows, in each target class, including derivatives of that class:
//
// Type* Deriv::Copy() const { return new Deriv(*this); }
//
template <class Type> struct cow_Virtual  : public mem_AllocationOperators
{
    static Type* Copy(const Type &value);
};

#endif  // #ifdef cow_Copy_h
