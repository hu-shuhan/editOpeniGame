//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_DiscreteSection_h
#define kseO_DiscreteSection_h
///////////////////////////////////////////////////////////////////////////////
// kseO_DiscreteSection
//

// Begin local includes
#include <odb_Types.h>
#include <odb_Section.h>
#include <odb_String.h>
#include <odb_Union.h>

// Forward declarations


class kseO_DiscreteSection: public odb_Section
{
  public:
    kseO_DiscreteSection( const kseC_SectionShortcut& shortcut );
    kseO_DiscreteSection();
    kseO_DiscreteSection( const kseO_DiscreteSection& copy );
    virtual odb_Section* Copy() const;
    kseO_DiscreteSection& operator=( const kseO_DiscreteSection& rhs );

    virtual ~kseO_DiscreteSection();
    
    odb_Union density() const;
    odb_Union radius() const;
    // TODO
    // ShapeEnum

    static unsigned int typeIdentifier();
};

#endif // kseO_DiscreteSection_h


