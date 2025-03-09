//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DruckerPragerCreep_h
#define kmaO_DruckerPragerCreep_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DruckerPragerCreep
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_DruckerPragerCreep;

class kmaO_DruckerPragerCreep: public kmaO_MaterialOption
{
  public:
    kmaO_DruckerPragerCreep( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DruckerPragerCreep();
    kmaO_DruckerPragerCreep( const odb_SequenceSequenceDouble& table,
                             odb_String law = "STRAIN",
                             bool temperatureDependency = false,
                             int dependencies = 0 );
    kmaO_DruckerPragerCreep( const kmaO_DruckerPragerCreep& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DruckerPragerCreep& operator=( const kmaO_DruckerPragerCreep& rhs );

    virtual ~kmaO_DruckerPragerCreep();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_DruckerPragerCreep* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DruckerPragerCreep_h
