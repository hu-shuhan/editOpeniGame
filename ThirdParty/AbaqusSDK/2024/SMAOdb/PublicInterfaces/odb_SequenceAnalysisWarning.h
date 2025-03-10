//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceAnalysisWarning_h
#define odb_SequenceAnalysisWarning_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticData.h>
// End local includes

// Forward Declarations
class odb_AnalysisWarning;


class odb_SequenceAnalysisWarning : public mem_AllocationOperators
{
public:
   // Returns the number of odb_AnalysisWarnings contained in the sequence
   int size() const;

   // Returns an odb_AnalysisWarning object by zero-based index. If the
   // index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_AnalysisWarning get(int index) const;
   odb_AnalysisWarning operator[](int index) const;

private:
   odb_DiagnosticData m_diagnosticData;

public: 
   // Unsupported
   odb_SequenceAnalysisWarning(const odb_DiagnosticData& diagnosticData);
   odb_SequenceAnalysisWarning(const odb_SequenceAnalysisWarning& sequence);
};


#endif
