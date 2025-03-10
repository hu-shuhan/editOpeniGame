//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_ApiExtension_h
#define kseO_ApiExtension_h

// Begin local includes
#include <odb_ApiExtension.h>
#include <kseO_SectionContainer.h>
#include <kbpO_ProfileContainer.h>
#include <kseO_SequenceSectionLayer.h>
#include <kbeO_SequenceBehaviorOption.h>
#include <odb_Union.h>

// Forward declarations
class kbpO_ArbitraryProfile;
class kbpO_BoxProfile;
class kbpO_CircularProfile;
class kbpO_GeneralizedProfile;
class kbpO_HexagonalProfile;
class kbpO_IProfile;
class kbpO_LProfile;
class kbpO_PipeProfile;
class kbpO_RectangularProfile;
class kbpO_TProfile;
class kbpO_TrapezoidalProfile;
class kbpO_CProfile;
class kbpO_HatProfile;
class kseO_AcousticInfiniteSection;
class kseO_AcousticInterfaceSection;
class kseO_BeamSection;
class kseO_CohesiveSection;
class kseO_CompositeShellSection;
class kseO_CompositeSolidSection;
class kseO_ConnectorSection;
class kseO_EulerianSection;
class kseO_GasketSection;
class kseO_GeneralStiffnessSection;
class kseO_GeometryShellSection;
class kseO_HomogeneousFluidSection;
class kseO_HomogeneousShellSection;
class kseO_HomogeneousSolidSection;
class kseO_MembraneSection;
class kseO_PEGSection;
class kseO_PointSection;
class kseO_SurfaceSection;
class kseO_TrussSection;
class kseO_DiscreteSection;
class kseO_FluidPipeSection;

class kseO_ApiExtension : public odb_ApiExtension
{
public:
    kseO_ApiExtension();
    virtual ~kseO_ApiExtension();

    kbpO_ArbitraryProfile& ArbitraryProfile(const odb_String& name,
                                            const odb_SequenceSequenceDouble& table);

    kbpO_BoxProfile& BoxProfile(const odb_String& name,
                                double a,
                                double b,
                                bool uniformThickness,
                                double t1,
                                double t2=0,
                                double t3=0,
                                double t4=0);

    kbpO_CircularProfile& CircularProfile(const odb_String& name,
                                          double r);

    kbpO_GeneralizedProfile& GeneralizedProfile(const odb_String& name,
                                                double area,
                                                double i11,
                                                double i12,
                                                double i22,
                                                double j,
                                                double gammaO,
                                                double gammaW);

    kbpO_HexagonalProfile& HexagonalProfile(const odb_String& name,
                                            double r,
                                            double t);

    kbpO_IProfile& IProfile(const odb_String& name,
                            double l,
                            double h,
                            double b1,
                            double b2,
                            double t1,
                            double t2,
                            double t3);

    kbpO_LProfile& LProfile(const odb_String& name,
                            double a,
                            double b,
                            double t1,
                            double t2);

    kbpO_PipeProfile& PipeProfile(const odb_String& name,
                                  double r,
                                  double t);

    kbpO_RectangularProfile& RectangularProfile(const odb_String& name,
                                                double a,
                                                double b);

    kbpO_TProfile& TProfile(const odb_String& name,
                            double b,
                            double h,
                            double l,
                            double tf,
                            double tw);

    kbpO_TrapezoidalProfile& TrapezoidalProfile(const odb_String& name,
                                                double a,
                                                double b,
                                                double c,
                                                double d);

    kbpO_CProfile& CProfile(const odb_String& name,
        double l,
        double h,
        double b1,
        double b2,
        double t1,
        double t2,
        double t3,
        double o);
    
    kbpO_HatProfile& HatProfile(const odb_String& name,
        double l,
        double h,
        double b,
        double b1,
        double b2,
        double t1,
        double t2,
        double t3);

    kseO_AcousticInfiniteSection& AcousticInfiniteSection(const odb_String& name,
                                                          const odb_String& material,
                                                          double thickness=1,
                                                          int order=10);

    kseO_AcousticInterfaceSection& AcousticInterfaceSection(const odb_String& name,
                                                            double thickness=1);

    kseO_BeamSection& BeamSection(const odb_String& name,
                                  const odb_String& integration,
                                  const odb_String& profile,
                                  double poissonRatio=0,
                                  bool thermalExpansion=false,
                                  bool temperatureDependency=false,
                                  int dependencies=0,
                                  odb_Union density="NONE",
                                  odb_Union referenceTemperature="NONE",
                                  odb_String temperatureVar="LINEAR",
                                  double alphaDamping=0,
                                  double betaDamping=0,
                                  double compositeDamping=0,
                                  bool useFluidInertia=false,
                                  odb_String submerged="FULLY",
                                  odb_Union fluidMassDensity="NONE",
                                  odb_Union crossSectionRadius="NONE",
                                  double lateralMassCoef=1,
                                  double axialMassCoef=0,
                                  double massOffsetX=0,
                                  double massOffsetY=0,
                                  odb_String material="",
                                  const odb_SequenceSequenceDouble& table=odb_SequenceSequenceDouble(),
                                  const odb_SequenceSequenceDouble& outputPts=odb_SequenceSequenceDouble(),
                                  odb_SequenceDouble centroid=odb_SequenceDouble(),
                                  odb_SequenceDouble shearCenter=odb_SequenceDouble());

    kseO_CohesiveSection& CohesiveSection(const odb_String& name,
                                          const odb_String& response,
                                          const odb_String& material,
                                          odb_String initialThicknessType="SOLVER_DEFAULT",
                                          double initialThickness=1,
                                          odb_Union outOfPlaneThickness="NONE");

    kseO_CompositeShellSection& CompositeShellSection(const odb_String& name,
                                                      const kseO_SequenceSectionLayer& layup,
                                                      bool symmetric=false,
                                                      odb_String layupName="",
                                                      odb_String thicknessType="UNIFORM",
                                                      bool preIntegrate=false,
                                                      odb_String poissonDefinition="DEFAULT",
                                                      double poisson=0.5,
                                                      odb_String integrationRule="SIMPSON",
                                                      odb_String temperature="GRADIENT",
                                                      odb_String simplification="NO_IDEALIZATION",
                                                      odb_Union nTemp="NONE",
                                                      odb_Union thicknessModulus="NONE",
                                                      bool useDensity=false,
                                                      double density=0.0,
                                                      odb_String thicknessField="",
                                                      odb_String nodalThicknessField="");

    kseO_CompositeSolidSection& CompositeSolidSection(const odb_String& name,
                                                      const kseO_SequenceSectionLayer& layup,
                                                      bool symmetric=false,
                                                      odb_String layupName="");

    kseO_ConnectorSection& ConnectorSection(const odb_String& name,
                                            odb_String assembledType="NONE",
                                            odb_String rotationalType="NONE",
                                            odb_String translationalType="NONE",
                                            odb_String integration="UNSPECIFIED",
                                            odb_Union u1ReferenceLength="NONE",
                                            odb_Union u2ReferenceLength="NONE",
                                            odb_Union u3ReferenceLength="NONE",
                                            odb_Union ur1ReferenceAngle="NONE",
                                            odb_Union ur2ReferenceAngle="NONE",
                                            odb_Union ur3ReferenceAngle="NONE",
                                            odb_Union massPerLength="NONE",
                                            odb_Union contactAngle="NONE",
                                            double materialFlowFactor=1.0,
                                            bool regularize=true,
                                            bool defaultTolerance=true,
                                            double regularization=0.03,
                                            odb_String extrapolation="CONSTANT",
                                            kbeO_SequenceConnectorBehaviorOption behaviorOptions=kbeO_SequenceConnectorBehaviorOption());

    kseO_EulerianSection& EulerianSection(const odb_String& name,
                                          const odb_String& defaultMaterial,
                                          const odb_SequenceString& materialMapKeys,
                                          const odb_SequenceString& materialMapValues);

    kseO_GasketSection& GasketSection(const odb_String& name,
                                      const odb_String& material,
                                      double crossSection=1,
                                      double initialGap=0,
                                      odb_Union initialThickness="DEFAULT",
                                      double initialVoid=0,
                                      odb_Union stabilizationStiffness="DEFAULT");

    kseO_GeneralStiffnessSection& GeneralStiffnessSection(const odb_String& name,
                                                          const odb_SequenceDouble& stiffnessMatrix,
                                                          const odb_SequenceDouble& thermalStresses,
                                                          odb_Union referenceTemperature="NONE",
                                                          bool applyThermalStress=false,
                                                          bool temperatureDependency=false,
                                                          int dependencies=0,
                                                          const odb_SequenceSequenceDouble& scalingData=odb_SequenceSequenceDouble(),
                                                          odb_String thicknessType="UNIFORM",
                                                          bool preIntegrate=false,
                                                          odb_String poissonDefinition="DEFAULT",
                                                          double poisson=0.5,
                                                          odb_String integrationRule="SIMPSON",
                                                          odb_String temperature="GRADIENT",
                                                          odb_Union nTemp="NONE",
                                                          odb_Union thicknessModulus="NONE",
                                                          bool useDensity=false,
                                                          double density=0.0,
                                                          odb_String thicknessField="",
                                                          odb_String nodalThicknessField="");

    kseO_GeometryShellSection& GeometryShellSection(const odb_String& name,
                                                    odb_String thicknessType="UNIFORM",
                                                    bool preIntegrate=false,
                                                    odb_String poissonDefinition="DEFAULT",
                                                    double poisson=0.5,
                                                    odb_String integrationRule="SIMPSON",
                                                    odb_String temperature="GRADIENT",
                                                    odb_String simplification="NO_IDEALIZATION",
                                                    odb_Union nTemp="NONE",
                                                    odb_Union thicknessModulus="NONE",
                                                    bool useDensity=false,
                                                    double density=0.0,
                                                    odb_String thicknessField="",
                                                    odb_String nodalThicknessField="");

    kseO_HomogeneousFluidSection& HomogeneousFluidSection(const odb_String& name,
                                                          const odb_String& material);

    kseO_HomogeneousShellSection& HomogeneousShellSection(const odb_String& name,
                                                          double thickness,
                                                          const odb_String& material,
                                                          int numIntPts=5,
                                                          odb_String thicknessType="UNIFORM",
                                                          bool preIntegrate=false,
                                                          odb_String poissonDefinition="DEFAULT",
                                                          double poisson=0.5,
                                                          odb_String integrationRule="SIMPSON",
                                                          odb_String temperature="GRADIENT",
                                                          odb_String simplification="NO_IDEALIZATION",
                                                          odb_Union nTemp="NONE",
                                                          odb_Union thicknessModulus="NONE",
                                                          bool useDensity=false,
                                                          double density=0.0,
                                                          odb_String thicknessField="",
                                                          odb_String nodalThicknessField="");

    kseO_HomogeneousSolidSection& HomogeneousSolidSection(const odb_String& name,
                                                          const odb_String& material,
                                                          double thickness=1);

    kseO_MembraneSection& MembraneSection(const odb_String& name,
                                          const odb_String& material,
                                          double thickness=1,
                                          odb_String poissonDefinition="DEFAULT",
                                          double poisson=0.5);

    kseO_PEGSection& PEGSection(const odb_String& name,
                                const odb_String& material,
                                double thickness=1,
                                double wedgeAngle1=0,
                                double wedgeAngle2=0);

    kseO_PointSection& PointSection(const odb_String& name,
                                    double mass=0.0,
                                    double i11=0.0,
                                    double i12=0.0,
                                    double i13=0.0,
                                    double i22=0.0,
                                    double i23=0.0,
                                    double i33=0.0,
                                    double massAlphaDamping=0.0,
                                    double massCompositeDamping=0.0,
                                    double rotaryAlphaDamping=0.0,
                                    double rotaryCompositeDamping=0.0,
                                    double heatCapacitance=0.0);
    kseO_DiscreteSection& DiscreteSection(const odb_String& name,
                                          double density = 0.0,
                                          const odb_String& densityField = "",
                                          double radius = 0.0,
                                          const odb_String& radiusField = "");

    kseO_SurfaceSection& SurfaceSection(const odb_String& name,
                                        bool useDensity=false,
                                        double density=0.0);

    kseO_TrussSection& TrussSection(const odb_String& name,
                                    const odb_String& material,
                                    double area=1);

    kseO_FluidPipeSection& FluidPipeSection(const odb_String& name,
                                           const odb_String& material,
                                           double innerRadius = DBL_MAX,
                                           double outerRadius = DBL_MAX,                                           
                                           double laminarFlowTransition = 0.0,
                                           odb_String type="BLASIUS",
                                           const odb_SequenceDouble& table = odb_SequenceDouble());


    const odb_Section& section( const odb_String& name ) const;
    kseO_SectionContainer& sections();
    int numSections() const;

    const kbpO_Profile& profile( const odb_String& name ) const;
    kbpO_ProfileContainer& profiles();
    int numProfiles() const;

    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;

// Undocumented and unsupported
    void ConstructContainer();

private:
    kseO_SectionContainer sectionCont;
    kbpO_ProfileContainer profileCont;
};

#endif
