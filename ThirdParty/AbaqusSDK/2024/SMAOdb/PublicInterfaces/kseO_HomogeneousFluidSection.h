//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_HomogeneousFluidSection_h
#define kseO_HomogeneousFluidSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_HomogeneousFluidSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_FluidSection.h>
// Forward declarations
class kseC_HomogenousFluidSection;

class kseO_HomogeneousFluidSection: public kseO_FluidSection
{
  public:
    kseO_HomogeneousFluidSection( const kseC_SectionShortcut& shortcut );
    kseO_HomogeneousFluidSection();
    kseO_HomogeneousFluidSection( const kseO_HomogeneousFluidSection& copy );
    virtual odb_Section* Copy() const;
    kseO_HomogeneousFluidSection& operator=( const kseO_HomogeneousFluidSection& rhs );

    virtual ~kseO_HomogeneousFluidSection();
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_HomogeneousFluidSection_h
