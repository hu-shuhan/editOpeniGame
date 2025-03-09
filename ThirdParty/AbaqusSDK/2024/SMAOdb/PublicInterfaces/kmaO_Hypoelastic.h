//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Hypoelastic_h
#define kmaO_Hypoelastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Hypoelastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Hypoelastic;

class kmaO_Hypoelastic: public kmaO_MaterialOption
{
  public:
    kmaO_Hypoelastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Hypoelastic();
    kmaO_Hypoelastic( const odb_SequenceSequenceDouble& table,
                      bool user = false );
    kmaO_Hypoelastic( const kmaO_Hypoelastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Hypoelastic& operator=( const kmaO_Hypoelastic& rhs );

    virtual ~kmaO_Hypoelastic();
    bool user() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Hypoelastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Hypoelastic_h
