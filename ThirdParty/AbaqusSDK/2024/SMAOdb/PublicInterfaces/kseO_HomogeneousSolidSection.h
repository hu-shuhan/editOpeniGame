//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_HomogeneousSolidSection_h
#define kseO_HomogeneousSolidSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_HomogeneousSolidSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_SolidSection.h>
// Forward declarations
class kseC_HomogenousSolidSection;

class kseO_HomogeneousSolidSection: public kseO_SolidSection
{
  public:
    kseO_HomogeneousSolidSection( const kseC_SectionShortcut& shortcut );
    kseO_HomogeneousSolidSection();
    kseO_HomogeneousSolidSection( const kseO_HomogeneousSolidSection& copy );
    virtual odb_Section* Copy() const;
    kseO_HomogeneousSolidSection& operator=( const kseO_HomogeneousSolidSection& rhs );

    virtual ~kseO_HomogeneousSolidSection();
    double thickness() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_HomogeneousSolidSection_h
