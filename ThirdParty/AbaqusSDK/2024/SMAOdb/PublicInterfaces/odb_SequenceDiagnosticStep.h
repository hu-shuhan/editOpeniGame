//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceDiagnosticStep_h
#define odb_SequenceDiagnosticStep_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticData.h>
// End local includes

// Forward Declarations
class odb_DiagnosticStep;


class odb_SequenceDiagnosticStep : public mem_AllocationOperators
{
public:
   // Returns the number of odb_DiagnosticStep objects contained in
   // the sequence
   int size() const;

   // Returns an odb_DiagnosticStep object by zero-based index. If the
   // index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_DiagnosticStep get(int index) const;
   odb_DiagnosticStep operator[](int index) const;

private:
   odb_DiagnosticData m_diagnosticData;

public: 
   // Unsupported
   odb_SequenceDiagnosticStep(const odb_DiagnosticData& diagnosticData);
   odb_SequenceDiagnosticStep(const odb_SequenceDiagnosticStep& stepSequence);
};


#endif
