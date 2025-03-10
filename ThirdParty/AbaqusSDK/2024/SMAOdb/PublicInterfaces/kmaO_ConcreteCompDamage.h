//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ConcreteCompressionDamage_h
#define kmaO_ConcreteCompressionDamage_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ConcreteCompressionDamage
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_ConcreteCompressionDamage;

class kmaO_ConcreteCompressionDamage: public kmaO_MaterialOption
{
  public:
    kmaO_ConcreteCompressionDamage( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ConcreteCompressionDamage();
    kmaO_ConcreteCompressionDamage( const odb_SequenceSequenceDouble& table,
                                    double tensionRecovery = 0,
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    kmaO_ConcreteCompressionDamage( const kmaO_ConcreteCompressionDamage& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ConcreteCompressionDamage& operator=( const kmaO_ConcreteCompressionDamage& rhs );

    virtual ~kmaO_ConcreteCompressionDamage();
    double tensionRecovery() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ConcreteCompressionDamage* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ConcreteCompressionDamage_h
