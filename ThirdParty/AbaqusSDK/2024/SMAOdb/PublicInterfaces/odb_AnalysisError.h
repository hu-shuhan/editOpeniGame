//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_AnalysisError
//
// Description :  Class odb_AnalysisError represents all the different types of
//                error that can be issued by an ABAQUS analysis. Most of the 
//                functionality is inherited from the base class 
//                odb_AnalysisWarningError.
// 
//                This class extends the base class by adding a string
//                that describes an "internal error". Internal error
//                strings typically do not have any meaning to users but 
//                may have meaning to the ABAQUS development organization. 
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_AnalysisError_h
#define odb_AnalysisError_h

// Begin local includes
#include <odb_AnalysisWarningError.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
class odb_String;
class dia_Error;
COW_ARRAYCOW_DECL(dia_Error)


////////////////////////////////////////////////////////////////////////////////

class odb_AnalysisError : public odb_AnalysisWarningError
{
public:
   // String describing the internal error.
   odb_String internalString() const;

private:
   dia_ErrorCOW m_error;

public:
   // Unsupported  
   odb_AnalysisError(const dia_Error& error);
   odb_AnalysisError(const odb_AnalysisError& error);
   ~odb_AnalysisError();
};

#endif
