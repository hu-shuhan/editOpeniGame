//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kcoC_Constraint_h
#define kcoC_Constraint_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kcoC_Constraint.h
//
// Description:  Constraint base class definition.
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_ArrayCOW.h>
#include <typ_typeTag.h>
#include <rgnC_RegionList.h>
#include <cls_Uid.h>
#include <cls_IntCOW.h>

// Forward declarations

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

// Class definition

class kcoC_Constraint  : public mem_AllocationOperators
{
  public:

    kcoC_Constraint();
    virtual ~kcoC_Constraint() {};

    virtual kcoC_Constraint* Copy() const = 0;

    // Methods
    void Suppress();
    void Resume();

    bool IsSuppressed() const { return suppress; } 
 
    // Services

    virtual void AssertValid() const = 0;      // Consistency checking
    virtual rgnC_RegionList ConstGetContactNodeSets() const;
    virtual rgnC_RegionList Regions(rgnC_RegionType) const = 0;

    // Static data members

    static omn_FixedString nameCmd();
    static omn_FixedString copyCmd();
    static omn_FixedString constraintCmd();
    static omn_FixedString constraintRepositoryCmd();
    static omn_FixedString setParametersCmd();
    
    static omn_FixedString suppressedCmd();
    static omn_FixedString suppressCmd();
    static omn_FixedString resumeCmd();
    
    static omn_FixedString globalCsysName();

    // Messaging/database interface

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

    bool                    suppress;

    // DDB persistence
    kcoC_Constraint( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    static void CowDtor( cls_IntCOW* cow );
    
private:
    cls_Uid m_ClsUid;
};

// Cow

COW_ARRAYCOW2_DECL(kcoC_Constraint, cow_Virtual);


#endif /* kcoC_Constraint_h */
