//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_TrussSection_h
#define kseO_TrussSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_TrussSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_TrussSection;

class kseO_TrussSection: public odb_Section
{
  public:
    kseO_TrussSection( const kseC_SectionShortcut& shortcut );
    kseO_TrussSection();
    kseO_TrussSection( const kseO_TrussSection& copy );
    virtual odb_Section* Copy() const;
    kseO_TrussSection& operator=( const kseO_TrussSection& rhs );

    virtual ~kseO_TrussSection();
    double area() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_TrussSection_h
