//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_DiagnosticIteration
//
// Description :  Class odb_DiagnosticIteration represents a single iteration
//                in the incremental-iterative solution process. Every
//                iteration is assigned a number that uniquely identifies the
//                iteration within the context of a given attempt.
//
//                Iterations serve as a container for nearly all of the
//                important diagnostic objects. Among them are residuals,
//                compatibility errors, and contact diagnostics.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_DiagnosticIteration_h
#define odb_DiagnosticIteration_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
class dia_Iteration;
class odb_Residual;
class odb_CompatibilityError;
class odb_ContactDiagnostic;
class odb_ElementDiagnostic;
class odb_ContactIteration;
class odb_SequenceResidual;
class odb_SequenceCompatibilityError;
class odb_SequenceContactDiagnostic;
class odb_SequenceElementDiagnostic;
class odb_SequenceContactIteration;
class odb_NumericalProblemSummary;
COW_ARRAYCOW_DECL(dia_Iteration)


////////////////////////////////////////////////////////////////////////////////

class odb_DiagnosticIteration  : public mem_AllocationOperators
{
public:
   // ------------------------------------------
   // Iteration identification

   // Iteration number. Iterations are numbered consecutively
   // starting from one within the context of a given attempt.
   // Zero in an invalid iteration number.
   int number() const;

   // ------------------------------------------
   // Convergence

   // True if the iteration is converged. An iteration is converged
   // when all the various convergence criteria (such as residuals,
   // compatibilty errors, contact criteria, etc) are satisfied within
   // tolerances. NOTE: Convergence cannot be determined by querying
   // the various criteria from an iteration because there is no
   // guarantee that all convergence criteria will be available from
   // an iteration.
   bool isConverged() const;

   // -------------------------------------------
   // Residuals

   // True if residual criteria is accepted for all active fields.
   // If there are no residual diagnostics associated with the
   // iteration (as will happen during linear perturbations) then
   // the return value will be false. This member only has meaning
   // when there is at least one residual associated with the iteration.
   bool isResidualCriteriaAccepted() const;

   // Number of residuals that are associated with this iteration.
   int numberOfResiduals() const;

   // Sequence of strings with names of the active field variables.
   odb_SequenceString fieldVariableNames() const;
   odb_SequenceInt fieldVariableEnums() const;

   // Access residual diagnostic by zero-based index. There will be one
   // residual diagnostic for each field variable. If the index is out
   // of bounds then an exception of type odb_Exception will be thrown.
   odb_Residual getResidual(int i) const;

   // Sequence of odb_Residual objects
   odb_SequenceResidual residuals() const;

   // -------------------------------------------
   // Compatibility errors

   // True if compatibility errors are within tolerance for all active
   // constraint variables. If there are no compatibility error
   // diagnostics associated with the iteration (as will happen for linear
   // perturbations or problems that do not involve constraint equations)
   // then the return value will be false. This member only has meaning
   // when there is at least one active constraint variable.
   bool isCompatibilityErrorCriteriaAccepted() const;

   // Number of compatibility errors that are associated with the iteration.
   int numberOfCompatibilityErrors() const;

   // Sequence of strings with names of the active constraint variables.
   odb_SequenceString constraintVariableNames() const;
   odb_SequenceInt constraintVariableEnums() const;

   // Access compatibility error diagnostic by zero-based index. There
   // will be one compatibility error diagnostic for each active
   // constraint variable. If the index is out of bounds then an exception
   // of type odb_Exception will be thrown.
   odb_CompatibilityError getCompatibilityError(int i) const;

   // Sequence of odb_CompatibilityError objects
   odb_SequenceCompatibilityError compatibilityErrors() const;

   // -------------------------------------------
   // Contact diagnostics

   // True if the iteration is a severe discontinuity iteration (SDI).
   // If the iteration is an SDI then contact is not converged.
   bool isSevereDiscontinuity() const;

   // True if the tolerance on separation pressure is likely to be
   // too large due to use of the automatic tolerance controls.
   // If the separation tolerance is too large then an additional
   // iteration is performed using the tighter contact tolerances.
   bool isSeparationToleranceTooLarge() const;

   // True if the penalty contact penetration tolerance has been
   // exceeded. When the penetration tolerance has been exceeded
   // additional iterations will be performed with augmentation.
   bool isPenetrationToleranceExceeded() const;

   // Number of contact diagnostics associated with the iteration.
   int numberOfContactDiagnostics() const;

   // Access contact diagnostic by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_ContactDiagnostic getContactDiagnostic(int i) const;

   // Sequence of odb_ContactDiagnostic objects
   odb_SequenceContactDiagnostic contactDiagnostics() const;

   // -------------------------------------------
   // Contact iterations

   // Returns the number of contact iterations that were performed during 
   // this iteration
   int numberOfContactIterations() const;

   // Returns an odb_ContactIteration object by zero-based index. If the index is 
   // out of bounds then an exception of type odb_Exception will be thrown. 
   odb_ContactIteration getContactIteration(int i) const;

   // Returns an odb_SequenceContactIteration object
   odb_SequenceContactIteration contactIterations() const;

   // -------------------------------------------
   // Numerical problems

   // Summary of numerical problems that occured during the iteration
   odb_NumericalProblemSummary numericalProblemSummary() const;

   // -------------------------------------------
   // Element diagnostics

   // Number of contact diagnostics associated with the iteration.
   int numberOfElementDiagnostics() const;

   // Access element diagnostic by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_ElementDiagnostic getElementDiagnostic(int i) const;

   // Sequence of odb_ElementDiagnostic objects
   odb_SequenceElementDiagnostic elementDiagnostics() const;

private:
   dia_IterationCOW m_iteration;

public:
   // Unsupported
   odb_DiagnosticIteration(const dia_Iteration& iteration);
   odb_DiagnosticIteration(const odb_DiagnosticIteration& iteration);
   ~odb_DiagnosticIteration();
};

#endif
