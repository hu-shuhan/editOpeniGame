//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_MohrCoulombPlasticity_h
#define kmaO_MohrCoulombPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_MohrCoulombPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_MohrCoulombHardening;
class kmaO_TensionCutOff;
class kmaC_MohrCoulomb;

class kmaO_MohrCoulombPlasticity: public kmaO_MaterialOption
{
  public:
    kmaO_MohrCoulombPlasticity( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_MohrCoulombPlasticity();
    kmaO_MohrCoulombPlasticity( const odb_SequenceSequenceDouble& table,
                                odb_Union deviatoricEccentricity = "NONE",
                                double meridionalEccentricity = 0.1,
                                bool temperatureDependency = false,
                                int dependencies = 0,
                                bool useTensionCutoff = false );
    kmaO_MohrCoulombPlasticity( const kmaO_MohrCoulombPlasticity& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_MohrCoulombPlasticity& operator=( const kmaO_MohrCoulombPlasticity& rhs );

    virtual ~kmaO_MohrCoulombPlasticity();
    odb_Union deviatoricEccentricity() const;
    double meridionalEccentricity() const;
    bool temperatureDependency() const;
    int dependencies() const;
    bool useTensionCutoff() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_MohrCoulombHardening mohrCoulombHardening() const;
    kmaO_TensionCutOff tensionCutOff() const;
    kmaO_MohrCoulombHardening MohrCoulombHardening( const odb_SequenceSequenceDouble& table,
                                                    bool temperatureDependency = false,
                                                    int dependencies = 0 );
    kmaO_TensionCutOff TensionCutOff( const odb_SequenceSequenceDouble& table,
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    const kmaC_MohrCoulomb* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_MohrCoulombPlasticity_h
