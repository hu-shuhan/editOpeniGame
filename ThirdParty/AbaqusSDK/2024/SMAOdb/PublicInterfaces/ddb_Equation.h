//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_Equation_h
#define ddb_Equation_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <ddb_Constraint.h>

struct ddb_EquationTerm  : public mem_AllocationOperators
{
    double coefficient;
    cow_String setName;
    int dof;
    int coordSysId;
};

COW_LIST_DECL(ddb_EquationTerm, ddb_EquationTermList);

class ddb_Equation : public ddb_Constraint
{
public:
    ddb_Equation( const kcoC_ConstraintShortcut& sc );
    ddb_Equation();
    
    ddb_EquationTermList terms() const;
    
    void setTerms( const ddb_EquationTermList& t );
};

#endif

