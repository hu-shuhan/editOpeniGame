//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_FailStress_h
#define kmaO_FailStress_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_FailStress
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_FailStress;

class kmaO_FailStress: public kmaO_MaterialOption
{
  public:
    kmaO_FailStress( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_FailStress();
    kmaO_FailStress( const odb_SequenceSequenceDouble& table,
                     bool temperatureDependency = false,
                     int dependencies = 0 );
    kmaO_FailStress( const kmaO_FailStress& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_FailStress& operator=( const kmaO_FailStress& rhs );

    virtual ~kmaO_FailStress();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_FailStress* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_FailStress_h
