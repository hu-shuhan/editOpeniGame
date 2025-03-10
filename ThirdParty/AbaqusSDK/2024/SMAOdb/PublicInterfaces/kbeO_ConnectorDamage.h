//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorDamage_h
#define kbeO_ConnectorDamage_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorDamage
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_String.h>
#include <kbeO_SequenceConnectorPotential.h>
// Forward declarations
class kbeO_ConnectorOptions;
class kbeC_ConnectorDamage;

class kbeO_ConnectorDamage: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorDamage( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorDamage( odb_String coupling = "UNCOUPLED",
                          odb_String criterion = "FORCE",
                          bool initiationTemperature = false,
                          odb_String initiationPotentialOperator = "SUM",
                          double initiationPotentialExponent = 2.0,
                          int initiationDependencies = 0,
                          bool evolution = true,
                          odb_String evolutionType = "MOTION_TYPE",
                          odb_String softening = "LINEAR",
                          bool useAffected = false,
                          odb_String degradation = "MAXIMUM",
                          bool evolutionTemperature = false,
                          int evolutionDependencies = 0,
                          odb_String evolutionPotentialOperator = "SUM",
                          double evolutionPotentialExponent = 2.0,
                          const kbeO_SequenceConnectorPotential& initiationPotentials = kbeO_SequenceConnectorPotential(),
                          const kbeO_SequenceConnectorPotential& evolutionPotentials = kbeO_SequenceConnectorPotential(),
                          const odb_SequenceSequenceDouble& initiationTable = odb_SequenceSequenceDouble(),
                          const odb_SequenceSequenceDouble& evolutionTable = odb_SequenceSequenceDouble(),
                          const odb_SequenceInt& affectedComponents = odb_SequenceInt(),
                          const odb_SequenceInt& components = odb_SequenceInt() );
    kbeO_ConnectorDamage( const kbeO_ConnectorDamage& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorDamage& operator=( const kbeO_ConnectorDamage& rhs );

    virtual ~kbeO_ConnectorDamage();
    odb_String coupling() const;
    odb_String criterion() const;
    bool initiationTemperature() const;
    odb_String initiationPotentialOperator() const;
    double initiationPotentialExponent() const;
    int initiationDependencies() const;
    bool evolution() const;
    odb_String evolutionType() const;
    odb_String softening() const;
    bool useAffected() const;
    odb_String degradation() const;
    bool evolutionTemperature() const;
    int evolutionDependencies() const;
    odb_String evolutionPotentialOperator() const;
    double evolutionPotentialExponent() const;
    kbeO_SequenceConnectorPotential initiationPotentials() const;
    kbeO_SequenceConnectorPotential evolutionPotentials() const;
    odb_SequenceSequenceDouble initiationTable() const;
    odb_SequenceSequenceDouble evolutionTable() const;
    odb_SequenceInt affectedComponents() const;
    odb_SequenceInt components() const;
    kbeO_ConnectorOptions initiationOptions() const;
    kbeO_ConnectorOptions evolutionOptions() const;
    kbeO_ConnectorOptions InitiationOptions( bool useBehRegSettings = true,
                                             bool regularize = true,
                                             bool defaultTolerance = true,
                                             double regularization = 0.03,
                                             bool defaultRateFactor = true,
                                             double rateFactor = 0.9,
                                             odb_String interpolation = "LINEAR",
                                             bool useBehExtSettings = true,
                                             odb_String extrapolation = "CONSTANT" );
    kbeO_ConnectorOptions EvolutionOptions( bool useBehRegSettings = true,
                                            bool regularize = true,
                                            bool defaultTolerance = true,
                                            double regularization = 0.03,
                                            bool defaultRateFactor = true,
                                            double rateFactor = 0.9,
                                            odb_String interpolation = "LINEAR",
                                            bool useBehExtSettings = true,
                                            odb_String extrapolation = "CONSTANT" );
    const kbeC_ConnectorDamage* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorDamage_h
