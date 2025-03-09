//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kinC_InteractionProperty_h
#define kinC_InteractionProperty_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kinC_InteractionProperty.h
//
// Description:  Interaction property base class definition.
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_ArrayCOW.h>
#include <omu_PrimEnum.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <idb_Utils.h>

#include <typ_typeTag.h>

// Forward declarations

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

// Class definition

class kinC_InteractionProperty  : public mem_AllocationOperators
{
  public:

    kinC_InteractionProperty();
    virtual ~kinC_InteractionProperty();

    virtual kinC_InteractionProperty* Copy() const = 0;

    // Enums

    enum CategoryEnm { MECHANICAL, THERMAL, OTHER };

    // Services

    virtual CategoryEnm Category() const = 0;
    virtual void AssertValid() const = 0;     // Consistency checking

    // Static data members

    static omn_FixedString interactionPropertiesCmd();
    static omn_FixedString interactionPropertyCmd();
    static omn_FixedString copyConstructorCmd();
    static omn_FixedString nameCmd();
    static omn_FixedString categoryCmd();

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();
    kinC_InteractionProperty *             DeepCopy() const {
        return (kinC_InteractionProperty*)DDB_DeepCopyPtr(this, DynTypeId());
    }

    // DDB persistence
    kinC_InteractionProperty( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor( cls_IntCOW* cow );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    
private:
    cls_Uid m_ClsUid;
};

// Map to symbolic constant

typedef omu_PrimEnum<kinC_InteractionProperty::CategoryEnm,1> kinC_InteractionPropertyCategoryConst;

// COW

COW_ARRAYCOW2_DECL(kinC_InteractionProperty, cow_Virtual);


#endif /* kinC_InteractionProperty_h */
