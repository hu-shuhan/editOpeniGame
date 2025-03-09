//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Hysteresis_h
#define kmaO_Hysteresis_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Hysteresis
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Hysteresis;

class kmaO_Hysteresis: public kmaO_MaterialOption
{
  public:
    kmaO_Hysteresis( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Hysteresis();
    kmaO_Hysteresis( const odb_SequenceSequenceDouble& table );
    kmaO_Hysteresis( const kmaO_Hysteresis& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Hysteresis& operator=( const kmaO_Hysteresis& rhs );

    virtual ~kmaO_Hysteresis();
    odb_SequenceSequenceDouble table() const;
    const kmaC_Hysteresis* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Hysteresis_h
