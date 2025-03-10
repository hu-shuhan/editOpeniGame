//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SoretEffect_h
#define kmaO_SoretEffect_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_SoretEffect
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_SoretEffect;

class kmaO_SoretEffect: public kmaO_MaterialOption
{
  public:
    kmaO_SoretEffect( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_SoretEffect();
    kmaO_SoretEffect( const odb_SequenceSequenceDouble& table,
                      bool temperatureDependency = false,
                      int dependencies = 0 );
    kmaO_SoretEffect( const kmaO_SoretEffect& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_SoretEffect& operator=( const kmaO_SoretEffect& rhs );

    virtual ~kmaO_SoretEffect();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_SoretEffect* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_SoretEffect_h
