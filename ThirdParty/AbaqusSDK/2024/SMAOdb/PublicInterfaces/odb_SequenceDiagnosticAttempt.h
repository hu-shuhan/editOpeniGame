//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceDiagnosticAttempt_h
#define odb_SequenceDiagnosticAttempt_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticIncrement.h>
// End local includes

// Forward Declarations
class odb_DiagnosticAttempt;


class odb_SequenceDiagnosticAttempt : public mem_AllocationOperators
{
public:
   // Returns the number of odb_DiagnosticAttempt objects contained in
   // the sequence
   int size() const;

   // Returns an odb_DiagnosticAttempt object by zero-based index. If 
   // the index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_DiagnosticAttempt get(int index) const;
   odb_DiagnosticAttempt operator[](int index) const;

private:
   odb_DiagnosticIncrement m_increment;

public: 
   // Unsupported
   odb_SequenceDiagnosticAttempt(const odb_DiagnosticIncrement& increment);
   odb_SequenceDiagnosticAttempt(const odb_SequenceDiagnosticAttempt& attemptSequence);
};


#endif
