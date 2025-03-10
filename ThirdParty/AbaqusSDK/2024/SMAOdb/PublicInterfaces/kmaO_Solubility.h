//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Solubility_h
#define kmaO_Solubility_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Solubility
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Solubility;

class kmaO_Solubility: public kmaO_MaterialOption
{
  public:
    kmaO_Solubility( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Solubility();
    kmaO_Solubility( const odb_SequenceSequenceDouble& table,
                     bool temperatureDependency = false,
                     int dependencies = 0 );
    kmaO_Solubility( const kmaO_Solubility& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Solubility& operator=( const kmaO_Solubility& rhs );

    virtual ~kmaO_Solubility();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Solubility* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Solubility_h
