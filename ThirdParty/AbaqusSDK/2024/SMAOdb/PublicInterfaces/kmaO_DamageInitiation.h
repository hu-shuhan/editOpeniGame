//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_DamageInitiation_h
#define kmaO_DamageInitiation_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_DamageInitiation
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
#include <odb_String.h>
#include <odb_Union.h>
// Forward declarations
class kmaO_DamageEvolution;
class kmaO_DamageStabilization;
class kmaO_DamageStabilizationCohesive;
class kmaC_DamageInitiation;

class kmaO_DamageInitiation: public kmaO_MaterialOption
{
  public:
    kmaO_DamageInitiation( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_DamageInitiation();
    kmaO_DamageInitiation( const odb_SequenceSequenceDouble& table,
                           odb_String definition = "MSFLD",
                           double feq = 10.0,
                           double fnn = 10.0,
                           double fnt = 10.0,
                           int frequency = 1,
                           double ks = 0.0,
                           int numberImperfections = 4,
                           bool temperatureDependency = false,
                           int dependencies = 0,
                           double alpha = 0.0,
                           double omega = 1.0,
                           double tolerance = 0.05,
                           odb_String direction = "NMORI" );
    kmaO_DamageInitiation( const kmaO_DamageInitiation& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_DamageInitiation& operator=( const kmaO_DamageInitiation& rhs );

    virtual ~kmaO_DamageInitiation();
    odb_String definition() const;
    double feq() const;
    double fnn() const;
    double fnt() const;
    int frequency() const;
    double ks() const;
    int numberImperfections() const;
    bool temperatureDependency() const;
    int dependencies() const;
    double alpha() const;
    double omega() const;
    double tolerance() const;
    odb_String direction() const;
    odb_SequenceSequenceDouble table() const;
    kmaO_DamageEvolution damageEvolution() const;
    kmaO_DamageStabilization damageStabilization() const;
    kmaO_DamageStabilizationCohesive damageStabilizationCohesive() const;
    kmaO_DamageEvolution DamageEvolution( odb_String type,
                                          const odb_SequenceSequenceDouble& table,
                                          odb_String degradation = "MAXIMUM",
                                          bool temperatureDependency = false,
                                          int dependencies = 0,
                                          odb_String mixedModeBehavior = "MODE_INDEPENDENT",
                                          odb_String modeMixRatio = "ENERGY",
                                          odb_Union power = "NONE",
                                          odb_String softening = "LINEAR" );
    kmaO_DamageStabilization DamageStabilization( double fiberTensileCoeff,
                                                  double fiberCompressiveCoeff,
                                                  double matrixTensileCoeff,
                                                  double matrixCompressiveCoeff );
    kmaO_DamageStabilizationCohesive DamageStabilizationCohesive( odb_Union cohesiveCoeff = "NONE" );
    const kmaC_DamageInitiation* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_DamageInitiation_h
