//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Constraint_h
#define odb_Constraint_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <kcoC_ConstraintShortcut.h>

// End local includes

// Forward declarations
class ddb_Constraint;
COW_COW_DECL(ddb_Constraint);


class odb_Constraint  : public mem_AllocationOperators
{
public:
    bool isDisplayBody() const;

protected:
    ddb_ConstraintCOW m_constraint;

public:   // undocumented and unsupported

    // Needed for container
    odb_Constraint();

    odb_Constraint(const ddb_Constraint& sec);

    virtual ~odb_Constraint();
    kcoC_ConstraintShortcut Shortcut() const;

    bool hasValue() const;

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

};

COW_COW_DECL(odb_Constraint);
 
#endif // odb_Constraint_h
