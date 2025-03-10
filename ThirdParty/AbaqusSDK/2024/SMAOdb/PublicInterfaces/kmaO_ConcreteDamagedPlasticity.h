//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ConcreteDamagedPlasticity_h
#define kmaO_ConcreteDamagedPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ConcreteDamagedPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_ConcreteCompressionHardening;
class kmaO_ConcreteTensionStiffening;
class kmaO_ConcreteCompressionDamage;
class kmaO_ConcreteTensionDamage;
class kmaC_ConcreteDamagedPlasticity;

class kmaO_ConcreteDamagedPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_ConcreteDamagedPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ConcreteDamagedPlasticity();
    kmaO_ConcreteDamagedPlasticity( const odb_SequenceSequenceDouble& table,
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    kmaO_ConcreteDamagedPlasticity( const kmaO_ConcreteDamagedPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ConcreteDamagedPlasticity& operator=( const kmaO_ConcreteDamagedPlasticity& rhs );

    virtual ~kmaO_ConcreteDamagedPlasticity();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_ConcreteCompressionHardening concreteCompressionHardening() const;
    kmaO_ConcreteTensionStiffening concreteTensionStiffening() const;
    kmaO_ConcreteCompressionDamage concreteCompressionDamage() const;
    kmaO_ConcreteTensionDamage concreteTensionDamage() const;
    kmaO_ConcreteCompressionHardening ConcreteCompressionHardening( const odb_SequenceSequenceDouble& table,
                                                                    bool rate = false,
                                                                    bool temperatureDependency = false,
                                                                    int dependencies = 0 );
    kmaO_ConcreteTensionStiffening ConcreteTensionStiffening( const odb_SequenceSequenceDouble& table,
                                                              bool rate = false,
                                                              odb_String type = "STRAIN",
                                                              bool temperatureDependency = false,
                                                              int dependencies = 0 );
    kmaO_ConcreteCompressionDamage ConcreteCompressionDamage( const odb_SequenceSequenceDouble& table,
                                                              double tensionRecovery = 0,
                                                              bool temperatureDependency = false,
                                                              int dependencies = 0 );
    kmaO_ConcreteTensionDamage ConcreteTensionDamage( const odb_SequenceSequenceDouble& table,
                                                      double compressionRecovery = 1,
                                                      odb_String type = "STRAIN",
                                                      bool temperatureDependency = false,
                                                      int dependencies = 0 );
    const kmaC_ConcreteDamagedPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ConcreteDamagedPlasticity_h
