//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_ConnectorPlasticity_h
#define kbeO_ConnectorPlasticity_h
///////////////////////////////////////////////////////////////////////////////
// kbeO_ConnectorPlasticity
//

// Begin local includes
#include <odb_Types.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <odb_String.h>
#include <kbeO_SequenceConnectorPotential.h>
// Forward declarations
class kbeO_ConnectorOptions;
class kbeC_ConnectorPlasticity;

class kbeO_ConnectorPlasticity: public kbeO_ConnectorBehaviorOption
{
  public:
    kbeO_ConnectorPlasticity( const kbeC_ConnectorBehaviorOptionShortcut& shortcut );
    kbeO_ConnectorPlasticity( odb_String coupling = "UNCOUPLED",
                              bool isotropic = true,
                              odb_String isotropicType = "TABULAR",
                              bool isotropicTemperature = false,
                              int isotropicDependencies = 0,
                              bool kinematic = false,
                              odb_String kinematicType = "HALF_CYCLE",
                              bool kinematicTemperature = false,
                              int kinematicDependencies = 0,
                              odb_String forcePotentialOperator = "SUM",
                              double forcePotentialExponent = 2.0,
                              const kbeO_SequenceConnectorPotential& connectorPotentials = kbeO_SequenceConnectorPotential(),
                              const odb_SequenceSequenceDouble& isotropicTable = odb_SequenceSequenceDouble(),
                              const odb_SequenceSequenceDouble& kinematicTable = odb_SequenceSequenceDouble(),
                              const odb_SequenceInt& components = odb_SequenceInt() );
    kbeO_ConnectorPlasticity( const kbeO_ConnectorPlasticity& copy );
    virtual kbeO_ConnectorBehaviorOption* Copy() const;
    kbeO_ConnectorPlasticity& operator=( const kbeO_ConnectorPlasticity& rhs );

    virtual ~kbeO_ConnectorPlasticity();
    odb_String coupling() const;
    bool isotropic() const;
    odb_String isotropicType() const;
    bool isotropicTemperature() const;
    int isotropicDependencies() const;
    bool kinematic() const;
    odb_String kinematicType() const;
    bool kinematicTemperature() const;
    int kinematicDependencies() const;
    odb_String forcePotentialOperator() const;
    double forcePotentialExponent() const;
    kbeO_SequenceConnectorPotential connectorPotentials() const;
    odb_SequenceSequenceDouble isotropicTable() const;
    odb_SequenceSequenceDouble kinematicTable() const;
    odb_SequenceInt components() const;
    kbeO_ConnectorOptions isotropicOptions() const;
    kbeO_ConnectorOptions kinematicOptions() const;
    kbeO_ConnectorOptions IsotropicOptions( bool useBehRegSettings = true,
                                            bool regularize = true,
                                            bool defaultTolerance = true,
                                            double regularization = 0.03,
                                            bool defaultRateFactor = true,
                                            double rateFactor = 0.9,
                                            odb_String interpolation = "LINEAR",
                                            bool useBehExtSettings = true,
                                            odb_String extrapolation = "CONSTANT" );
    kbeO_ConnectorOptions KinematicOptions( bool useBehRegSettings = true,
                                            bool regularize = true,
                                            bool defaultTolerance = true,
                                            double regularization = 0.03,
                                            bool defaultRateFactor = true,
                                            double rateFactor = 0.9,
                                            odb_String interpolation = "LINEAR",
                                            bool useBehExtSettings = true,
                                            odb_String extrapolation = "CONSTANT" );
    const kbeC_ConnectorPlasticity* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kbeO_ConnectorPlasticity_h
