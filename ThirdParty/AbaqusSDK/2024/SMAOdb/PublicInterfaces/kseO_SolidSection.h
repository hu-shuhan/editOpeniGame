//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_SolidSection_h
#define kseO_SolidSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_SolidSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_SolidSection;

class kseO_SolidSection: public odb_Section
{
  public:
    kseO_SolidSection( const kseC_SectionShortcut& shortcut );
    kseO_SolidSection();
    virtual odb_Section* Copy() const;
    virtual ~kseO_SolidSection();
    static unsigned int typeIdentifier();
};

#endif // kseO_SolidSection_h
