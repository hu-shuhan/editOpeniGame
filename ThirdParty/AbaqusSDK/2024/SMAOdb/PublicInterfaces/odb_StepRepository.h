//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_StepRepository_h
#define odb_StepRepository_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Step.h>
#include <odb_Union.h>
#include <odb_MassScaling.h>
#include <cow_List.h>
#include <cow_COW.h>

// End local includes

COW_LIST_DECL( odb_Step, odb_StepList );

// Forward declarations
class ddb_Ddb;
COW_COW_DECL(ddb_Ddb);
class odb_Odb;
COW_COW_DECL(odb_Odb);
class kprC_AnalysisStep;


class odb_StepRepository : public mem_AllocationOperators
{
    friend class odb_StepRepositoryIT;
    friend class odb_Odb;
public:
    int size() const;
    const odb_Step& constGet(const odb_String& name) const;
    odb_Step& get(const odb_String& name);
    odb_Step& operator[](const odb_String& name);
    bool isMember(const odb_String& name) const;
    void release(const odb_String& name) const;
    void release() const;


    // Constructors

    odb_Step& StaticStep(const odb_String& stepName,
                         odb_String previousStepName="",
                         odb_String description="",
                         double timePeriod=1.0,
                         odb_String nlgeom="OFF",
                         int maxNumInc=100,
                         double initialInc=1.0,
                         double minInc=1.e-5,
                         double maxInc=1.0,
                         odb_String stabilizationMethod="NONE",
                         double stabilizationMagnitude=0.0002,
                         bool adiabatic=false,
                         odb_String timeIncrementationMethod="AUTOMATIC",
                         odb_String amplitude = "RAMP");

    odb_Step& StaticLinearPerturbationStep( const odb_String& stepName,
                                            odb_String previousStepName="",
                                            odb_String description="",
                                            odb_String matrixSolver="SOLVER_DEFAULT");

    odb_Step& HeatTransferStep(const odb_String& stepName,
                               odb_String previousStepName="",
                               odb_String description="",
                               odb_String response="TRANSIENT",
                               odb_String amplitude="STEP",
                               odb_String timeIncrementationMethod="AUTOMATIC",
                               double timePeriod=1.0,
                               int maxNumInc=100,
                               double initialInc=1.0,
                               double minInc=1.e-5,
                               double maxInc=1.0,
                               double end=DBL_MAX,
                               double deltmx=0.0,
                               odb_String extrapolation="LINEAR",
                               odb_String matrixSolver="SOLVER_DEFAULT");

    odb_Step& StaticRiksStep(const odb_String& stepName,
                             odb_String previousStepName="",
                             odb_String description="",
                             odb_String nlgeom="OFF",
                             bool adiabatic=false,
                             double maxLPF=DBL_MAX,
                             bool nodeOn=false,
                             double maximumDisplacement=0.0,
                             int monitorDof=0,
                             const odb_Set& region=odb_Set(),
                             odb_String timeIncrementationMethod="AUTOMATIC",
                             int maxNumInc=100,
                             double totalArcLength=1.0,
                             double initialArcInc=1.0,
                             double minArcInc=1.e-5,
                             double maxArcInc=DBL_MAX,
                             odb_String matrixSolver="SOLVER_DEFAULT",
                             odb_String extrapolation="LINEAR",
                             odb_String fullyPlastic="",
                             bool noStop=false,
                             bool useLongTermSolution=false);

    odb_Step& ExplicitDynamicsStep(const odb_String& stepName,
                                   odb_String previousStepName="",
                                   odb_String description="",
                                   double timePeriod=1.0,
                                   odb_String nlgeom="ON",
                                   bool adiabatic=false,
                                   odb_String timeIncrementationMethod="AUTOMATIC_GLOBAL",
                                   odb_Union maxIncrement="NONE",
                                   double scaleFactor=1.0,
                                   double linearBulkViscosity=0.06,
                                   double quadBulkViscosity=1.2,
                                   odb_Union userDefinedInc="NONE",
                                   odb_MassScalingList msList = odb_MassScalingList());

  odb_Step& FrequencyStep(const odb_String& stepName,
                          odb_String previousStepName="",
                          odb_String description="",
                          int numEigen=INT_MAX,
                          double maxEigen=DBL_MAX,
                          double shift=DBL_MAX,
                          int vectors=0,
                          int maxIterations=30,
                          odb_String eigensolver="LANCZOS",
                          double minEigen=DBL_MAX,
                          int blockSize=INT_MAX,
                          int maxBlocks=INT_MAX,
                          double timePeriod=0.0,
                          odb_String normalization="DISPLACEMENT",
                          double propertyEvaluationFrequency=DBL_MAX,
                          odb_String acousticCoupling="AC_ON",
                          int numberInterval=1,
                          double intervalBias=1.0,
                          bool residualModes=false,
                          double substructureCutoffMultiplier=5.0,
                          double firstCutoffMultiplier=1.6,
                          double secondCutoffMultiplier=1.3,
                          double acousticRangeFactor=1.0,
                          const odb_SequenceString& residualModeRegion=odb_SequenceString(),
                          const odb_SequenceInt& residualModeDof=odb_SequenceInt(),
                          bool simLinearDynamics = false);


/*
This method creates a SteadyStateModalStep object

Arguments 
	name:
	frequencyRange:Sequence of(lower, upper, nPoints(integer), bias)
	previousStepName:
	description:
	scale: Possible values are LOGARITHMIC and LINEAR. 
	directDamping: Sequence of(start(integer), end(integer), fraction)
	compositeDamping:Sequence of(Start(integer), end(integer))
	rayleighDamping:Sequence of(start(integer), end(integer), alpha, beta)
	structuralDamping:Sequence of(start(integer), end(integer), factor)
	directDampingByFrequency:Sequence of(frequency, fraction)
	rayleighDampingByFrequency:Sequence of(frequency, alpha, beta)
	structuralDampingByFrequency:Sequence of(frequency, factor)
	subdivideUsingEigenfrequencies
*/

odb_Step& SteadyStateModalStep(const odb_String& stepName,
                               const odb_SequenceSequenceFloat& frequencyRange,
                               odb_String previousStepName="",
                               odb_String description="",
                               odb_String scale="LOGARITHMIC",
                               const odb_SequenceSequenceFloat& directDamping = odb_SequenceSequenceFloat(),
                               const odb_SequenceSequenceFloat& compositeDamping = odb_SequenceSequenceFloat(),
                               const odb_SequenceSequenceFloat& rayleighDamping = odb_SequenceSequenceFloat(),
                               const odb_SequenceSequenceFloat& structuralDamping = odb_SequenceSequenceFloat(),
                               const odb_SequenceSequenceFloat& directDampingByFrequency = odb_SequenceSequenceFloat(),
                               const odb_SequenceSequenceFloat& rayleighDampingByFrequency = odb_SequenceSequenceFloat(),
                               const odb_SequenceSequenceFloat& structuralDampingByFrequency = odb_SequenceSequenceFloat(),
                               bool subdivideUsingEigenfrequencies = true);

/*
This method creates a SteadyStateSubspaceStep object
Arguments 
	name:
	frequencyRange:Sequence of(lower, upper, nPoints(integer), bias)
	previousStepName:
	description:
	factorization:Possible values are REAL_ONLY and COMPLEX.
	scale:Possible values are LOGARITHMIC and LINEAR. 
	matrixStorage:Possible values are SYMMETRIC, UNSYMMETRIC, and SOLVER_DEFAULT. 
	subdivideUsingEigenfrequencies:
	projection:Possible values are ALL_FREQUENCIES, CONSTANT, EIGENFREQUENCY, PROPERTY_CHANGE, and RANGE. 
	maxDampingChange:
	maxStiffnessChange:
	frictionDamping:
*/

odb_Step& SteadyStateSubspaceStep(const odb_String& stepName,
                                  const odb_SequenceSequenceFloat& frequencyRange,
                                  odb_String previousStepName="",
                                  odb_String description="",
                                  odb_String factorization="COMPLEX",
                                  odb_String scale="LOGARITHMIC",
                                  odb_String matrixStorage="SOLVER_DEFAULT",
                                  bool subdivideUsingEigenfrequencies = true,
                                  odb_String projection="ALL_FREQUENCIES", 
                                  double maxDampingChange=0.1,
                                  double maxStiffnessChange=0.1,
                                  bool frictionDamping=false);



private:
    odb_StepRepository();
    //   void setInitialStep();
    void insert(const odb_String& stepName,
                const odb_Step& );
    void insertAfter(const odb_String& stepName,
                     const odb_String& prevStepName,
                     const odb_Step& s);

    odb_StepList       List;
    odb_SequenceString StepNames;

    ddb_DdbCOW m_ddb;
    void CreateOdbStep(const odb_String& stepName,
                       const odb_String& previousStepName,
                       const odb_String& description,
                       kprC_AnalysisStep& stepEO);

    bool IsValueInteger(float valueFloat,
                        const odb_String& variableName,
                        int rindex,
                        int cindex);

public: //undocumented and unsupported
    odb_StepRepository(const ddb_Ddb& ddb);
    void setInitialStep();
    const odb_StepList& stepList() const;
    const odb_SequenceString& stepNames() const;

};

class odb_StepRepositoryIT : public mem_AllocationOperators
{
public:
    odb_StepRepositoryIT (const odb_StepRepository& );

    void first();
    void next();
    bool isDone();
    const odb_Step& currentValue();
    const odb_String& currentKey();

private:
    // odb_StepRepositoryIT()
    int OffSet;
    const odb_StepRepository& StepCon;
};

COW_COW_DECL(odb_StepRepository);

#endif // #ifndef odb_StepRepository_h

