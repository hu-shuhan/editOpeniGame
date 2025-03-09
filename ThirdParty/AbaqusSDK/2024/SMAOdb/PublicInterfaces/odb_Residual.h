//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Residual_h
#define odb_Residual_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
struct dia_FieldVariableOdbEnum;
struct dia_ResidualCheckOdbEnum;
struct dia_CorrectionCheckOdbEnum;
class dia_Residual;
class odb_String;
COW_ARRAYCOW_DECL(dia_Residual)


////////////////////////////////////////////////////////////////////////////////
//
// This class represents diagnostic information that is 
// associated with the field equation convergence checks. 
// (i.e. residuals). There is an instance of this class for
// each field that is active during a given iteration.
//
// Diagnostic information associated with instances of this
// class includes the value of the maximum flux residual, the 
// flux "norm" (i.e. the time average value or user-defined 
// value to which the maximum residual will be compared), the 
// maximum increment in the field, and the maximum correction 
// to the field.
//

    class odb_Residual : public mem_AllocationOperators
{
public:
   dia_FieldVariableOdbEnum fieldVariable() const;

   // Returns a string representation of the field variable
   odb_String fieldVariableName() const;

   // Returns a string naming the field variables conjugate flux
   odb_String conjugateFluxName() const;

   // Returns true if both the residual and correction checks have been
   // accepted.
   bool isAccepted() const;

   dia_ResidualCheckOdbEnum residualCheck() const;

   dia_CorrectionCheckOdbEnum correctionCheck() const;

   // Returns estimated correction (for the next iteration). The value
   // will be non-zero only when the correction check has been accepted 
   // based on the estimated correction.
   double estimatedCorrection() const;

   // Returns true if the response of the field was considered to be linear 
   // based on the criteria for the ratio of the max residual to the 
   // flux norm for convergence to be accepted in one iteration. This has
   // nothing at all to do with linear perturbations! If you don't understand
   // what this means consult the section on convergence criteria in the 
   // ABAQUS/Standard User's Manual.
   bool isResponseLinear() const;

   // Returns the average value of the conjugate flux for this field. This
   // is a spatial average over the entire model for the current iteration.
   double averageFlux() const;

   // Returns the time average value of the conjugate flux for this field.
   // If hasUserDefinedNorm returns true then the return value from this
   // function should be interpreted as a user-defined norm.
   double timeAverageFlux() const;

   // Returns true a user-defined norm has been defined for this field
   // and false otherwise.
   bool hasUserDefinedNorm() const;

   // Access value of the maximum flux residual along with the node label,
   // part instance name, and dof at which it occurs.
   double            maxResidual() const;
   int               maxResidualNodeLabel() const;
   odb_String        maxResidualInstanceName() const;
   int               maxResidualDof() const;

   // Access value of the maximum increment in the field along with
   // the node label, part instance name, and dof at which it occurs.
   double            maxIncrement() const;
   int               maxIncrementNodeLabel() const;
   odb_String        maxIncrementInstanceName() const;
   int               maxIncrementDof() const;

   // Access value of the maximum correction to the field along with
   // the node label, part instance name, and dof at which it occurs.
   double            maxCorrection() const;
   int               maxCorrectionNodeLabel() const;
   odb_String        maxCorrectionInstanceName() const;
   int               maxCorrectionDof() const;

private:
   dia_ResidualCOW m_residual;

public:  
   // Unsupported  
   odb_Residual(const dia_Residual& residual);
   odb_Residual(const odb_Residual& residual);
};

#endif
