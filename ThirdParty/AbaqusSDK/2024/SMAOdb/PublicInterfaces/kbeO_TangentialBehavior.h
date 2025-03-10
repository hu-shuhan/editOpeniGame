//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_TangentialBehavior_h
#define kbeO_TangentialBehavior_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_TangentialBehavior
//

// Begin local includes
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kbeC_TangentialBehaviorShortcut.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kbeC_TangentialBehavior;

class kbeO_TangentialBehavior: public mem_AllocationOperators
{
  public:
    kbeO_TangentialBehavior( const kbeC_TangentialBehaviorShortcut& shortcut );
    kbeO_TangentialBehavior();
    kbeO_TangentialBehavior( const kbeO_TangentialBehavior& copy );
    virtual kbeO_TangentialBehavior* Copy() const;
    kbeO_TangentialBehavior& operator=( const kbeO_TangentialBehavior& rhs );

    virtual ~kbeO_TangentialBehavior();
    odb_String formulation() const;
    bool slipRateDependency() const;
    bool pressureDependency() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String exponentialDecayDefinition() const;
    odb_Union shearStressLimit() const;
    odb_String maximumElasticSlip() const;
    double fraction() const;
    odb_Union absoluteDistance() const;
    odb_SequenceSequenceDouble table() const;
    kbeC_TangentialBehaviorShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kbeC_TangentialBehaviorShortcut shortcut;
};

#endif // kbeO_TangentialBehavior_h
