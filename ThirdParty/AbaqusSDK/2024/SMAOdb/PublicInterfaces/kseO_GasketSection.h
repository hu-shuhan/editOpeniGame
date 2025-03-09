//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_GasketSection_h
#define kseO_GasketSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_GasketSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_Union.h>
// Forward declarations
class kseC_GasketSection;

class kseO_GasketSection: public odb_Section
{
  public:
    kseO_GasketSection( const kseC_SectionShortcut& shortcut );
    kseO_GasketSection();
    kseO_GasketSection( const kseO_GasketSection& copy );
    virtual odb_Section* Copy() const;
    kseO_GasketSection& operator=( const kseO_GasketSection& rhs );

    virtual ~kseO_GasketSection();
    double crossSection() const;
    double initialGap() const;
    odb_Union initialThickness() const;
    double initialVoid() const;
    odb_Union stabilizationStiffness() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_GasketSection_h
