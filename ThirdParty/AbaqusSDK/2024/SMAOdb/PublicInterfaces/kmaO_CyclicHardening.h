//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CyclicHardening_h
#define kmaO_CyclicHardening_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CyclicHardening
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_CyclicHardening;

class kmaO_CyclicHardening: public kmaO_MaterialOption
{
  public:
    kmaO_CyclicHardening( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CyclicHardening();
    kmaO_CyclicHardening( const odb_SequenceSequenceDouble& table,
                          bool temperatureDependency = false,
                          int dependencies = 0,
                          bool parameters = false );
    kmaO_CyclicHardening( const kmaO_CyclicHardening& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CyclicHardening& operator=( const kmaO_CyclicHardening& rhs );

    virtual ~kmaO_CyclicHardening();
    bool temperatureDependency() const;
    int dependencies() const;
    bool parameters() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CyclicHardening* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CyclicHardening_h
