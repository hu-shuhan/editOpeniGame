//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef mdl_PropagatingAttribute_h
#define mdl_PropagatingAttribute_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: mdl_PropagatingAttribute.h
//
// Description:  Propagating attributes consist of a value and a state.
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
#include <cow_String.h>

#include <cls_xpListInt.h>
#include <cls_xpListString.h>
#include <cls_xp1DArrayInt.h>
#include <cls_xp1DArrayListFloat.h>
#include <cls_Uid.h>

#include <typ_typeTag.h>
#include <omi_complex.h>
#include <mdl_propAttributeState.h>


// Forward declarations

class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;


// Class definition

template <class Value>
class mdl_PropAttr : public mem_AllocationOperators
{
  public:

    mdl_PropAttr();
    mdl_PropAttr(const Value&);
    ~mdl_PropAttr();

    mdl_PropAttr<Value>& operator=(const Value&);
    mdl_PropAttr<Value>& operator=(
	const mdl_PropAttributeStateEnm&);

    bool operator==(const mdl_PropAttr<Value>&) const;
    bool operator!=(const mdl_PropAttr<Value>&) const;
    bool operator==(const Value&) const;
    bool operator!=(const Value&) const;
    bool operator==(mdl_PropAttributeStateEnm) const;
    bool operator!=(mdl_PropAttributeStateEnm) const;

    // Interface

    void Zero();                // Zero current value
    void Propagate();           // Propagate current state
    void Modify(const mdl_PropAttr<Value>&);
    void SetStatus(const char* attName,
		   const mdl_PropAttributeStateConst &setting,
		   bool beingCreated);

    // WARNING:  ForceState SHOULD ALMOST NEVER BE USED.  The client
    // should rely on the Propagate(), Modify() and other methods 
    // for proper behavior.  The ForceState() interface is only 
    // provided as a means to build a propagating attribute 
    // from scratch in special situations
    void ForceState(mdl_PropAttributeStateEnm);

    // Query
    
    Value& Get();                 // Get non-const ref to current value
    const Value& ConstGet() const;// Get const ref to current value
    bool IsModified() const;
    bool IsConditionModified() const;
    bool IsActive() const;
    bool IsFree() const;
    mdl_PropAttributeStateEnm State() const { return state; }

    // Messaging/database interface
    typ_typeTag DynTypeId() const;

    static typ_typeTag TypeId();

    mdl_PropAttr( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void Dtor(cls_IntCOW* cow);
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    virtual void NewUid();

  protected:

    // Data members

    Value val;
    mdl_PropAttributeStateEnm state;

private:
    cls_Uid m_ClsUid;
};


// Help with typetable entries
// stream operators for class mdl_PropAttr
//
// We need to explicitely instantiate the TypeId method before it is used
//
#define MDL_PROP_ATTR_DECL(TYPE,NAME) \
typedef mdl_PropAttr<TYPE> NAME;\
COW_COW2_DECL(NAME,cow_Direct);

//#if defined(HKS_NT)
//#define MDL_PROP_ATTR_PRE_DECL(TYPE)
//#else
//#define MDL_PROP_ATTR_PRE_DECL(TYPE)			\
//template <> typ_typeTag mdl_PropAttr<TYPE>::TypeId();
//#endif

#define MDL_PROP_ATTR_IMPL(TYPE,NAME) \
template <> typ_typeTag \
mdl_PropAttr<TYPE>::TypeId()\
{\
    return TYP_TIX_ ## NAME;\
}\
template class mdl_PropAttr<TYPE>;\
COW_COW2_IMPL(NAME,cow_Direct);


#define MDL_PROP_ATTR_TT(NAME) \
    {\
        typ_Phase0TT,\
        typ_NONE,\
        #NAME\
    },

// Some useful pre-instantiations

MDL_PROP_ATTR_DECL(cow_String, mdl_PropString);
MDL_PROP_ATTR_DECL(double, mdl_PropDouble);
MDL_PROP_ATTR_DECL(bool, mdl_PropBool);
MDL_PROP_ATTR_DECL(cls_xpListInt, mdl_PropListInt);
MDL_PROP_ATTR_DECL(int, mdl_PropInt);
MDL_PROP_ATTR_DECL(cls_xp1DArrayInt, mdl_Prop1DArrayInt);
MDL_PROP_ATTR_DECL(cls_xpListString, mdl_PropListString);
MDL_PROP_ATTR_DECL(cls_xp1DArrayListFloat, mdl_Prop1DArrayListFloat);


#define MDL_PROP_ATTR_IMPL(TYPE,NAME) \
template <> typ_typeTag \
mdl_PropAttr<TYPE>::TypeId()\
{\
    return TYP_TIX_ ## NAME;\
}\
template class mdl_PropAttr<TYPE>;\
COW_COW2_IMPL(NAME,cow_Direct);




#endif /* mdl_PropagatingAttribute_h */
