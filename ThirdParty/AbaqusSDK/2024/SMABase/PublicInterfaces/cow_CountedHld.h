//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
//
// A reference counted pointer template.
//
#ifndef cow_CountedHld_h
#define cow_CountedHld_h

// Begin Local Includes
#include <mem_AllocationOperatorsTiny.h>
#include <omi_types.h>
#include <cow_Counter.h>
#include <omn_pragma.h>

template <class Type> class cow_CountedHld  : public mem_AllocationOperatorsTiny
{

 public:
    inline cow_CountedHld();
    inline cow_CountedHld(Type *);
    inline cow_CountedHld(const cow_CountedHld<Type> &);
    inline ~cow_CountedHld();

    Type* GetPtr() const {return ref;}
    const Type& GetRef() const {return *ref;}
    const Type& ConstGet() const {return *ref;}

    inline cow_CountedHld<Type>& operator= (Type* optr); 
    inline cow_CountedHld<Type>& operator= (const cow_CountedHld<Type> &c);

    bool Exclusive() const { return bool(count->References() == 1); }
    bool IsNull()  const  { return bool(ref == 0); }
    bool HasValue() const { return bool(ref != 0); }

 protected:
    void Delete(); // delete referent and counter
    void DeleteReferent(); // delete referent only
    inline void DropReference(); // drop the reference

    cow_Counter *count;   // pointer to avoid two indirections in dereference
    Type *ref;            // of ref which would occur if used a "holder"

};

// 
// The cow_CountedHld class adopts a pointer that is passed in and
// will manages this pointer with a reference count.
//
template <class Type>
inline cow_CountedHld<Type>::cow_CountedHld()
:
    count(new cow_Counter),  // initialize the counter
    ref(NULL)
{}


// 
// The cow_CountedHld class adopts a pointer that is passed in and
// will manages this pointer with a reference count.
//
template <class Type>
inline cow_CountedHld<Type>::cow_CountedHld(Type *obj)    // adopt pointer
:
    count(new cow_Counter),  // initialize the counter
    ref(obj)
{}

//
// The copy constructor makes an additional reference to the referent
// held by the cow_CountedHld being copied.
//
template <class Type>
inline cow_CountedHld<Type>::cow_CountedHld(const cow_CountedHld<Type>& c)
:
    count(c.count),
    ref(c.ref)
{
    count->Increment();
}

//
// The destructor drops its reference to the referent.
// 
template <class Type>
inline cow_CountedHld<Type>::~cow_CountedHld()
{
    DropReference();
}

//
// Assignment of a Type* to a cow_CountedHld will drop the reference the
// referent which it is currently holding, and create a reference to the
// new target.
//
template <class Type>
inline
cow_CountedHld<Type>& cow_CountedHld<Type>::operator= (Type* rsh)
{
        // if we are the only ones using the counter
        if( count->References() == 1 )
                {
                // delete the referent
                DeleteReferent();
                }

        // else, (the counter is also in use by someone else)
        else
                {
                // Detach us from the current counter
                count->Decrement();
                
                // create a new counter
                count = new cow_Counter;
                }

    ref = rsh;

    return *this;
}

//
// Assignment operator between cow_CountedHlds drops the current referent and
// creates a additional reference to the referent held by the rhs
//
template <class Type>
inline cow_CountedHld<Type>& 
cow_CountedHld<Type>::operator= (const cow_CountedHld<Type> &rhs)
{
    // IMPORTANT:
    // The rhs is copied to temp variables in order to avoid problems with
    // self assignment, or any assignment which could cause the rhs to be
    // deleted.  We need a copy to: 1) prevent DropReference from deleting
    // the referent (i.e., increment the ref count) and 2) since the rsh is
    // passed by reference, DropReference would cause the rsh holder to be
    // deleted causing free memory reads on the count = rsh.count operations.

    // We used to only increment the ref count on rsh to prevent case 1)
    // above, but with that technique we ran into a Free Memory Read when
    // doing operations like this node = node.next (e.g., list.Remove()).

    rhs.count->Increment();                // hold a reference.
    cow_Counter *tempCount = rhs.count;    // temp copies in case
    Type *tempRef = rhs.ref;               // of self-reference.

    DropReference();           // drop the reference we are holding
    count = tempCount;
    ref = tempRef;

    return *this;
}

// The DropReference decrements the reference counter and calls the
// out-of-line Delete method if and only if the the referent and counter
// should be deleted. This technique maximizes performance when no
// deletion is performed with minimal code bloat.
//
template <class Type>
inline void cow_CountedHld<Type>::DropReference()
{
    if (count->Decrement())
        Delete();
}

//
// The COW_COUNTEDHLD_DECL declares a cow_CountedHld. You choose the NAME.
//
#define COW_COUNTEDHLD_FWDL(TYPE, NAME) \
typedef cow_CountedHld<TYPE> NAME;

#ifdef SMA_NO_TGEN
#define COW_COUNTEDHLD_EXTL(TYPE, NAME) \
extern template class cow_CountedHld<TYPE>;
#else
#define COW_COUNTEDHLD_EXTL(TYPE, NAME)
#endif

#define COW_COUNTEDHLD_BLT_EXTL(TYPE, NAME) \
COW_COUNTEDHLD_EXTL(TYPE, NAME) \
static const bool NAME ## _ist = true;

#define COW_COUNTEDHLD_DECL(TYPE, NAME) \
COW_COUNTEDHLD_FWDL(TYPE, NAME) \
COW_COUNTEDHLD_EXTL(TYPE, NAME)

#define COW_COUNTEDHLD_BLT_DECL(TYPE, NAME) \
COW_COUNTEDHLD_FWDL(TYPE, NAME) \
COW_COUNTEDHLD_BLT_EXTL(TYPE, NAME)

#define COW_COUNTEDHLD_IMPL(TYPE, NAME) \
template class cow_CountedHld<TYPE>;

#define COW_COUNTEDHLD_BLT_IMPL(TYPE, NAME) \
COW_COUNTEDHLD_IMPL(TYPE, NAME) \
static_assert(cow_CountedHld_builtin); \
static_assert(NAME ## _ist);

#endif
