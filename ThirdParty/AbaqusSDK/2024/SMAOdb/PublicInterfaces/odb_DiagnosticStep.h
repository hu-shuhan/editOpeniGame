//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_DiagnosticStep
//
// Description :  Class odb_DiagnosticStep represents a single step in an
//                analysis job's history. Every step is assigned a number,
//                name, and procedure. Both the number and the name must be
//                unique within the context of a given job.
//
//                Step objects serve as a container for increment objects
//                and also provide access to step-level information that
//                is useful for purposes of diagnostics such as active
//                solution controls (stabilization, time incrementation)
//                and problem metrics (chacteristic element length).
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_DiagnosticStep_h
#define odb_DiagnosticStep_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
// End local includes


// Forward declarations
struct dia_IncrementationSchemeOdbEnum;
struct dia_MatrixSolverOdbEnum;
struct dia_StepExtrapolationOdbEnum;
class odb_DiagnosticIncrement;
class odb_SequenceDiagnosticIncrement;
class odb_ContactDiagnostic;
class odb_SequenceContactDiagnostic;
class odb_XplStatus;
class odb_SequenceXplStatus;
class dia_Step;
class odb_String;
COW_ARRAYCOW_DECL(dia_Step)


////////////////////////////////////////////////////////////////////////////////

class odb_DiagnosticStep  : public mem_AllocationOperators
{
public:

   // ------------------------------------------
   // Step identification

   // Step name.
   odb_String name() const;

   // Step number.
   int number() const;

   // ------------------------------------------
   // Procedure

   // Procedure code
   int procedureCode() const;

   // True if procedure is a linear perturbation
   bool isPerturbation() const;

   // True if procedure uses the modified Riks algorithm. Meaningful
   // only for general static procedures.
   bool isRiks() const;

   // ------------------------------------------
   // Increments

   // Number of increments for which diagnostic output is available.
   // In general, data will NOT be available for all increments that
   // were performed during the step.
   int numberOfIncrements() const;

   // Access increment object by zero-based index. Increment objects
   // will be returned in order of increasing increment number. In
   // general, not all increments that were performed during the step
   // will be available. If the index is out-of-bounds then an exception
   // of type odb_Exception will be thrown.
   odb_DiagnosticIncrement getIncrement(int i) const;

   // Sequence of increment objects.
   odb_SequenceDiagnosticIncrement increments() const;

   // Step time that has been completed. If the step is a linear
   // perturbation or if the step is a general nonlinear step but
   // there are no converged/completed increments then the return
   // value will be zero. Note: If the procedure that was used is
   // not yet supported then the return value will also be zero.
   double stepTimeCompleted() const;

   // Number of increments that have been completed. In general, the
   // number of increments completed will be different from the number
   // of increments for which diagnostic data is available.
   int incrementsCompleted() const;

   //--------------------------------------------
   // Incrementation controls

   // Symbolic constant indicating the time incrementation scheme.
   // Possible values are:
   //    AUTO_INCREMENTATION
   //    DIRECT_INCREMENTATION
   //    DIRECT_NO_STOP_INCREMENTATION
   dia_IncrementationSchemeOdbEnum incrementationScheme() const;

   // Time period of the step
   double timePeriod() const;

   // Suggested initial time increment. If the direct incrementation
   // scheme is used then the return value should be interpreted as
   // the constant time increment used during the step.
   double initialTimeIncrement() const;

   // Minimum allowable time increment. Only meaningful when the
   // automatic time incrementation scheme is used.
   double minimumTimeIncrement() const;

   // Maximum allowable time increment. Only meaningful when the
   // automatic time incrementation scheme is used.
   double maximumTimeIncrement() const;

   // Maximum allowable number of increments.
   int maximumNumberOfIncrements() const;

   //--------------------------------------------
   //  Stabilization controls

   // True if automatic stabilization is active during the step.
   bool isStabilized() const;

   // Static stabilization damping factor for the step. The return
   // value will be zero if stabilization is not active.
   double stabilizeFactor() const;

   //--------------------------------------------
   //  Solver controls

   // True if unsymmetric storage and solution is used during the step
   bool isUnsymm() const;

   // True if nonlinear geometry is active during the step.
   bool isNlgeom() const;

   // Symbolic constant indicating which matrix solver was used during the step
   dia_MatrixSolverOdbEnum matrixSolver() const;

   // Symbolic constant indicating which extrapolation scheme was used during the step
   dia_StepExtrapolationOdbEnum extrapolation() const;

   //--------------------------------------------
   // Metrics

   // Characteristic element length for the step.
   double characteristicElementLength() const;

   // -------------------------------------------
   // Contact diagnostics

   // Number of contact diagnostics associated with the iteration.
   int numberOfContactDiagnostics() const;

   // Access contact diagnostic by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_ContactDiagnostic getContactDiagnostic(int i) const;

   // Sequence of odb_ContactDiagnostic objects
   odb_SequenceContactDiagnostic contactDiagnostics() const;

   // -------------------------------------------
   // Xpl status

   // Number of XplStatus associated with the step.
   int numberOfXplStatus() const;

   // Access XplStatus by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_XplStatus getXplStatus(int i) const;

   // Sequence of odb_XplStatus objects
   odb_SequenceXplStatus XplStatus() const;

   odb_SequenceInt getActiveXplStatus() const;

   const dia_Step &constGet() const { return m_step.ConstGet(); }

private:
   dia_StepCOW m_step;

public:
   // Unsupported
   odb_DiagnosticStep(const dia_Step& step);
   odb_DiagnosticStep(const odb_DiagnosticStep& step);
   ~odb_DiagnosticStep();
};

#endif
