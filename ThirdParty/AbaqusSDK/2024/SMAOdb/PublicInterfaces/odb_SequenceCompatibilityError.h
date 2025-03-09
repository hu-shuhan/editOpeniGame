//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceCompatibilityError_h
#define odb_SequenceCompatibilityError_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_DiagnosticIteration.h>
// End local includes

// Forward Declarations
class odb_CompatibilityError;


class odb_SequenceCompatibilityError : public mem_AllocationOperators
{
public:
   // Returns the number of odb_CompatibilityError objects contained in
   // the sequence
   int size() const;

   // Returns an odb_CompatibilityError object by zero-based index. If 
   // the index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_CompatibilityError get(int index) const;
   odb_CompatibilityError operator[](int index) const;

private:
   odb_DiagnosticIteration m_iteration;

public: 
   // Unsupported
   odb_SequenceCompatibilityError(const odb_DiagnosticIteration& iteration);
   odb_SequenceCompatibilityError(const odb_SequenceCompatibilityError& sequence);
};


#endif
