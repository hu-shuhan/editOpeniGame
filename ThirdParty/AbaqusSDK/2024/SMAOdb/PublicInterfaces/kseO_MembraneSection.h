//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_MembraneSection_h
#define kseO_MembraneSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_MembraneSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseO_RebarLayers;
class kseC_MembraneSection;

class kseO_MembraneSection: public odb_Section
{
  public:
    kseO_MembraneSection( const kseC_SectionShortcut& shortcut );
    kseO_MembraneSection();
    kseO_MembraneSection( const kseO_MembraneSection& copy );
    virtual odb_Section* Copy() const;
    kseO_MembraneSection& operator=( const kseO_MembraneSection& rhs );

    virtual ~kseO_MembraneSection();
    double thickness() const;
    odb_String poissonDefinition() const;
    double poisson() const;
    odb_String material() const;
    kseO_RebarLayers rebarLayers() const;
    kseO_RebarLayers RebarLayers( odb_String rebarSpacing,
                                  const kseO_SequenceLayerProperties& layerTable = kseO_SequenceLayerProperties() );
    static unsigned int typeIdentifier();
};

#endif // kseO_MembraneSection_h
