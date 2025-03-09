//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_CohesiveSection_h
#define kseO_CohesiveSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_CohesiveSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kseC_CohesiveSection;

class kseO_CohesiveSection: public odb_Section
{
  public:
    kseO_CohesiveSection( const kseC_SectionShortcut& shortcut );
    kseO_CohesiveSection();
    kseO_CohesiveSection( const kseO_CohesiveSection& copy );
    virtual odb_Section* Copy() const;
    kseO_CohesiveSection& operator=( const kseO_CohesiveSection& rhs );

    virtual ~kseO_CohesiveSection();
    odb_String response() const;
    odb_String initialThicknessType() const;
    double initialThickness() const;
    odb_Union outOfPlaneThickness() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_CohesiveSection_h
