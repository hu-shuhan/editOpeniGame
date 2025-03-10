//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_EulerianSection_h
#define kseO_EulerianSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_EulerianSection
//

// Begin local includes
#include <odb_Types.h>
#include <kseO_SolidSection.h>
// Forward declarations
class kseC_EulerianSection;

class kseO_EulerianSection: public kseO_SolidSection
{
  public:
    kseO_EulerianSection( const kseC_SectionShortcut& shortcut );
    kseO_EulerianSection();
    kseO_EulerianSection( const kseO_EulerianSection& copy );
    virtual odb_Section* Copy() const;
    kseO_EulerianSection& operator=( const kseO_EulerianSection& rhs );

    virtual ~kseO_EulerianSection();
    odb_String defaultMaterial() const;
    odb_SequenceString materialMapKeys() const;
    odb_SequenceString materialMapValues() const;
    static unsigned int typeIdentifier();
};

#endif // kseO_EulerianSection_h
