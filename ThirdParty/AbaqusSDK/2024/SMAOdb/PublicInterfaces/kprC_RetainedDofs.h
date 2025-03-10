//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kprC_RetainedDofs_h
#define kprC_RetainedDofs_h

///////////////////////////////////////////////////////////////////////////////
//
// Description:  Retained Nodal DOFs pairs and list definition
//

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cls_xo1DArrayList.h>
#include <cls_xp1DArrayListInt.h>
#include <cow_ListInt.h>
#include <rgnC_Region.h>

// Forward declarations

class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Class definition

class kprC_RetainedDofs  : public mem_AllocationOperators
{
public:
    kprC_RetainedDofs();
    kprC_RetainedDofs(const rgnC_Region& region, 
                      const cow_ListInt& dofs);
    
    // Data members
    rgnC_Region region;
    cls_xp1DArrayListInt dofs;
    
    // DDB pesistency
    kprC_RetainedDofs(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;

private:
    cls_Uid m_ClsUid; // object identifier for odb
};

CLS_xo1DARRAYLIST_DECL(kprC_RetainedDofs, kprC_RetainedDofsList);

#endif /* kprC_RetainedDofs_h */
