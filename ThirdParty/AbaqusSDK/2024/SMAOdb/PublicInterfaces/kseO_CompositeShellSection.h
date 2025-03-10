//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_CompositeShellSection_h
#define kseO_CompositeShellSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_CompositeShellSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_GeometryShellSection.h>
#include <kseO_SequenceSectionLayer.h>
#include <odb_String.h>
#include <odb_Union.h>
#include <kseO_SequenceLayerProperties.h>
// Forward declarations
class kseO_RebarLayers;
class kseO_TransverseShearShell;
class kseC_CompositeShellSection;

class kseO_CompositeShellSection: public kseO_GeometryShellSection
{
  public:
    kseO_CompositeShellSection( const kseC_SectionShortcut& shortcut );
    kseO_CompositeShellSection();
    kseO_CompositeShellSection( const kseO_CompositeShellSection& copy );
    virtual odb_Section* Copy() const;
    kseO_CompositeShellSection& operator=( const kseO_CompositeShellSection& rhs );

    virtual ~kseO_CompositeShellSection();
    bool symmetric() const;
    kseO_SequenceSectionLayer layup() const;
    odb_String layupName() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_CompositeShellSection_h
