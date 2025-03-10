//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ClayHardening_h
#define kmaO_ClayHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ClayHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_ClayHardening;

class kmaO_ClayHardening: public kmaO_MaterialOption
{
  public:
    kmaO_ClayHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ClayHardening();
    kmaO_ClayHardening( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_ClayHardening( const kmaO_ClayHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ClayHardening& operator=( const kmaO_ClayHardening& rhs );

    virtual ~kmaO_ClayHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ClayHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ClayHardening_h
