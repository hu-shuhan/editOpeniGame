//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_HomogeneousShellSection_h
#define kseO_HomogeneousShellSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_HomogeneousShellSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_GeometryShellSection.h>
#include <odb_String.h>
#include <odb_Union.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseO_RebarLayers;
class kseO_TransverseShearShell;
class kseC_HomogenousShellSection;

class kseO_HomogeneousShellSection: public kseO_GeometryShellSection
{
  public:
    kseO_HomogeneousShellSection( const kseC_SectionShortcut& shortcut );
    kseO_HomogeneousShellSection();
    kseO_HomogeneousShellSection( const kseO_HomogeneousShellSection& copy );
    virtual odb_Section* Copy() const;
    kseO_HomogeneousShellSection& operator=( const kseO_HomogeneousShellSection& rhs );

    virtual ~kseO_HomogeneousShellSection();
    double thickness() const;
    int numIntPts() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_HomogeneousShellSection_h
