//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_ContactDiagnostic
//
// Description :  Class odb_ContactDiagnostic represents all of the different 
//                types of contact diagnostic that are issued by ABAQUS. 
//
//                The type of contact diagnostic is indicated by an enum and
//                interpretation of the auxiliary data depends on the type. 
//
//                See source code comments at the top of odb_DiagnosticBase.h
//                for more details.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef odb_ContactDiagnostic_h
#define odb_ContactDiagnostic_h

// Begin local includes
#include <odb_DiagnosticBase.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
struct dia_ContactDiagnosticTypeOdbEnum;
class dia_ContactDiagnostic;
class odb_String;
COW_ARRAYCOW_DECL(dia_ContactDiagnostic)

////////////////////////////////////////////////////////////////////////////////

class odb_ContactDiagnostic : public odb_DiagnosticBase
{
public:
   dia_ContactDiagnosticTypeOdbEnum type() const;

protected:
   // Must implement this...
   virtual dia_MetadataTable& getMetadata() const;

private:
   dia_ContactDiagnosticCOW m_diagnostic;

public:
   // Unsupported
   odb_ContactDiagnostic();
   odb_ContactDiagnostic(const dia_ContactDiagnostic& diagnostic);
   odb_ContactDiagnostic(const odb_ContactDiagnostic& diagnostic);
   ~odb_ContactDiagnostic();
};

#endif
