//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_AcousticInterfaceSection_h
#define kseO_AcousticInterfaceSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_AcousticInterfaceSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_AcousticInterfaceSection;

class kseO_AcousticInterfaceSection: public odb_Section
{
  public:
    kseO_AcousticInterfaceSection( const kseC_SectionShortcut& shortcut );
    kseO_AcousticInterfaceSection();
    kseO_AcousticInterfaceSection( const kseO_AcousticInterfaceSection& copy );
    virtual odb_Section* Copy() const;
    kseO_AcousticInterfaceSection& operator=( const kseO_AcousticInterfaceSection& rhs );

    virtual ~kseO_AcousticInterfaceSection();
    double thickness() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_AcousticInterfaceSection_h
