//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorFriction_h
#define kbeO_ConnectorFriction_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorFriction
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_String.h>
#include <odb_Union.h>
#include <kbeO_SequenceConnectorPotential.h>
#include <kbeO_SequenceCDCTerm.h>
// Forward declarations
class kbeO_TangentialBehavior;
class kbeO_DerivedComponent;
class kbeO_ConnectorOptions;
class kbeC_ConnectorFriction;

class kbeO_ConnectorFriction: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorFriction( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorFriction( odb_String frictionModel = "PREDEFINED",
                            odb_String slipStyle = "SPECIFY",
                            odb_Union tangentDirection = "NONE",
                            odb_Union stickStiffness = "NONE",
                            odb_String componentType = "NO_INDEPENDENT_COMPONENTS",
                            bool slipDependency = false,
                            bool temperatureDependency = false,
                            int dependencies = 0,
                            bool useContactForceComponent = false,
                            odb_String contactForceStyle = "COMPONENT_NUMBER",
                            int contactForceComponent = 0,
                            odb_String forcePotentialOperator = "SUM",
                            double forcePotentialExponent = 2.0,
                            const kbeO_SequenceConnectorPotential& connectorPotentials = kbeO_SequenceConnectorPotential(),
                            const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble(),
                            const odb_SequenceInt& independentComponents = odb_SequenceInt() );
    kbeO_ConnectorFriction( const kbeO_ConnectorFriction& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorFriction& operator=( const kbeO_ConnectorFriction& rhs );

    virtual ~kbeO_ConnectorFriction();
    odb_String frictionModel() const;
    odb_String slipStyle() const;
    odb_Union tangentDirection() const;
    odb_Union stickStiffness() const;
    odb_String componentType() const;
    bool slipDependency() const;
    bool temperatureDependency() const;
    int dependencies() const;
    bool useContactForceComponent() const;
    odb_String contactForceStyle() const;
    int contactForceComponent() const;
    odb_String forcePotentialOperator() const;
    double forcePotentialExponent() const;
    kbeO_SequenceConnectorPotential connectorPotentials() const;
    odb_SequenceSequenceDouble table() const;
    kbeO_TangentialBehavior tangentialBehavior() const;
    odb_SequenceInt independentComponents() const;
    kbeO_DerivedComponent derivedComponent() const;
    kbeO_ConnectorOptions options() const;
    kbeO_TangentialBehavior TangentialBehavior( odb_String formulation = "PENALTY",
                                                bool slipRateDependency = false,
                                                bool pressureDependency = false,
                                                bool temperatureDependency = false,
                                                int dependencies = 0,
                                                odb_String exponentialDecayDefinition = "COEFFICIENTS",
                                                odb_Union shearStressLimit = "NONE",
                                                odb_String maximumElasticSlip = "FRACTION",
                                                double fraction = 0.0001,
                                                odb_Union absoluteDistance = "NONE",
                                                const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kbeO_DerivedComponent DerivedComponent( const kbeO_SequenceCDCTerm& cdcTerms = kbeO_SequenceCDCTerm() );
    kbeO_ConnectorOptions Options( bool useBehRegSettings = true,
                                   bool regularize = true,
                                   bool defaultTolerance = true,
                                   double regularization = 0.03,
                                   bool defaultRateFactor = true,
                                   double rateFactor = 0.9,
                                   odb_String interpolation = "LINEAR",
                                   bool useBehExtSettings = true,
                                   odb_String extrapolation = "CONSTANT" );
    const kbeC_ConnectorFriction* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorFriction_h
