//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_TensionCutOff_h
#define kmaO_TensionCutOff_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_TensionCutOff
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_TensionCutOff;

class kmaO_TensionCutOff: public kmaO_MaterialOption
{
  public:
    kmaO_TensionCutOff( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_TensionCutOff();
    kmaO_TensionCutOff( const odb_SequenceSequenceDouble& table,
                        bool temperatureDependency = false,
                        int dependencies = 0 );
    kmaO_TensionCutOff( const kmaO_TensionCutOff& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_TensionCutOff& operator=( const kmaO_TensionCutOff& rhs );

    virtual ~kmaO_TensionCutOff();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_TensionCutOff* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_TensionCutOff_h
