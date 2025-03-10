//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_FluidSection_h
#define kseO_FluidSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_FluidSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_FluidSection;

class kseO_FluidSection: public odb_Section
{
public:
    kseO_FluidSection( const kseC_SectionShortcut& shortcut );
    kseO_FluidSection();
    virtual odb_Section* Copy() const;
    virtual ~kseO_FluidSection();
    static unsigned int typeIdentifier();
};

#endif // kseO_FluidSection_h
