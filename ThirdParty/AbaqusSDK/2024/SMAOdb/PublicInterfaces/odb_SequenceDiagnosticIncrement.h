//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceDiagnosticIncrement_h
#define odb_SequenceDiagnosticIncrement_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticStep.h>
// End local includes

// Forward Declarations
class odb_DiagnosticIncrement;


class odb_SequenceDiagnosticIncrement : public mem_AllocationOperators
{
public:
   // Returns the number of odb_DiagnosticIncrement objects contained in
   // the sequence
   int size() const;

   // Returns an odb_DiagnosticIncrement object by zero-based index. If 
   // the index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_DiagnosticIncrement get(int index) const;
   odb_DiagnosticIncrement operator[](int index) const;

private:
   odb_DiagnosticStep m_step;

public: 
   // Unsupported
   odb_SequenceDiagnosticIncrement(const odb_DiagnosticStep& step);
   odb_SequenceDiagnosticIncrement(const odb_SequenceDiagnosticIncrement& incrementSequence);
};


#endif
