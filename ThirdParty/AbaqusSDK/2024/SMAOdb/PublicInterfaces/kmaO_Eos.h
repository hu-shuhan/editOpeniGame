//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Eos_h
#define kmaO_Eos_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Eos
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
// Forward declarations
class kmaO_DetonationPoint;
class kmaO_EosCompaction;
class kmaC_Eos;

class kmaO_Eos: public kmaO_MaterialOption
{
  public:
    kmaO_Eos( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Eos( odb_String type = "IDEALGAS",
              bool temperatureDependency = false,
              int dependencies = 0,
              double detonationEnergy = 0.0,
              const odb_SequenceSequenceDouble& solidTable = odb_SequenceSequenceDouble(),
              const odb_SequenceSequenceDouble& gasTable = odb_SequenceSequenceDouble(),
              const odb_SequenceSequenceDouble& reactionTable = odb_SequenceSequenceDouble(),
              const odb_SequenceSequenceDouble& gasSpecificTable = odb_SequenceSequenceDouble(),
              const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_Eos( const kmaO_Eos& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Eos& operator=( const kmaO_Eos& rhs );

    virtual ~kmaO_Eos();
    odb_String type() const;
    bool temperatureDependency() const;
    int dependencies() const;
    double detonationEnergy() const;
    odb_SequenceSequenceDouble solidTable() const;
    odb_SequenceSequenceDouble gasTable() const;
    odb_SequenceSequenceDouble reactionTable() const;
    odb_SequenceSequenceDouble gasSpecificTable() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_DetonationPoint detonationPoint() const;
    kmaO_EosCompaction eosCompaction() const;
    kmaO_DetonationPoint DetonationPoint( const odb_SequenceSequenceDouble& table );
    kmaO_EosCompaction EosCompaction( double soundSpeed,
                                      double porosity,
                                      double pressure,
                                      double compactionPressure );
    const kmaC_Eos* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Eos_h
