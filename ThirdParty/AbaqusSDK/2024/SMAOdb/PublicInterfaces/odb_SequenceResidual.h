//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceResidual_h
#define odb_SequenceResidual_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticIteration.h>
// End local includes

// Forward Declarations
class odb_Residual;


class odb_SequenceResidual : public mem_AllocationOperators
{
public:
   // Returns the number of odb_Residual objects contained in
   // the sequence
   int size() const;

   // Returns an odb_Residual object by zero-based index. If the
   // index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_Residual get(int index) const;
   odb_Residual operator[](int index) const;

private:
   odb_DiagnosticIteration m_iteration;

public: 
   // Unsupported
   odb_SequenceResidual(const odb_DiagnosticIteration& iteration);
   odb_SequenceResidual(const odb_SequenceResidual& sequence);
};


#endif
