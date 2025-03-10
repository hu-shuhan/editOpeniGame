//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceDiagnosticIteration_h
#define odb_SequenceDiagnosticIteration_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticAttempt.h>
// End local includes

// Forward Declarations
class odb_DiagnosticIteration;


class odb_SequenceDiagnosticIteration : public mem_AllocationOperators
{
public:
   // Returns the number of odb_DiagnosticIteration objects contained in
   // the sequence
   int size() const;

   // Returns an odb_DiagnosticIteration object by zero-based index. If
   // the index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_DiagnosticIteration get(int index) const;
   odb_DiagnosticIteration operator[](int index) const;

private:
   odb_DiagnosticAttempt m_attempt;

public: 
   // Unsupported
   odb_SequenceDiagnosticIteration(const odb_DiagnosticAttempt& attempt);
   odb_SequenceDiagnosticIteration(const odb_SequenceDiagnosticIteration& iterationSequence);
};


#endif
