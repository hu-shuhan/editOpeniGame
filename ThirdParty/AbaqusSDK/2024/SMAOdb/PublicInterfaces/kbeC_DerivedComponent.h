//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_DerivedComponent_h
#define kbeC_DerivedComponent_h
///////////////////////////////////////////////////////////////////////////////
// kbeC_DerivedComponent
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <kbeC_CDCTerm.h>
#include <kbeC_CDCTermList.h>
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kbeC_DerivedComponent : public mem_AllocationOperators
{
public:
    kbeC_DerivedComponent();
    // Methods
    virtual void AssertValid() const;
    bool operator==(const kbeC_DerivedComponent& ) const;
    bool operator!=(const kbeC_DerivedComponent& ) const;
    bool ValidComponents(const cow_ListInt&, bool testIntrinsic=true) const;

    // Default methods
    // command string methods
    static omn_FixedString cdcTermsCmd();
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();

    virtual kbeC_DerivedComponent* Copy() const;


    // ODB database interface
    kbeC_DerivedComponent(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members
    kbeC_CDCTermList cdcTerms;

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kbeC_DerivedComponent, cow_Virtual);

// Default methods implementation
#endif // kbeC_DerivedComponent_h
