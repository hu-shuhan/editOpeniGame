//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kprC_Step_h
#define kprC_Step_h

// Begin local includes
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <cow_ArrayCOW.h>
#include <cow_String.h>
#include <mem_AllocationOperators.h>
#include <omu_PrimEnum.h>
#include <typ_typeTag.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Base class step

class kprC_Step  : public mem_AllocationOperators
{
  public:

    kprC_Step();
    virtual ~kprC_Step();

    // Enums
    enum ResponseEnm { STEADY_STATE, TRANSIENT, NOT_APPLICABLE };
    enum NlgeomEnm { OFF, ON, NOT_AVAILABLE, ALWAYS_OFF };
    enum AmplitudeEnm { STEP, RAMP };
    enum ExtrapolationEnm { LINEAR, PARABOLIC, NONE, VELOCITY_PARABOLIC, EXTRAPOLATION_DEFAULT };
    enum DefaultEnm { DEFAULT };
    enum EigensolverEnm { SUBSPACE, LANCZOS, AMS };
    enum SolutionTechniqueEnm { FULL_NEWTON, SEPARATED, QUASI_NEWTON, LCP_CONTACT };
    enum TimeIncrementationMethodEnm { AUTOMATIC, FIXED };
    enum MatrixSolverEnm { DIRECT, ITERATIVE,
                           DDM_ITERATIVE_DEPR,      // obsolete value from 6.9-1
                           DIRECT_SYMMETRIC_DEPR,   // obsolete value from 6.9-1
                           DIRECT_UNSYMMETRIC_DEPR, // obsolete value from 6.9-1
                           SOLVER_DEFAULT_DEPR };   // obsolete value from 6.9-1
    enum MatrixStorageEnm { SOLVER_DEFAULT, SYMMETRIC, UNSYMMETRIC };
    enum ConvertSDIEnm { CONVERT_SDI_OFF, CONVERT_SDI_ON, CONVERT_SDI_PROPAGATED };

    // Virtual methods

    virtual kprC_Step* Copy() const = 0;
    virtual bool IsAdiabatic() const = 0;
    virtual bool AllowLoadCases() const = 0;
    virtual bool AllowAdaptiveMeshDomains() const = 0;
    virtual typ_typeTag ProcedureType() const = 0;
    virtual bool Nonmechanical() const = 0;
    virtual bool Explicit() const = 0;
    virtual ResponseEnm Response() const = 0;
    virtual double TimePeriod() const = 0;
    virtual NlgeomEnm Nlgeom() const = 0;
    virtual bool Perturbation() const = 0;
    virtual void AssertValid() const = 0;               // Consistency checking
    virtual bool Flow() const;
    virtual bool SimBased() const; 

    static bool Perturbation( typ_typeTag );
    static double TypicalMinimumDef(double, double);
    static double TimePeriodBasedMinimumDef(double);
    static inline int reformKernelDef();

    // Methods
    virtual void Suppress() = 0;
    virtual void Resume() = 0;

    bool IsSuppressed() const { return suppress; }

    // Data member

    enum StepTypeEnm { INITIAL, ANALYSIS };

    cow_String solverInput;
    cow_String description;
    bool       suppress;
    int        stepId;

    // Messaging/database interface

    kprC_Step *             DeepCopy() const {
        return (kprC_Step*)DDB_DeepCopyPtr(this, DynTypeId());
    }

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    static omn_FixedString nameCmd();
    static omn_FixedString stepCmd();
    static omn_FixedString stepRepositoryCmd();
    static omn_FixedString copyCmd();
    static omn_FixedString setParametersCmd();
    static omn_FixedString solverInputCmd();
    static omn_FixedString boundaryConditionCmd();
    static omn_FixedString descriptionCmd();
    static omn_FixedString generalStepsCmd();
    static omn_FixedString perturbationStepsCmd();
    static omn_FixedString generalReplacementStepsCmd();
    static omn_FixedString perturbationReplacementStepsCmd();
    static omn_FixedString initialCmd();
    static omn_FixedString maintainAttributesCmd();
    static omn_FixedString procedureTypeCmd();
    static omn_FixedString allowAdaptiveMeshDomainsCmd();
    static omn_FixedString suppressedCmd();
    static omn_FixedString suppressCmd();
    static omn_FixedString resumeCmd();

    static omn_FixedString NLGEOM_ON();
    static omn_FixedString NLGEOM_OFF();
    static omn_FixedString NLGEOM_NA();


    kprC_Step( const cls_ReadVisitor& rv );
    static void* Ctor( cls_ReadVisitor& rv );
    static void CowDtor(cls_IntCOW* cow);
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(kprC_Step, cow_Virtual);

typedef omu_PrimEnum<kprC_Step::AmplitudeEnm,1> kprC_AmplitudeConst;
typedef omu_PrimEnum<kprC_Step::ExtrapolationEnm,1> kprC_ExtrapolationConst;
typedef omu_PrimEnum<kprC_Step::MatrixSolverEnm,1> kprC_MatrixSolverConst;
typedef omu_PrimEnum<kprC_Step::MatrixStorageEnm,1> kprC_MatrixStorageConst;
typedef omu_PrimEnum<kprC_Step::ConvertSDIEnm,1> kprC_ConvertSDIConst;
typedef omu_PrimEnum<kprC_Step::DefaultEnm,1> kprC_DefaultConst;
typedef omu_PrimEnum<kprC_Step::EigensolverEnm,1> kprC_EigensolverConst;
typedef omu_PrimEnum<kprC_Step::SolutionTechniqueEnm,1> kprC_SolutionTechniqueConst;
typedef omu_PrimEnum<kprC_Step::NlgeomEnm,1> kprC_NlgeomConst;
typedef omu_PrimEnum<kprC_Step::ResponseEnm,1> kprC_ResponseConst;
typedef omu_PrimEnum<kprC_Step::TimeIncrementationMethodEnm,1> kprC_IncrementationConst;
typedef omu_PrimEnum<typ_typeTag,602> kprC_ProcedureTypeConst;

inline int kprC_Step::reformKernelDef() {
    return 8;
}

#endif /* kprC_Step_h */

