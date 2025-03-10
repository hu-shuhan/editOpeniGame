//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ContactProperty_h
#define odb_ContactProperty_h

// Begin local includes
#include <odb_InteractionProperty.h>
#include <kinC_NormalBehavior.h>

// Forward declarations
class odb_TangentialBehavior;
class odb_NormalBehavior;
class odb_ThermalConductance;
class ddb_ContactProperty;
class odb_Radiation;

class odb_ContactProperty: public odb_InteractionProperty
{
public: 

  // ONLY TANGENTIAL BEHAVIOR IS SUPPORTED AT THIS TIME
  // member access
  odb_TangentialBehavior tangentialBehavior() const;
      

  // Undocumented and unsupported

 

    // Options

    odb_TangentialBehavior TangentialBehavior(
	    odb_String formulation = "FRICTIONLESS",
	    odb_String directionality = "ISOTROPIC",
	    bool slipRateDependency = false,
	    bool pressureDependency = false,
	    bool temperatureDependency = false,
	    int dependencies = 0,
	    odb_String exponentialDecayDefinition = "COEFFICIENTS",
	    odb_SequenceSequenceFloat table = odb_SequenceSequenceFloat(),
	    double shearStressLimit = DBL_MAX,
	    odb_String maximumElasticSlip = "FRACTION",
	    double fraction = 0.005,
	    double absoluteDistance = 0.0,
	    double elasticSlipStiffness = DBL_MAX,
	    int nStateDependentVars = 0,
	    bool useProperties = false);

    odb_NormalBehavior NormalBehavior( odb_String pressureOverclosure="HARD",
	    bool allowSeparation=true,
            double maxStiffness=DBL_MAX,
            double contactStiffness=DBL_MAX,
            odb_SequenceSequenceFloat table = odb_SequenceSequenceFloat(),
	    kinC_NormalBehavior::EnforcementMethodEnm method =  kinC_NormalBehavior::ANALYSIS_PRODUCT_DEFAULT,
	    double overclosureFactor=0.0,
	    double overclosureMeasure=0.0,
	    double contactStiffnessScaleFactor=1.0,
	    double initialStiffnessScaleFactor=1.0,
	    double clearanceAtZeroContactPressure=0.0);
    odb_NormalBehavior normalBehavior();

    odb_ThermalConductance ThermalConductance(
	    odb_String definition="TABULAR",
	    bool clearanceDependency=true,
            bool pressureDependency=false,
            bool temperatureDependencyC=false,
            bool massFlowRateDependencyC=false,
	    int dependenciesC=0,
            odb_SequenceSequenceFloat clrncDepTble=odb_SequenceSequenceFloat(),
            bool temperatureDependencyP=false,
            bool massFlowRateDependencyP=false,
	    int dependenciesP=0,
            odb_SequenceSequenceFloat pressDepTble=odb_SequenceSequenceFloat());
    odb_ThermalConductance thermalConductance();

    odb_Radiation Radiation(double masterEmissivity=0.0,
	    double slaveEmissivity=0.0,
	    odb_SequenceSequenceFloat table = odb_SequenceSequenceFloat());
    odb_Radiation radiation();


public:   // Undocumented and unsupported

    static unsigned int typeIdentifier();
    // Needed for container
    odb_ContactProperty();

    odb_ContactProperty(const ddb_ContactProperty& ip);

    ddb_ContactProperty contactProperty() const;
};

COW_COW_DECL(odb_ContactProperty);

#endif // odb_ContactProperty_h
