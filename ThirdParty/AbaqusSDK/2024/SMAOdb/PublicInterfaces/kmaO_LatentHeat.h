//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_LatentHeat_h
#define kmaO_LatentHeat_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_LatentHeat
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_LatentHeat;

class kmaO_LatentHeat: public kmaO_MaterialOption
{
  public:
    kmaO_LatentHeat( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_LatentHeat();
    kmaO_LatentHeat( const odb_SequenceSequenceDouble& table );
    kmaO_LatentHeat( const kmaO_LatentHeat& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_LatentHeat& operator=( const kmaO_LatentHeat& rhs );

    virtual ~kmaO_LatentHeat();
    odb_SequenceSequenceDouble table() const;
    const kmaC_LatentHeat* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_LatentHeat_h
