//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_DiagnosticIncrement
//
// Description :  Class odb_DiagnosticIncrement represents a single increment
//                in the incremental iterative solution process. Every
//                increment is assigned a number that uniquely identifies the
//                increment within the context of a given step. Increment
//                numbers are always greater than or equal to one.
//
//                Increment objects serve as container for attempt objects
//                and provide information as to the status of the automatic
//                incrementation scheme (such as the increment size and
//                whether or not the increment has converged).
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_DiagnosticIncrement_h
#define odb_DiagnosticIncrement_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
class odb_DiagnosticAttempt;
class odb_SequenceDiagnosticAttempt;
class dia_Increment;
class odb_ElementDiagnostic;
class odb_SequenceElementDiagnostic;
COW_ARRAYCOW_DECL(dia_Increment);


////////////////////////////////////////////////////////////////////////////////

class odb_DiagnosticIncrement  : public mem_AllocationOperators
{
public:
   // ------------------------------------------
   // Increment identification

   // Increment number.
   int number() const;

   // ------------------------------------------
   // Increment attributes

   // Initial increment size.
   double initialSize() const;

   // Increment size. The increment size can be different from the
   // initial increment size if an automatic incrementation scheme
   // is used and the increment required multiple attempts. If the
   // increment did not converge then the return value will be zero.
   double size() const;

   // Step time completed as of conclusion of this increment.
   double stepTimeCompleted() const;

   // Returns true if the increment is converged. An increment is
   // converged if the last attempt of the increment is converged.
   // If there are no attempts associated with the increment then
   // the return value will be true (the assumption being that if
   // the increment does not have attempts then the increment was
   // performed during a procedure where increments are converged
   // by definition - i.e. Explicit).
   bool isConverged() const;

   // ------------------------------------------
   // Attempts

   // Number of attempts that were performed for this increment
   int numberOfAttempts() const;

   // Access attempt object by zero-based index. If the index is
   // out-of-bounds then an exception of type odb_Exception will
   // be thrown.
   odb_DiagnosticAttempt getAttempt(int i) const;

   // Sequence of odb_DiagnosticAttempt object
   odb_SequenceDiagnosticAttempt attempts() const;
   // -------------------------------------------
   // Element diagnostics

   // Number of element diagnostics associated with the increment.
   int numberOfElementDiagnostics() const;

   // Access element diagnostic by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_ElementDiagnostic getElementDiagnostic(int i) const;

   // Sequence of odb_ElementDiagnostic objects
   odb_SequenceElementDiagnostic elementDiagnostics() const;

private:
   dia_IncrementCOW m_increment;

public:
   // Unsupported
   odb_DiagnosticIncrement(const dia_Increment& increment);
   odb_DiagnosticIncrement(const odb_DiagnosticIncrement& increment);
   ~odb_DiagnosticIncrement();
};

#endif
