//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_CompositeSolidSection_h
#define kseO_CompositeSolidSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_CompositeSolidSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <kseO_SequenceSectionLayer.h>
// Forward declarations
class kseC_CompositeSolidSection;

class kseO_CompositeSolidSection: public odb_Section
{
  public:
    kseO_CompositeSolidSection( const kseC_SectionShortcut& shortcut );
    kseO_CompositeSolidSection();
    kseO_CompositeSolidSection( const kseO_CompositeSolidSection& copy );
    virtual odb_Section* Copy() const;
    kseO_CompositeSolidSection& operator=( const kseO_CompositeSolidSection& rhs );

    virtual ~kseO_CompositeSolidSection();
    bool symmetric() const;
    kseO_SequenceSectionLayer layup() const;
    odb_String layupName() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_CompositeSolidSection_h
