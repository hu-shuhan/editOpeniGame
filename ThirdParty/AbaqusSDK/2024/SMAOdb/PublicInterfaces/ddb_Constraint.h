//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_Constraint_h
#define ddb_Constraint_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kcoC_ConstraintShortcut.h>
#include <odiK_ModelShortcut.h>
#include <cow_COW.h>

// Forward declarations
class ddb_Tie;
class ddb_Equation;
class ddb_RigidBody;
class ddb_DisplayBody;
class ddb_EmbeddedRegion;
class ddb_Coupling;
class ddb_ShellSolidCoupling;
class ddb_Mpc;

class ddb_Constraint  : public mem_AllocationOperators
{
public:
    ddb_Constraint( const kcoC_ConstraintShortcut& sc );
    ddb_Constraint( const kcoC_ConstraintShortcut& sc,
		    const odiK_ModelShortcut& model );
    ddb_Constraint();
    
    typ_typeTag type() const;
    
    operator ddb_Tie() const;
    operator ddb_RigidBody() const;
    operator ddb_DisplayBody() const;
    operator ddb_Equation() const;
    operator ddb_EmbeddedRegion() const;
    operator ddb_Coupling() const;
    operator ddb_ShellSolidCoupling() const;
    operator ddb_Mpc() const;

    kcoC_ConstraintShortcut Shortcut() const { return shortcut; }

protected:
    kcoC_ConstraintShortcut shortcut;
    odiK_ModelShortcut model;
};

COW_COW_DECL(ddb_Constraint);

#endif
    
    
