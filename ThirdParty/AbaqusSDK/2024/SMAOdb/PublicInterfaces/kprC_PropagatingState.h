//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kprC_PropagatingState_h
#define kprC_PropagatingState_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kprC_PropagatingState.h
//
// Description:
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_COW.h>
#include <mdl_propagationStatus.h>
#include <cls_xoList.h>
#include <kprC_Step.h>
#include <typ_typeTag.h>

// Forward declarations

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

#include <cls_Uid.h>

// Class to hold step-name, status, state and procedure-cow

template <class State> struct kprC_PropagatingState  : public mem_AllocationOperators
{
    typedef cow_ArrayCOW<State, cow_Virtual<State> > StateCOW;
    kprC_PropagatingState();

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;

    static typ_typeTag      TypeId();

    static void* Ctor( cls_ReadVisitor& rv );
    static void Dtor(cls_IntCOW* cow);
    kprC_PropagatingState( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

    // Data members

    cow_String stepName;
    mdl_PropagationStatusEnm status;
    cow_ArrayCOW<State, cow_Virtual<State> > state;
    kprC_StepCOW procedure;

private:
    cls_Uid m_ClsUid;
};

// Macros
#define KPRC_PROPAGATING_STATE_DECL(TYPE,NAME) \
typedef kprC_PropagatingState<TYPE> NAME;\
CLS_xoLIST_ITER_DECL(NAME, NAME ## List);\
COW_COW2_DECL(NAME,cow_Direct);



#define KPRC_PROPAGATING_STATE_TT(NAME) \
    {\
        typ_Phase0TT,\
        typ_NONE,\
        #NAME\
    },

#endif /* kprC_PropagatingState_h */
