//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_GeometryShellSection_h
#define kseO_GeometryShellSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_GeometryShellSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_ShellSection.h>
#include <odb_String.h>
#include <odb_Union.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseO_RebarLayers;
class kseO_TransverseShearShell;
class kseC_GeometryShellSection;

class kseO_GeometryShellSection: public kseO_ShellSection
{
  public:
    kseO_GeometryShellSection( const kseC_SectionShortcut& shortcut );
    kseO_GeometryShellSection();
    kseO_GeometryShellSection( const kseO_GeometryShellSection& copy );
    virtual odb_Section* Copy() const;
    kseO_GeometryShellSection& operator=( const kseO_GeometryShellSection& rhs );

    virtual ~kseO_GeometryShellSection();
    odb_String thicknessType() const;
    bool preIntegrate() const;
    odb_String poissonDefinition() const;
    double poisson() const;
    odb_String integrationRule() const;
    odb_String temperature() const;
    odb_String idealization() const;
    odb_Union nTemp() const;
    odb_Union thicknessModulus() const;
    bool useDensity() const;
    double density() const;
    odb_String thicknessField() const;
    kseO_RebarLayers rebarLayers() const;
    odb_String nodalThicknessField() const;
    kseO_RebarLayers RebarLayers( odb_String rebarSpacing,
                                  const kseO_SequenceLayerProperties& layerTable = kseO_SequenceLayerProperties() );
    static unsigned int typeIdentifier();
};

#endif // kseO_GeometryShellSection_h
