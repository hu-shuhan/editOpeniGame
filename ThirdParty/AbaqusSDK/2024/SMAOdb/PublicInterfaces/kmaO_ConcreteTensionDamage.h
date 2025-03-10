//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ConcreteTensionDamage_h
#define kmaO_ConcreteTensionDamage_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ConcreteTensionDamage
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaC_ConcreteTensionDamage;

class kmaO_ConcreteTensionDamage: public kmaO_MaterialOption
{
  public:
    kmaO_ConcreteTensionDamage( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ConcreteTensionDamage();
    kmaO_ConcreteTensionDamage( const odb_SequenceSequenceDouble& table,
                                double compressionRecovery = 1,
                                odb_String type = "STRAIN",
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_ConcreteTensionDamage( const kmaO_ConcreteTensionDamage& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ConcreteTensionDamage& operator=( const kmaO_ConcreteTensionDamage& rhs );

    virtual ~kmaO_ConcreteTensionDamage();
    double compressionRecovery() const;
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ConcreteTensionDamage* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ConcreteTensionDamage_h
