//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kinC_IPOption_h
#define kinC_IPOption_h
///////////////////////////////////////////////////////////////////////////////
//
// File Name: kinC_IPOption
//
// Description: Engineering object class for kinC_IPOption
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_COW.h>
#include <typ_typeTag.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>

#include <cls_xp2DArrayDouble.h>
#include <cls_xpListDouble.h>
#include <idb_Utils.h>

// Forward declarations

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

//

class kinC_IPOption  : public mem_AllocationOperators
{
  public:

    kinC_IPOption();
    virtual ~kinC_IPOption();

    virtual kinC_IPOption* Copy() const = 0;

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();
    kinC_IPOption *             DeepCopy() const {
        return (kinC_IPOption*)DDB_DeepCopyPtr(this, DynTypeId());
    }


    // DDB persistence
    kinC_IPOption( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    
private:
    cls_Uid m_ClsUid;

protected:
    bool kinC_IsEqual(const cls_xp2DArrayDouble&, const cls_xp2DArrayDouble&) const;
    bool kinC_IsEqual(const cls_xpListDouble&, const cls_xpListDouble&) const;

};

COW_COW2_DECL(kinC_IPOption, cow_Virtual);

#endif
