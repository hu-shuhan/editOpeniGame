//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_RebarLayers_h
#define kseO_RebarLayers_h
///////////////////////////////////////////////////////////////////////////////
// kseO_RebarLayers
//

// Begin local includes
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kseC_RebarLayersShortcut.h>
#include <odb_String.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseC_RebarLayers;

class kseO_RebarLayers: public mem_AllocationOperators
{
  public:
    kseO_RebarLayers( const kseC_RebarLayersShortcut& shortcut );
    kseO_RebarLayers();
    kseO_RebarLayers( const kseO_RebarLayers& copy );
    virtual kseO_RebarLayers* Copy() const;
    kseO_RebarLayers& operator=( const kseO_RebarLayers& rhs );

    virtual ~kseO_RebarLayers();
    odb_String rebarSpacing() const;
    kseO_SequenceLayerProperties layerTable() const;
    kseC_RebarLayersShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kseC_RebarLayersShortcut shortcut;
};

#endif // kseO_RebarLayers_h
