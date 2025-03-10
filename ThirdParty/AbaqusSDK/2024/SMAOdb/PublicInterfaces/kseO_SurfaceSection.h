//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_SurfaceSection_h
#define kseO_SurfaceSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_SurfaceSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseO_RebarLayers;
class kseC_SurfaceSection;

class kseO_SurfaceSection: public odb_Section
{
  public:
    kseO_SurfaceSection( const kseC_SectionShortcut& shortcut );
    kseO_SurfaceSection();
    kseO_SurfaceSection( const kseO_SurfaceSection& copy );
    virtual odb_Section* Copy() const;
    kseO_SurfaceSection& operator=( const kseO_SurfaceSection& rhs );

    virtual ~kseO_SurfaceSection();
    bool useDensity() const;
    double density() const;
    kseO_RebarLayers rebarLayers() const;
    kseO_RebarLayers RebarLayers( odb_String rebarSpacing,
                                  const kseO_SequenceLayerProperties& layerTable = kseO_SequenceLayerProperties() );
    static unsigned int typeIdentifier();
};

#endif // kseO_SurfaceSection_h
