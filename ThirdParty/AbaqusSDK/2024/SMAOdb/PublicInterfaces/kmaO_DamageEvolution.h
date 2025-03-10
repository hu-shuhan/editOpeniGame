//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DamageEvolution_h
#define kmaO_DamageEvolution_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DamageEvolution
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaC_DamageEvolution;

class kmaO_DamageEvolution: public kmaO_MaterialOption
{
  public:
    kmaO_DamageEvolution( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DamageEvolution();
    kmaO_DamageEvolution( odb_String type,
                          const odb_SequenceSequenceDouble& table,
                          odb_String degradation = "MAXIMUM",
                          bool temperatureDependency = false,
                          int dependencies = 0,
                          odb_String mixedModeBehavior = "MODE_INDEPENDENT",
                          odb_String modeMixRatio = "ENERGY",
                          odb_Union power = "NONE",
                          odb_String softening = "LINEAR" );
    kmaO_DamageEvolution( const kmaO_DamageEvolution& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DamageEvolution& operator=( const kmaO_DamageEvolution& rhs );

    virtual ~kmaO_DamageEvolution();
    odb_String type() const;
    odb_String degradation() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_String mixedModeBehavior() const;
    odb_String modeMixRatio() const;
    odb_Union power() const;
    odb_String softening() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_DamageEvolution* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DamageEvolution_h
