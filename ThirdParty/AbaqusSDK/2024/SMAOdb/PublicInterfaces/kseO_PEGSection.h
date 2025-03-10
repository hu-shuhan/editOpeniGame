//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_PEGSection_h
#define kseO_PEGSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_PEGSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_PEGSection;

class kseO_PEGSection: public odb_Section
{
  public:
    kseO_PEGSection( const kseC_SectionShortcut& shortcut );
    kseO_PEGSection();
    kseO_PEGSection( const kseO_PEGSection& copy );
    virtual odb_Section* Copy() const;
    kseO_PEGSection& operator=( const kseO_PEGSection& rhs );

    virtual ~kseO_PEGSection();
    double thickness() const;
    double wedgeAngle1() const;
    double wedgeAngle2() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_PEGSection_h
