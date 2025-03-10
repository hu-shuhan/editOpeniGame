//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CapHardening_h
#define kmaO_CapHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CapHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_CapHardening;

class kmaO_CapHardening: public kmaO_MaterialOption
{
  public:
    kmaO_CapHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CapHardening();
    kmaO_CapHardening( const odb_SequenceSequenceDouble& table,
                       bool temperatureDependency = false,
                       int dependencies = 0 );
    kmaO_CapHardening( const kmaO_CapHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CapHardening& operator=( const kmaO_CapHardening& rhs );

    virtual ~kmaO_CapHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CapHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CapHardening_h
