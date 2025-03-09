//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_CycledPlastic_h
#define kmaO_CycledPlastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_CycledPlastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_CycledPlastic;

class kmaO_CycledPlastic: public kmaO_MaterialOption
{
  public:
    kmaO_CycledPlastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_CycledPlastic();
    kmaO_CycledPlastic( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false );
    kmaO_CycledPlastic( const kmaO_CycledPlastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_CycledPlastic& operator=( const kmaO_CycledPlastic& rhs );

    virtual ~kmaO_CycledPlastic();
    bool temperatureDependency() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_CycledPlastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_CycledPlastic_h
