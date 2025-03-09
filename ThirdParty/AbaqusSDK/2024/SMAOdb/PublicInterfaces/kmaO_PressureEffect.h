//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_PressureEffect_h
#define kmaO_PressureEffect_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_PressureEffect
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_PressureEffect;

class kmaO_PressureEffect: public kmaO_MaterialOption
{
  public:
    kmaO_PressureEffect( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_PressureEffect();
    kmaO_PressureEffect( const odb_SequenceSequenceDouble& table,
                         bool temperatureDependency = false,
                         int dependencies = 0 );
    kmaO_PressureEffect( const kmaO_PressureEffect& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_PressureEffect& operator=( const kmaO_PressureEffect& rhs );

    virtual ~kmaO_PressureEffect();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_PressureEffect* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_PressureEffect_h
