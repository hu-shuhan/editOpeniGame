//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_DerivedComponent_h
#define kbeO_DerivedComponent_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_DerivedComponent
//

// Begin local includes
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kbeC_DerivedComponentShortcut.h>
#include <kbeO_SequenceCDCTerm.h>
// Forward declarations
class kbeC_DerivedComponent;

class kbeO_DerivedComponent: public mem_AllocationOperators
{
  public:
    kbeO_DerivedComponent( const kbeC_DerivedComponentShortcut& shortcut );
    kbeO_DerivedComponent();
    kbeO_DerivedComponent( const kbeO_DerivedComponent& copy );
    virtual kbeO_DerivedComponent* Copy() const;
    kbeO_DerivedComponent& operator=( const kbeO_DerivedComponent& rhs );

    virtual ~kbeO_DerivedComponent();
    kbeO_SequenceCDCTerm cdcTerms() const;
    kbeC_DerivedComponentShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kbeC_DerivedComponentShortcut shortcut;
};

#endif // kbeO_DerivedComponent_h
