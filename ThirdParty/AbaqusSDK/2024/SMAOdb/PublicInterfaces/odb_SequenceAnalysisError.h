//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceAnalysisError_h
#define odb_SequenceAnalysisError_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticData.h>
// End local includes

// Forward Declarations
class odb_AnalysisError;


class odb_SequenceAnalysisError : public mem_AllocationOperators
{
public:
   // Returns the number of odb_AnalysisErrors contained in the sequence
   int size() const;

   // Returns an odb_AnalysisError object by zero-based index. If the
   // index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_AnalysisError get(int index) const;
   odb_AnalysisError operator[](int index) const;

private:
   odb_DiagnosticData m_diagnosticData;

public: 
   // Unsupported
   odb_SequenceAnalysisError(const odb_DiagnosticData& diagnosticData);
   odb_SequenceAnalysisError(const odb_SequenceAnalysisError& sequence);
};


#endif
