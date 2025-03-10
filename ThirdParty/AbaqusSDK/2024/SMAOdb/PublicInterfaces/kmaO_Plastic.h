//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Plastic_h
#define kmaO_Plastic_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Plastic
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_RateDependent;
class kmaO_Potential;
class kmaO_CyclicHardening;
class kmaO_Ornl;
class kmaO_CycledPlastic;
class kmaO_AnnealTemperature;
class kmaC_Plastic;

class kmaO_Plastic: public kmaO_MaterialOption
{
  public:
    kmaO_Plastic( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Plastic();
    kmaO_Plastic( const odb_SequenceSequenceDouble& table,
                  odb_String hardening = "ISOTROPIC",
                  bool rate = false,
                  odb_String dataType = "HALF_CYCLE",
                  bool strainRangeDependency = false,
                  int numBackstresses = 1,
                  bool temperatureDependency = false,
                  int dependencies = 0 );
    kmaO_Plastic( const kmaO_Plastic& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Plastic& operator=( const kmaO_Plastic& rhs );

    virtual ~kmaO_Plastic();
    odb_String hardening() const;
    bool rate() const;
    odb_String dataType() const;
    bool strainRangeDependency() const;
    int numBackstresses() const;
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_RateDependent rateDependent() const;
    kmaO_Potential potential() const;
    kmaO_CyclicHardening cyclicHardening() const;
    kmaO_Ornl ornl() const;
    kmaO_CycledPlastic cycledPlastic() const;
    kmaO_AnnealTemperature annealTemperature() const;
    kmaO_RateDependent RateDependent( const odb_SequenceSequenceDouble& table,
                                      odb_String type = "POWER_LAW",
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_Potential Potential( const odb_SequenceSequenceDouble& table,
                              bool temperatureDependency = false,
                              int dependencies = 0 );
    kmaO_CyclicHardening CyclicHardening( const odb_SequenceSequenceDouble& table,
                                          bool temperatureDependency = false,
                                          int dependencies = 0,
                                          bool parameters = false );
    kmaO_Ornl Ornl( double a = 0.3,
                    odb_Union h = "NONE",
                    bool reset = false );
    kmaO_CycledPlastic CycledPlastic( const odb_SequenceSequenceDouble& table,
                                      bool temperatureDependency = false );
    kmaO_AnnealTemperature AnnealTemperature( const odb_SequenceSequenceDouble& table,
                                              int dependencies = 0 );
    const kmaC_Plastic* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Plastic_h
