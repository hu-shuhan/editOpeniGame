//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kprC_MassScaling_h
#define kprC_MassScaling_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kprC_MassScaling.h
//
// Description:  Mass scaling class definition.
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <rgnC_Region.h>
#include <typ_typeTag.h>

#include <cow_COW.h>

#include <cls_xo1DArrayList.h>
#include <cls_Uid.h>
#include <cls_IntCOW.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Class definition

class kprC_MassScaling  : public mem_AllocationOperators
{
public:
    
    kprC_MassScaling();
    kprC_MassScaling* Copy() const;
    
    // Enum
    enum MassScalingObjectiveEnm { SEMI_AUTOMATIC, AUTOMATIC, REINITIALIZE, 
                          DISABLE_THROUGHOUT_STEP };
    enum MassScalingTypeEnm { UNIFORM, BELOW_MIN, SET_EQUAL_DT, ROLLING };
    enum MassScalingDirectionEnm { GLOBAL_X, GLOBAL_Y, GLOBAL_Z, GLOBAL_NONE };
    enum MassScalingModelEnm { MODEL } ;
    enum MassScalingPreviousStepEnm { PREVIOUS_STEP, ARRAY } ;
    enum MassScalingOccursEnm { AT_BEGINNING, THROUGHOUT_STEP } ;
    
    // Methods
    
    void AssertValid() const;
    bool IsVariable() const; // Whether fixed or variable mass scaling
    bool IsEqual(const kprC_MassScaling&) const;

    // Data members
    MassScalingObjectiveEnm objective;
    rgnC_Region region;
    MassScalingOccursEnm occurs;
    double factor;
    double dt;
    MassScalingTypeEnm type;
    int frequency;
    int numberInterval;
    double feedRate;
    double extrudedLength;
    int crossSection;
    MassScalingDirectionEnm direction;
    
    cls_Uid m_ClsUid;

    // Static data members
    
    static inline MassScalingObjectiveEnm objectiveDef();
    static inline MassScalingOccursEnm occursDef();
    static inline double factorDef();
    static inline double dtDef();
    static inline MassScalingTypeEnm typeDef();
    static inline int frequencyDef();
    static inline int numberIntervalDef();
    static inline double feedRateDef();
    static inline double extrudedLengthDef();
    static inline int crossSectionDef();
    static inline MassScalingDirectionEnm directionDef();
    
    static omn_FixedString objectiveCmd();
    static omn_FixedString regionCmd();
    static omn_FixedString occursCmd();
    static omn_FixedString factorCmd();
    static omn_FixedString dtCmd();
    static omn_FixedString typeCmd();
    static omn_FixedString frequencyCmd();
    static omn_FixedString numberIntervalCmd();
    static omn_FixedString feedRateCmd();
    static omn_FixedString extrudedLengthCmd();
    static omn_FixedString crossSectionCmd();
    static omn_FixedString directionCmd();
    static omn_FixedString massScalingCmd();
    
    // Messaging/database interface
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

public: // Database interface
    kprC_MassScaling(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;
};

COW_COW2_DECL(kprC_MassScaling,cow_Virtual);

typedef omu_PrimEnum<kprC_MassScaling::MassScalingObjectiveEnm,1> kprC_MassScalingObjectiveConst;
typedef omu_PrimEnum<kprC_MassScaling::MassScalingModelEnm,1> kprC_MassScalingModelConst;
typedef omu_PrimEnum<kprC_MassScaling::MassScalingPreviousStepEnm,1> kprC_MassScalingPreviousStepConst;
typedef omu_PrimEnum<kprC_MassScaling::MassScalingOccursEnm,1> kprC_MassScalingOccursConst;
typedef omu_PrimEnum<kprC_MassScaling::MassScalingTypeEnm,1> kprC_MassScalingTypeConst;
typedef omu_PrimEnum<kprC_MassScaling::MassScalingDirectionEnm,1> kprC_MassScalingDirectionConst;

CLS_xo1DARRAYLIST_DECL(kprC_MassScaling, kprC_MassScalingList)

bool operator==(const kprC_MassScaling&, const kprC_MassScaling&);
bool operator!=(const kprC_MassScaling&, const kprC_MassScaling&);


inline kprC_MassScaling::MassScalingObjectiveEnm kprC_MassScaling::objectiveDef() {
    return kprC_MassScaling::SEMI_AUTOMATIC;
}
inline kprC_MassScaling::MassScalingOccursEnm kprC_MassScaling::occursDef() {
    return kprC_MassScaling::AT_BEGINNING;
}
inline double kprC_MassScaling::factorDef() {
    return 0.0;
}
inline double kprC_MassScaling::dtDef() {
    return 0.0;
}
inline kprC_MassScaling::MassScalingTypeEnm kprC_MassScaling::typeDef() {
    return kprC_MassScaling::BELOW_MIN;
}
inline int kprC_MassScaling::frequencyDef() {
    return 0;
}
inline int kprC_MassScaling::numberIntervalDef() {
    return 0;
}
inline double kprC_MassScaling::feedRateDef() {
    return 0.0;
}
inline double kprC_MassScaling::extrudedLengthDef() {
    return 0.0;
}
inline int kprC_MassScaling::crossSectionDef() {
    return 0;
}
inline kprC_MassScaling::MassScalingDirectionEnm kprC_MassScaling::directionDef() {
    return kprC_MassScaling::GLOBAL_NONE;
}

#endif /* kprC_MassScaling_h */
