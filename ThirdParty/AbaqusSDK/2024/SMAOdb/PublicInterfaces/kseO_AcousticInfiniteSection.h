//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_AcousticInfiniteSection_h
#define kseO_AcousticInfiniteSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_AcousticInfiniteSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
// Forward declarations
class kseC_AcousticInfiniteSection;

class kseO_AcousticInfiniteSection: public odb_Section
{
  public:
    kseO_AcousticInfiniteSection( const kseC_SectionShortcut& shortcut );
    kseO_AcousticInfiniteSection();
    kseO_AcousticInfiniteSection( const kseO_AcousticInfiniteSection& copy );
    virtual odb_Section* Copy() const;
    kseO_AcousticInfiniteSection& operator=( const kseO_AcousticInfiniteSection& rhs );

    virtual ~kseO_AcousticInfiniteSection();
    double thickness() const;
    int order() const;
    odb_String material() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_AcousticInfiniteSection_h
