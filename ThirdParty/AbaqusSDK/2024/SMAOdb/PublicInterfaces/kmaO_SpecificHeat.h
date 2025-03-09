//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SpecificHeat_h
#define kmaO_SpecificHeat_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_SpecificHeat
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_SpecificHeat;

class kmaO_SpecificHeat: public kmaO_MaterialOption
{
  public:
    kmaO_SpecificHeat( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_SpecificHeat();
    kmaO_SpecificHeat( const odb_SequenceSequenceDouble& table,
                       odb_String law = "CONSTANTVOLUME",
                       bool temperatureDependency = false,
                       int dependencies = 0 );
    kmaO_SpecificHeat( const kmaO_SpecificHeat& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_SpecificHeat& operator=( const kmaO_SpecificHeat& rhs );

    virtual ~kmaO_SpecificHeat();
    odb_String law() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_SpecificHeat* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_SpecificHeat_h
