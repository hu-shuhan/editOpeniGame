//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CastIronTensionHardening_h
#define kmaO_CastIronTensionHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CastIronTensionHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_CastIronTensHard;

class kmaO_CastIronTensionHardening: public kmaO_MaterialOption
{
  public:
    kmaO_CastIronTensionHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CastIronTensionHardening();
    kmaO_CastIronTensionHardening( const odb_SequenceSequenceDouble& table,
                                   bool temperatureDependency = false,
                                   int dependencies = 0 );
    kmaO_CastIronTensionHardening( const kmaO_CastIronTensionHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CastIronTensionHardening& operator=( const kmaO_CastIronTensionHardening& rhs );

    virtual ~kmaO_CastIronTensionHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CastIronTensHard* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CastIronTensionHardening_h
