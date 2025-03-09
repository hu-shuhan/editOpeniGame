//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_AnalysisWarning
//
// Description :  Class odb_AnalysisWarning represents all the different types 
//                of warning that can be issued by an ABAQUS analysis. 
//                Currently, this class inherits all of its functionality from 
//                the base class odb_AnalysisWarningError. 
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_AnalysisWarning_h
#define odb_AnalysisWarning_h

// Begin local includes
#include <odb_AnalysisWarningError.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
class dia_Warning;
COW_ARRAYCOW_DECL(dia_Warning)


////////////////////////////////////////////////////////////////////////////////

class odb_AnalysisWarning : public odb_AnalysisWarningError
{
public:
   // TODO-> Add warning-specific functionality.

private:
   dia_WarningCOW m_warning;

public:
   // Unsupported  
   odb_AnalysisWarning(const dia_Warning& warning);
   odb_AnalysisWarning(const odb_AnalysisWarning& warning);
   ~odb_AnalysisWarning();
};

#endif
