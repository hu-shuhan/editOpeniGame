//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Material_h
#define kmaO_Material_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Material
//

// Begin local includes
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <mem_AllocationOperators.h>
#include <kmaC_MaterialShortcut.h>
#include <odb_String.h>
#include <odb_Union.h>
#include <kmaO_SequenceUniaxialTestData.h>
#include <kmaO_SequenceBiaxialTestData.h>
#include <kmaO_SequencePlanarTestData.h>
// Forward declarations
class kmaO_AcousticMedium;
class kmaO_BrittleCracking;
class kmaO_CapPlasticity;
class kmaO_CastIronPlasticity;
class kmaO_ClayPlasticity;
class kmaO_Concrete;
class kmaO_ConcreteDamagedPlasticity;
class kmaO_Conductivity;
class kmaO_Creep;
class kmaO_CrushableFoam;
class kmaO_DamageInitiation;
class kmaO_Damping;
class kmaO_DeformationPlasticity;
class kmaO_Density;
class kmaO_Depvar;
class kmaO_Dielectric;
class kmaO_Diffusivity;
class kmaO_DruckerPrager;
class kmaO_Elastic;
class kmaO_ElectricalConductivity;
class kmaO_Eos;
class kmaO_Expansion;
class kmaO_FluidLeakoff;
class kmaO_GapFlow;
class kmaO_GasketThicknessBehavior;
class kmaO_GasketTransverseShearElastic;
class kmaO_GasketMembraneElastic;
class kmaO_Gel;
class kmaO_HeatGeneration;
class kmaO_Hyperelastic;
class kmaO_Hyperfoam;
class kmaO_Hypoelastic;
class kmaO_InelasticHeatFraction;
class kmaO_JouleHeatFraction;
class kmaO_LatentHeat;
class kmaO_LowDensityFoam;
class kmaO_MohrCoulombPlasticity;
class kmaO_MoistureSwelling;
class kmaO_MullinsEffect;
class kmaO_Permeability;
class kmaO_Piezoelectric;
class kmaO_Plastic;
class kmaO_PoreFluidExpansion;
class kmaO_PorousBulkModuli;
class kmaO_PorousElastic;
class kmaO_PorousMetalPlasticity;
class kmaO_Regularization;
class kmaO_Shear;
class kmaO_Solubility;
class kmaO_Sorption;
class kmaO_SpecificHeat;
class kmaO_Swelling;
class kmaO_UserDefinedField;
class kmaO_UserMaterial;
class kmaO_UserOutputVariables;
class kmaO_Viscoelastic;
class kmaO_Viscosity;
class kmaO_Viscous;
class kmaC_Material;

class kmaO_Material: public mem_AllocationOperators
{
  public:
    kmaO_Material( const kmaC_MaterialShortcut& shortcut );
    kmaO_Material();
    kmaO_Material( const kmaO_Material& copy );
    virtual kmaO_Material* Copy() const;
    kmaO_Material& operator=( const kmaO_Material& rhs );

    virtual ~kmaO_Material();
    kmaO_AcousticMedium acousticMedium() const;
    kmaO_BrittleCracking brittleCracking() const;
    kmaO_CapPlasticity capPlasticity() const;
    kmaO_CastIronPlasticity castIronPlasticity() const;
    kmaO_ClayPlasticity clayPlasticity() const;
    kmaO_Concrete concrete() const;
    kmaO_ConcreteDamagedPlasticity concreteDamagedPlasticity() const;
    kmaO_Conductivity conductivity() const;
    kmaO_Creep creep() const;
    kmaO_CrushableFoam crushableFoam() const;
    kmaO_DamageInitiation ductileDamageInitiation() const;
    kmaO_DamageInitiation fldDamageInitiation() const;
    kmaO_DamageInitiation flsdDamageInitiation() const;
    kmaO_DamageInitiation johnsonCookDamageInitiation() const;
    kmaO_DamageInitiation maxeDamageInitiation() const;
    kmaO_DamageInitiation maxsDamageInitiation() const;
    kmaO_DamageInitiation maxpeDamageInitiation() const;
    kmaO_DamageInitiation maxpsDamageInitiation() const;
    kmaO_DamageInitiation mkDamageInitiation() const;
    kmaO_DamageInitiation msfldDamageInitiation() const;
    kmaO_DamageInitiation quadeDamageInitiation() const;
    kmaO_DamageInitiation quadsDamageInitiation() const;
    kmaO_DamageInitiation shearDamageInitiation() const;
    kmaO_DamageInitiation hashinDamageInitiation() const;
    kmaO_Damping damping() const;
    kmaO_DeformationPlasticity deformationPlasticity() const;
    kmaO_Density density() const;
    kmaO_Depvar depvar() const;
    kmaO_Dielectric dielectric() const;
    kmaO_Diffusivity diffusivity() const;
    kmaO_DruckerPrager druckerPrager() const;
    kmaO_Elastic elastic() const;
    kmaO_ElectricalConductivity electricalConductivity() const;
    kmaO_Eos eos() const;
    kmaO_Expansion expansion() const;
    kmaO_FluidLeakoff fluidLeakoff() const;
    kmaO_GapFlow gapFlow() const;
    kmaO_GasketThicknessBehavior gasketThicknessBehavior() const;
    kmaO_GasketTransverseShearElastic gasketTransverseShearElastic() const;
    kmaO_GasketMembraneElastic gasketMembraneElastic() const;
    kmaO_Gel gel() const;
    kmaO_HeatGeneration heatGeneration() const;
    kmaO_Hyperelastic hyperelastic() const;
    kmaO_Hyperfoam hyperfoam() const;
    kmaO_Hypoelastic hypoelastic() const;
    kmaO_InelasticHeatFraction inelasticHeatFraction() const;
    kmaO_JouleHeatFraction jouleHeatFraction() const;
    kmaO_LatentHeat latentHeat() const;
    kmaO_LowDensityFoam lowDensityFoam() const;
    kmaO_MohrCoulombPlasticity mohrCoulombPlasticity() const;
    kmaO_MoistureSwelling moistureSwelling() const;
    kmaO_MullinsEffect mullinsEffect() const;
    kmaO_Permeability permeability() const;
    kmaO_Piezoelectric piezoelectric() const;
    kmaO_Plastic plastic() const;
    kmaO_PoreFluidExpansion poreFluidExpansion() const;
    kmaO_PorousBulkModuli porousBulkModuli() const;
    kmaO_PorousElastic porousElastic() const;
    kmaO_PorousMetalPlasticity porousMetalPlasticity() const;
    kmaO_Regularization regularization() const;
    kmaO_Shear shear() const;
    kmaO_Solubility solubility() const;
    kmaO_Sorption sorption() const;
    kmaO_SpecificHeat specificHeat() const;
    kmaO_Swelling swelling() const;
    kmaO_UserDefinedField userDefinedField() const;
    kmaO_UserMaterial userMaterial() const;
    kmaO_UserOutputVariables userOutputVariables() const;
    kmaO_Viscoelastic viscoelastic() const;
    kmaO_Viscosity viscosity() const;
    kmaO_Viscous viscous() const;
    odb_String description() const;
    odb_String materialIdentifier() const;
    kmaO_AcousticMedium AcousticMedium( bool acousticVolumetricDrag = false,
                                        bool temperatureDependencyB = false,
                                        bool temperatureDependencyV = false,
                                        int dependenciesB = 0,
                                        int dependenciesV = 0,
                                        const odb_SequenceSequenceDouble& bulkTable = odb_SequenceSequenceDouble(),
                                        const odb_SequenceSequenceDouble& volumetricTable = odb_SequenceSequenceDouble() );
    kmaO_BrittleCracking BrittleCracking( const odb_SequenceSequenceDouble& table,
                                          bool temperatureDependency = false,
                                          int dependencies = 0,
                                          odb_String type = "STRAIN" );
    kmaO_CapPlasticity CapPlasticity( const odb_SequenceSequenceDouble& table,
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_CastIronPlasticity CastIronPlasticity( const odb_SequenceSequenceDouble& table,
                                                bool temperatureDependency = false,
                                                int dependencies = 0 );
    kmaO_ClayPlasticity ClayPlasticity( const odb_SequenceSequenceDouble& table,
                                        odb_Union intercept = "NONE",
                                        odb_String hardening = "EXPONENTIAL",
                                        bool temperatureDependency = false,
                                        int dependencies = 0 );
    kmaO_Concrete Concrete( const odb_SequenceSequenceDouble& table,
                            bool temperatureDependency = false,
                            int dependencies = 0 );
    kmaO_ConcreteDamagedPlasticity ConcreteDamagedPlasticity( const odb_SequenceSequenceDouble& table,
                                                              bool temperatureDependency = false,
                                                              int dependencies = 0 );
    kmaO_Conductivity Conductivity( const odb_SequenceSequenceDouble& table,
                                    odb_String type = "ISOTROPIC",
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    kmaO_Creep Creep( const odb_SequenceSequenceDouble& table,
                      odb_String law = "STRAIN",
                      bool temperatureDependency = false,
                      int dependencies = 0 );
    kmaO_CrushableFoam CrushableFoam( const odb_SequenceSequenceDouble& table,
                                      odb_String hardening = "VOLUMETRIC",
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_DamageInitiation DuctileDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation FldDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation FlsdDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation JohnsonCookDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation MaxeDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation MaxsDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation MaxpeDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation MaxpsDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation MkDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation MsfldDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation QuadeDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation QuadsDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation ShearDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_DamageInitiation HashinDamageInitiation( const odb_SequenceSequenceDouble& table,
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
    kmaO_Damping Damping( double alpha = 0,
                          double beta = 0,
                          double composite = 0,
                          double structural = 0 );
    kmaO_DeformationPlasticity DeformationPlasticity( const odb_SequenceSequenceDouble& table,
                                                      bool temperatureDependency = false );
    kmaO_Density Density( const odb_SequenceSequenceDouble& table,
                          bool temperatureDependency = false,
                          int dependencies = 0,
                          odb_String distributionType = "UNIFORM",
                          odb_String fieldName = "" );
    kmaO_Depvar Depvar( int deleteVar = 0,
                        int n = 0 );
    kmaO_Dielectric Dielectric( const odb_SequenceSequenceDouble& table,
                                odb_String type = "ISOTROPIC",
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_Diffusivity Diffusivity( const odb_SequenceSequenceDouble& table,
                                  odb_String type = "ISOTROPIC",
                                  odb_String law = "GENERAL",
                                  bool temperatureDependency = false,
                                  int dependencies = 0 );
    kmaO_DruckerPrager DruckerPrager( const odb_SequenceSequenceDouble& table,
                                      odb_String shearCriterion = "LINEAR",
                                      double eccentricity = 0.1,
                                      bool testData = false,
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_Elastic Elastic( const odb_SequenceSequenceDouble& table,
                          odb_String type = "ISOTROPIC",
                          bool noCompression = false,
                          bool noTension = false,
                          bool temperatureDependency = false,
                          int dependencies = 0,
                          odb_String moduli = "LONG_TERM" );
    kmaO_ElectricalConductivity ElectricalConductivity( const odb_SequenceSequenceDouble& table,
                                                        odb_String type = "ISOTROPIC",
                                                        bool temperatureDependency = false,
                                                        int dependencies = 0 );
    kmaO_Eos Eos( odb_String type = "IDEALGAS",
                  bool temperatureDependency = false,
                  int dependencies = 0,
                  double detonationEnergy = 0.0,
                  const odb_SequenceSequenceDouble& solidTable = odb_SequenceSequenceDouble(),
                  const odb_SequenceSequenceDouble& gasTable = odb_SequenceSequenceDouble(),
                  const odb_SequenceSequenceDouble& reactionTable = odb_SequenceSequenceDouble(),
                  const odb_SequenceSequenceDouble& gasSpecificTable = odb_SequenceSequenceDouble(),
                  const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_Expansion Expansion( odb_String type = "ISOTROPIC",
                              bool userSubroutine = false,
                              double zero = 0,
                              bool temperatureDependency = false,
                              int dependencies = 0,
                              const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_FluidLeakoff FluidLeakoff( bool temperatureDependency = false,
                                    int dependencies = 0,
                                    odb_String type = "COEFFICIENTS",
                                    const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_GapFlow GapFlow( const odb_SequenceSequenceDouble& table,
                          odb_Union kmax = "NONE",
                          bool temperatureDependency = false,
                          int dependencies = 0,
                          odb_String type = "NEWTONIAN" );
    kmaO_GasketThicknessBehavior GasketThicknessBehavior( const odb_SequenceSequenceDouble& table,
                                                          bool temperatureDependency = false,
                                                          int dependencies = 0,
                                                          double tensileStiffnessFactor = 0.0010,
                                                          odb_String type = "ELASTIC_PLASTIC",
                                                          int unloadingDependencies = 0,
                                                          bool unloadingTemperatureDependency = false,
                                                          odb_String variableUnits = "STRESS",
                                                          double yieldOnset = 0.1,
                                                          odb_String yieldOnsetMethod = "RELATIVE_SLOPE_DROP",
                                                          const odb_SequenceSequenceDouble& unloadingTable = odb_SequenceSequenceDouble() );
    kmaO_GasketTransverseShearElastic GasketTransverseShearElastic( const odb_SequenceSequenceDouble& table,
                                                                    odb_String variableUnits = "STRESS",
                                                                    bool temperatureDependency = false,
                                                                    int dependencies = 0 );
    kmaO_GasketMembraneElastic GasketMembraneElastic( const odb_SequenceSequenceDouble& table,
                                                      bool temperatureDependency = false,
                                                      int dependencies = 0 );
    kmaO_Gel Gel( const odb_SequenceSequenceDouble& table );
    kmaO_HeatGeneration HeatGeneration(  );
    kmaO_Hyperelastic Hyperelastic( const odb_SequenceSequenceDouble& table,
                                    odb_String type = "UNKNOWN",
                                    odb_String moduliTimeScale = "LONG_TERM",
                                    bool temperatureDependency = false,
                                    int n = 1,
                                    odb_Union beta = "FITTED_VALUE",
                                    bool testData = true,
                                    bool compressible = false,
                                    int properties = 0,
                                    odb_String deviatoricResponse = "UNIAXIAL",
                                    odb_String volumetricResponse = "DEFAULT",
                                    double poissonRatio = 0,
                                    odb_String materialType = "ISOTROPIC",
                                    odb_String anisotropicType = "FUNG_ANISOTROPIC",
                                    odb_String formulation = "STRAIN",
                                    odb_String behaviorType = "INCOMPRESSIBLE",
                                    int dependencies = 0,
                                    int localDirections = 0 );
    kmaO_Hyperfoam Hyperfoam( bool testData = false,
                              odb_Union poisson = "NONE",
                              int n = 1,
                              bool temperatureDependency = false,
                              odb_String moduli = "LONG_TERM",
                              const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble() );
    kmaO_Hypoelastic Hypoelastic( const odb_SequenceSequenceDouble& table,
                                  bool user = false );
    kmaO_InelasticHeatFraction InelasticHeatFraction( double fraction = 0.9 );
    kmaO_JouleHeatFraction JouleHeatFraction( double fraction = 1 );
    kmaO_LatentHeat LatentHeat( const odb_SequenceSequenceDouble& table );
    kmaO_LowDensityFoam LowDensityFoam( bool elementRemoval = false,
                                        odb_Union maxAllowablePrincipalStress = "NONE",
                                        bool extrapolateStressStrainCurve = false,
                                        odb_String strainRateType = "VOLUMETRIC",
                                        double mu0 = 0.0001,
                                        double mu1 = 0.005,
                                        double alpha = 2.0 );
    kmaO_MohrCoulombPlasticity MohrCoulombPlasticity( const odb_SequenceSequenceDouble& table,
                                                      odb_Union deviatoricEccentricity = "NONE",
                                                      double meridionalEccentricity = 0.1,
                                                      bool temperatureDependency = false,
                                                      int dependencies = 0,
                                                      bool useTensionCutoff = false );
    kmaO_MoistureSwelling MoistureSwelling( const odb_SequenceSequenceDouble& table );
    kmaO_MullinsEffect MullinsEffect( odb_String definition = "CONSTANTS",
                                      bool temperatureDependency = false,
                                      int dependencies = 0,
                                      int properties = 0,
                                      const odb_SequenceSequenceDouble& table = odb_SequenceSequenceDouble(),
                                      const kmaO_SequenceUniaxialTestData& uniaxialTests = kmaO_SequenceUniaxialTestData(),
                                      const kmaO_SequenceBiaxialTestData& biaxialTests = kmaO_SequenceBiaxialTestData(),
                                      const kmaO_SequencePlanarTestData& planarTests = kmaO_SequencePlanarTestData() );
    kmaO_Permeability Permeability( double specificWeight,
                                    const odb_SequenceSequenceDouble& table,
                                    odb_String type = "ISOTROPIC",
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    kmaO_Piezoelectric Piezoelectric( const odb_SequenceSequenceDouble& table,
                                      odb_String type = "STRESS",
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_Plastic Plastic( const odb_SequenceSequenceDouble& table,
                          odb_String hardening = "ISOTROPIC",
                          bool rate = false,
                          odb_String dataType = "HALF_CYCLE",
                          bool strainRangeDependency = false,
                          int numBackstresses = 1,
                          bool temperatureDependency = false,
                          int dependencies = 0 );
    kmaO_PoreFluidExpansion PoreFluidExpansion( const odb_SequenceSequenceDouble& table,
                                                double zero = 0,
                                                bool temperatureDependency = false,
                                                int dependencies = 0 );
    kmaO_PorousBulkModuli PorousBulkModuli( const odb_SequenceSequenceDouble& table,
                                            bool temperatureDependency = false );
    kmaO_PorousElastic PorousElastic( const odb_SequenceSequenceDouble& table,
                                      odb_String shear = "POISSON",
                                      bool temperatureDependency = false,
                                      int dependencies = 0 );
    kmaO_PorousMetalPlasticity PorousMetalPlasticity( const odb_SequenceSequenceDouble& table,
                                                      odb_Union relativeDensity = "NONE",
                                                      bool temperatureDependency = false,
                                                      int dependencies = 0 );
    kmaO_Regularization Regularization( double rtol = 0.03,
                                        odb_String strainRateRegularization = "LOGARITHMIC" );
    kmaO_Shear Shear( const odb_SequenceSequenceDouble& table,
                      bool temperatureDependency = false,
                      int dependencies = 0 );
    kmaO_Solubility Solubility( const odb_SequenceSequenceDouble& table,
                                bool temperatureDependency = false,
                                int dependencies = 0 );
    kmaO_Sorption Sorption( const odb_SequenceSequenceDouble& absorptionTable,
                            odb_String lawAbsorption = "TABULAR",
                            bool exsorption = false,
                            odb_String lawExsorption = "TABULAR",
                            double scanning = 0,
                            const odb_SequenceSequenceDouble& exsorptionTable = odb_SequenceSequenceDouble() );
    kmaO_SpecificHeat SpecificHeat( const odb_SequenceSequenceDouble& table,
                                    odb_String law = "CONSTANTVOLUME",
                                    bool temperatureDependency = false,
                                    int dependencies = 0 );
    kmaO_Swelling Swelling( const odb_SequenceSequenceDouble& table,
                            odb_String law = "INPUT",
                            bool temperatureDependency = false,
                            int dependencies = 0 );
    kmaO_UserDefinedField UserDefinedField(  );
    kmaO_UserMaterial UserMaterial( odb_String type = "MECHANICAL",
                                    bool unsymm = false,
                                    const odb_SequenceDouble& mechanicalConstants = odb_SequenceDouble(),
                                    const odb_SequenceDouble& thermalConstants = odb_SequenceDouble() );
    kmaO_UserOutputVariables UserOutputVariables( int n = 0 );
    kmaO_Viscoelastic Viscoelastic( odb_String domain,
                                    const odb_SequenceSequenceDouble& table,
                                    odb_String frequency = "FORMULA",
                                    odb_String type = "ISOTROPIC",
                                    odb_String preload = "NONE",
                                    odb_String time = "PRONY",
                                    double errtol = 0.01,
                                    int nmax = 13,
                                    const odb_SequenceSequenceDouble& volumetricTable = odb_SequenceSequenceDouble() );
    kmaO_Viscosity Viscosity( const odb_SequenceSequenceDouble& table,
                              odb_String type = "NEWTONIAN",
                              bool temperatureDependency = false,
                              int dependencies = 0 );
    kmaO_Viscous Viscous( const odb_SequenceSequenceDouble& table,
                          odb_String law = "STRAIN",
                          bool temperatureDependency = false,
                          int dependencies = 0 );
    kmaC_MaterialShortcut Shortcut() const { return shortcut; } 

    bool hasValue() const { return shortcut.HasValue(); }

    odb_String name() const { if (hasValue()) return shortcut.Name();
      else return odb_String(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    kmaC_MaterialShortcut shortcut;
};

COW_ARRAYCOW2_DECL(kmaO_Material, cow_Virtual);

#endif // kmaO_Material_h
