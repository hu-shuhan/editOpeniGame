//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CapCreepConsolidation_h
#define kmaO_CapCreepConsolidation_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CapCreepConsolidation
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_CapCreepConsolidation;

class kmaO_CapCreepConsolidation: public kmaO_MaterialOption
{
  public:
    kmaO_CapCreepConsolidation( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CapCreepConsolidation();
    kmaO_CapCreepConsolidation( const odb_SequenceSequenceDouble& table,
                                odb_String law = "STRAIN",
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_CapCreepConsolidation( const kmaO_CapCreepConsolidation& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CapCreepConsolidation& operator=( const kmaO_CapCreepConsolidation& rhs );

    virtual ~kmaO_CapCreepConsolidation();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CapCreepConsolidation* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CapCreepConsolidation_h
