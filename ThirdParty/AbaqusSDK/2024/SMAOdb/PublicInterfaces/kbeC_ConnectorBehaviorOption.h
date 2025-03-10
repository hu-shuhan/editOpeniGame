//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeC_ConnectorBehaviorOption_h
#define kbeC_ConnectorBehaviorOption_h
///////////////////////////////////////////////////////////////////////////////
// kbeC_ConnectorBehaviorOption
//

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// System include
#include <float.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <omn_FixedString.h>


class kbeC_ConnectorBehaviorOption : public mem_AllocationOperators
{
public:
    kbeC_ConnectorBehaviorOption();
    // Methods
    virtual void AssertValid() const = 0;
    virtual bool operator==(const kbeC_ConnectorBehaviorOption& ) const = 0;
    virtual bool operator!=(const kbeC_ConnectorBehaviorOption& ) const = 0;

    // Default methods
    // command string methods
    static omn_FixedString setValuesCmd();
    static omn_FixedString constructorCmd();

    virtual kbeC_ConnectorBehaviorOption* Copy() const = 0;


    // ODB database interface
    kbeC_ConnectorBehaviorOption(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    // Data members

private:
    cls_Uid m_ClsUid; // ODB database id

};

COW_ARRAYCOW2_DECL(kbeC_ConnectorBehaviorOption, cow_Virtual);

// Default methods implementation
#endif // kbeC_ConnectorBehaviorOption_h
