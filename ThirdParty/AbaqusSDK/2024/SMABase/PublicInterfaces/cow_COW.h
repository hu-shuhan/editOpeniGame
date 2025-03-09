//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef cow_COW_h
#define cow_COW_h

// Begin local includes

#include <SMABasCowAllocator.h>
#include <cow_CountedHld.h>
#include <cow_Copy.h>

// End local includes

// This is the COW, Copy-On-Write agent template. It is manages a value
// of the parameter class with selected cow_Copy strategies.

template <class Type, class Dup> class cow_COW  
#if defined(SMA_TRACK_COW_ALLOC)
  : public SMABasCowTrackingAllocator 
#else
  : public SMABasCowAllocator 
#endif 
{

public:
    // This constructor constructs a COW by *COPYING* a value.
    //
    cow_COW(const Type& copy_me);

    // This constructor constructs a COW by *ADOPTING* a value.
    // The typical usage will be
    //        cow_COW<type>(new type()).
    // This constructor is important because it avoids the copy of the
    // object passed in; however, it should be used with caution
    // because the cow_COW *ADOPTS* (i.e. *DELETES*) the object
    // pointed to by the pointer passed in.
    cow_COW(Type* adopt_me);

    // This assignment operator changes the value of the cow by
    // *ADOPTING*  the pointer passed in.
    // The typical usage will be
    //        cow_COW<type> t = new type();
    //         -or-
    //        cow_COW<type> t = 0;
    // This assignment is important because it avoids the copy of the
    // object on the right hand side. however, it should be used with
    // caution because the cow_COW *ADOPTS* (i.e. *DELETES*) the
    // object on the right hand side.
    inline cow_COW<Type,Dup>& operator=(Type* adopt_me);

    // The ConstGet method returns a const reference to the current value.
    //
    inline const Type& ConstGet() const;

    // The Get method returns a mutable reference to the current value.
    // It ensures that the target object is held only by this reference.
    // If necessary, it creates a new copy of the current value by assigning
    // its value to itself.
    //
    Type& Get();

    // HasValue() is provided to check whether the held pointer is
    // valid, i.e., non-zero.
    //
    inline bool HasValue() const;

    // IsNull() is the complement of HasValue()
    //
    inline bool IsNull() const;

    // Identical() returns true if and only if the held pointer is
    // equivalent to the passed in cow's held pointer
    inline bool Identical(const cow_COW<Type,Dup>& other) const;

protected:
    cow_CountedHld<Type> valuePtr;

};


// This assignment operator changes the value of the cow by
// *ADOPTING*  the pointer passed in.
// The typical usage will be
//        cow_COW<type> t = new type();
//         -or-
//        cow_COW<type> t = 0;
// This assignment is important because it avoids the copy of the
// object on the right hand side. however, it should be used with
// caution because the cow_COW *ADOPTS* (i.e. *DELETES*) the
// object on the right hand side.
//
template <class Type,class Dup>
inline cow_COW<Type,Dup>&  
cow_COW<Type,Dup>::operator=(Type* adopt_me)
{
    valuePtr = adopt_me;

    return *this;
}

// The ConstGet method returns a const reference to the current value.
//
template <class Type,class Dup>
inline const Type& 
cow_COW<Type,Dup>::ConstGet() const
{
    return valuePtr.GetRef();
}


// HasValue() is provided to check whether the held pointer is
// valid, i.e., non-zero.
//
template <class Type, class Dup> 
inline bool 
cow_COW<Type,Dup>::HasValue() const
{ return valuePtr.HasValue(); }

// IsNull() is the complement of HasValue()
//
template <class Type, class Dup> 
inline bool
cow_COW<Type,Dup>::IsNull() const
{ return valuePtr.IsNull(); }

// Identical() returns true if and only if the held pointer is
// equivalent to the passed in cow's held pointer
template <class Type, class Dup> 
inline bool 
cow_COW<Type,Dup>::Identical(const cow_COW<Type,Dup>& other) const
{ return bool(valuePtr.GetPtr() == other.valuePtr.GetPtr()); }


// The COW_COW2_DECL macro will instantiate a cow_CountedHld, cow_Copy, 
// and a cow_COW class, respectively, as <typ>Ptr, <Type>Dup and <Type>COW.
//
#define COW_COW2_FWDL(Type,Copy) \
COW_COUNTEDHLD_FWDL(Type,Type ## Hld) \
typedef Copy<Type> Type ## Dup; \
typedef cow_COW<Type,Type ## Dup> Type ## COW;

#define COW_COW_FWDL(Type) COW_COW2_FWDL(Type,cow_Direct)

#ifdef SMA_NO_TGEN
#define COW_COW2_EXTL(Type,Copy) \
COW_COUNTEDHLD_EXTL(Type,Type ## Hld) \
extern template class Copy<Type>; \
extern template class cow_COW<Type,Type ## Dup>;
#else
#define COW_COW2_EXTL(Type,Copy) \
COW_COUNTEDHLD_EXTL(Type,Type ## Hld)
#endif

#define COW_COW_EXTL(Type) COW_COW2_EXTL(Type,cow_Direct)

#define COW_COW2_BLT_EXTL(Type,Copy) \
COW_COW2_EXTL(Type,Copy) \
static const bool Type ## COW ## _ist = true;

#define COW_COW_BLT_EXTL(Type) COW_COW2_BLT_EXTL(Type,cow_Direct)

#define COW_COW2_DECL(Type,Copy) \
COW_COW2_FWDL(Type,Copy) \
COW_COW2_EXTL(Type,Copy)

// The COW_COW_DECL macro takes only a Type argument and selects cow_Direct 
// as the Copy type.
#define COW_COW_DECL(Type) COW_COW2_DECL(Type,cow_Direct)

#define COW_COW2_BLT_DECL(Type,Copy) \
COW_COW2_FWDL(Type,Copy) \
COW_COW2_BLT_EXTL(Type,Copy)

#define COW_COW_BLT_DECL(Type) COW_COW2_BLT_DECL(Type,cow_Direct)

// The COW_COW2_IMPL and COW_COW_IMPL macros go into the implementation
// file were you want the template to generate object code. Use the
// form which matches the way you defined the generated class.
#define COW_COW2_IMPL(Type,Copy) \
COW_COUNTEDHLD_IMPL(Type,Type ## Hld) \
template class Copy<Type>; \
template class cow_COW<Type,Type ## Dup>;


#define COW_COW_IMPL(Type) COW_COW2_IMPL(Type,cow_Direct)

#define COW_COW2_BLT_IMPL(Type,Copy) \
COW_COW2_IMPL(Type,Copy) \
static_assert(cow_COW_builtin); \
static_assert(cow_CountedHld_builtin); \
static_assert(Type ## COW ## _ist);

#define COW_COW_BLT_IMPL(Type) COW_COW2_BLT_IMPL(Type,cow_Direct)


#endif // cow_COW_h
