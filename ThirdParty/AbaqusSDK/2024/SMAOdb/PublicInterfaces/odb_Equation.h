//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Equation_h
#define odb_Equation_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <BasicConstants.h>
#include <odb_String.h>
#include <odb_Constraint.h>
#include <odb_DatumCsys.h>
#include <odb_Set.h>
#include <ddb_Equation.h>

// Forward declarations
class odb_Set;

class odb_Equation: public odb_Constraint
{
public:


private:
    
public:   //undocumented and unsupported

    //Needed for container
    odb_Equation();

    odb_Equation(const ddb_Equation& constraint);
};

class odb_EquationTerm  : public mem_AllocationOperators
{
public:
    odb_EquationTerm(const odb_Set& set, 
                     int dof, 
		     double coefficient = DBL_MAX,
                     const odb_DatumCsys& datumCsys = odb_DatumCsys());

private:
    ddb_EquationTerm eqTerm;

public:   //undocumented and unsupported
    ddb_EquationTerm equationTerm();

};

class odb_EquationTermList  : public mem_AllocationOperators
{
public:
    odb_EquationTermList();
    
    void append(odb_EquationTerm eqTerm);

private:
    ddb_EquationTermList eqTermList;

public:   //undocumented and unsupported

    ddb_EquationTermList equationTermList();
};

#endif // odb_Equation_h

