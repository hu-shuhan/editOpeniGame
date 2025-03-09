//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_CompatibilityError_h
#define odb_CompatibilityError_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
struct dia_ConstraintVariableOdbEnum;
class dia_CompatibilityError;
class odb_String;
COW_ARRAYCOW_DECL(dia_CompatibilityError)

////////////////////////////////////////////////////////////////////////////////
//
// This class represents diagnostic information that is 
// associated with the constraint equation convergence checks. 
// There is an instance of this class for each constraint
// variable that is active during a given iteration.
//
// Diagnostic information associated with instances of this
// class includes the maximum constraint compatibilty error 
// and the the number of elements where the constraint 
// compatibility error exceeds the tolerance.
//

class odb_CompatibilityError  : public mem_AllocationOperators
{
public:
   dia_ConstraintVariableOdbEnum constraintVariable() const;

   // Returns a string naming the constraint variable
   odb_String constraintVariableName() const;

   // Returns the number of elements where the constraint compatibility
   // error exceeds the tolerance.
   int numberOfElementsExceedingTolerance() const;

   // Returns the maximum constraint compatibility error
   double maxError() const;

   // Returns the part instance name where the maximum constraint 
   // compatibility error occurs
   odb_String instanceName() const;

   // Returns the label of the element where the maximum constraint
   // compatibility error occurs
   int elementLabel() const;

private:
   dia_CompatibilityErrorCOW m_compatibilityError;

public:
   // Unsupported  
   odb_CompatibilityError(const dia_CompatibilityError& compatibilityError);
   odb_CompatibilityError(const odb_CompatibilityError& compatibilityError);
};

#endif
