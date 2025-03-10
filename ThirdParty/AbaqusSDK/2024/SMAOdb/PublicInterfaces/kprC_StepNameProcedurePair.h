//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kprC_StepNameProcedurePair_h
#define kprC_StepNameProcedurePair_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cls_Uid.h>
#include <cow_String.h>
#include <cls_xo1DArrayList.h>
#include <cls_IntCOW.h>
#include <kprC_Step.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

struct kprC_StepNameProcedurePair : public mem_AllocationOperators
{
    kprC_StepNameProcedurePair(): procedure(0), m_ClsUid(cls_Uid::AllocateUid()) {}

    cow_String stepName;
    kprC_StepCOW procedure;

    cls_Uid m_ClsUid;

    // Database interface
    kprC_StepNameProcedurePair(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;
    static void InitializeMetadata(cls_ClassRegistrar& reg);

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();
};

CLS_xo1DARRAYLIST_DECL(kprC_StepNameProcedurePair, kprC_StepNameProcedurePairList);

COW_COW_DECL(kprC_StepNameProcedurePair);

#endif
