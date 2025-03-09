//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_AnalysisWarningError
//
// Description :  Class odb_AnalysisWarningError represents all of the different
//                types of warnings and errors that can be issued by an ABAQUS
//                analysis
//
//                Support for type ids and auxiliary data is inherited from
//                the base class (see odb_DiagnosticBase.h for details). 
//                This class introduces the concept of a "category" and also 
//                adds a mechanism to identify a warning or error with a
//                a step/increment/attempt/iteration. 
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_AnalysisWarningError_h
#define odb_AnalysisWarningError_h

// Begin local includes
#include <odb_DiagnosticBase.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
// End local includes


// Forward declarations
struct dia_WarningErrorCategoryOdbEnum;
class dia_WarningErrorBase;
COW_ARRAYCOW2_DECL(dia_WarningErrorBase, cow_Virtual)
class odb_String;


////////////////////////////////////////////////////////////////////////////////

class odb_AnalysisWarningError : public odb_DiagnosticBase
{
public:

   // Symbolic constant indicating category of the warning/error
   dia_WarningErrorCategoryOdbEnum category() const;

   // Step, increment, attempt, and iteration numbers indicating where
   // the warning/error was issued. If the warning/error was issued
   // from outside an analysis step (i.e. during initialization) then
   // the numbers will all be zero.
   int stepNumber() const;
   int incrementNumber() const;
   int attemptNumber() const;
   int iterationNumber() const;

protected:
   // Must implement this...
   virtual dia_MetadataTable& getMetadata() const;

private:
   dia_WarningErrorBaseCOW m_base;

public:
   // Unsupported
   odb_AnalysisWarningError(const dia_WarningErrorBase& base);
   odb_AnalysisWarningError(const odb_AnalysisWarningError& base);
};


#endif
