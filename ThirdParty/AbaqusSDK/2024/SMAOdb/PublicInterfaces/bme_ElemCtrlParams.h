//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: bme_ElemCtrlParams.h
//
//
//
//

#ifndef bme_ElemCtrlParams_h
#define bme_ElemCtrlParams_h

//
// SECTION: System includes
//

//
// Begin Local includes
//

#include <omi_types.h>
#include <typ_typeTag.h>
#include <idb_Utils.h>

#include <omu_PrimEnum.h>

#include <cow_COW.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>


//
// SECTION: Forward Declarations
//
// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;



enum bme_KinematicSplit
{
    bme_AverageStrain=1,
    bme_Orthogonal=2,
    bme_Centroid=3
};

typedef omu_PrimEnum<bme_KinematicSplit,1> bme_KinematicSplitPE;
   
enum bme_HourglassControl
{
    bme_RelaxStiffness=1,
    bme_Stiffness=2,
    bme_Viscous=3,
    bme_Combined=4,
    bme_Enhanced=5,
    bme_DefaultHC=6    
};

typedef omu_PrimEnum<bme_HourglassControl,1> bme_HourglassControlPE;

enum bme_StressRate
{
    bme_Polar=1,
    bme_Spin=2
};     

typedef omu_PrimEnum<bme_StressRate,1> bme_StressRatePE;

enum bme_ElemLibrary
{
    bme_UnknownLib = 0,
    bme_STD = 1,
    bme_EXP = 2,
    bme_CFD = 3
};

enum bme_SoAccuracy
{
    bme_UnknownSoAccuracy = 0,
    bme_SoAccuracyON = 1,
    bme_SoAccuracyOFF = 2
};

enum bme_DistortionControl
{
    bme_UnknownDistortCtrl = 0,
    bme_DistortCtrlON = 1,
    bme_DistortCtrlOFF = 2,
    bme_DefaultDistortCtrl=3 
};

enum bme_ElemDeletion
{
    bme_ElemDeletionUnknown = 0,
    bme_ElemDeletionON = 1,
    bme_ElemDeletionOFF = 2,
    bme_DefaultElemDeletion=3 
};

enum bme_ParticleConvCriteria
{
    bme_ParticleConvDEFAULT = 0,
    bme_ParticleConvOFF = 1,
    bme_ParticleConvTIME = 2,
    bme_ParticleConvSTRAIN = 3,
    bme_ParticleConvSTRESS = 4
};


typedef omu_PrimEnum<bme_SoAccuracy,1> bme_SoAccuracyPE;
typedef omu_PrimEnum<bme_ElemLibrary,1> bme_ElemLibraryPE;
typedef omu_PrimEnum<bme_DistortionControl,1> bme_DistortionControlPE;
typedef omu_PrimEnum<bme_ElemDeletion,1> bme_ElemDeletionPE;
typedef omu_PrimEnum<bme_ParticleConvCriteria,1> bme_ParticleConvCriteriaPE;


class bme_ElemCtrlParams  : public mem_AllocationOperators
{

public: // Database interface
    bme_ElemCtrlParams(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;


public:

    bme_ElemCtrlParams();
    
    // Operators 

    bool operator==(const bme_ElemCtrlParams&) const;
    bool operator!=(const bme_ElemCtrlParams&) const;

    // Query

    bool IsDefault() const; // Uses current element library
    bool IsDefault(bme_ElemLibrary) const;  // Uses specified library
    bool HasDefaultSectionControls(bme_ElemLibrary) const;
    bool HasDefaultHourglassStiffness() const;

    // Change

    void SetElemLibrary(bme_ElemLibrary val) { elemLib=val; }

    void SetHrGlsStiffness(double val) { hrGlsStiffness=val;}
    void SetBendingHrGls(double val)   { bendingHrGls=val;}
    void SetDrillingHrGls(double val)  { drillingHrGls=val;}
    
    void SetKinematicSplit(bme_KinematicSplit val)   { kineSplit=val; }
    void SetHourglassCtrl(bme_HourglassControl val)  { hrGlsControl=val;}
    void SetWeightFactor(double val) { weightFactor=val;}
    
    void SetStressRate(bme_StressRate val) { stressRate=val; } 
    
    void SetSoAccuracy(bme_SoAccuracy val) { soAccuracy = val; }
    void SetDistortionControl(bme_DistortionControl val) { distortionCtrl = val;}
    void SetLengthRatio(double val) { lengthRatio=val;}

    void SetParticleConvCriteria(bme_ParticleConvCriteria val) {particleConvCriteria=val;}
    void SetParticleConvThreshold(double val) {particleConvThreshold=val;}
    void SetParticleConvPPD(int val) {particleConvPPD=val;}
    void SetParticleConvKer(int val) {particleConvKer=val;}

    void SetElemDeletion(bme_ElemDeletion val) { elemDeletion = val;}
    void SetMaxDegradation(double val) { maxDegradation=val;}
    void SetViscosity(double val) { viscosity=val;}

    void SetDisplHrGls(double val)   { displHrGls=val;}
    void SetRotHrGls(double val)     { rotHrGls=val;}
    void SetOOPDispHrGls(double val) { OOPDispHrGls=val;}

    void SetLinearBulkVisk(double val) { linearBulkVisk=val;}
    void SetQuadraticBulkVisk(double val) { quadraticBulkVisk=val;}

    void SetFourierMode(int val) { fourierMode=val;}
    void SetNodeOffset(int val) { nodeOffset=val;}

	void SetLinearKinematicCtrl(double val) { linearKinematicCtrl = val; }
	void SetInitialGapOpening(double val) { initialGapOpening = val; }
    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();



    bme_ElemCtrlParams *             DeepCopy() const {
        return (bme_ElemCtrlParams*)DDB_DeepCopyPtr(this, DynTypeId());
    }


public:

    bme_ElemLibrary elemLib;

    double hrGlsStiffness;
    double bendingHrGls;
    double drillingHrGls;
    
    bme_KinematicSplit   kineSplit;
    bme_HourglassControl hrGlsControl;
    double weightFactor;
    
    bme_StressRate stressRate; 

    bme_SoAccuracy soAccuracy;
    double displHrGls;
    double rotHrGls;
    double OOPDispHrGls;

    bme_DistortionControl distortionCtrl;
    double lengthRatio;

    bme_ElemDeletion elemDeletion; 
    bme_ParticleConvCriteria particleConvCriteria;
    double particleConvThreshold;
    int particleConvPPD, particleConvKer;
    double maxDegradation;        
    double viscosity;       
    double linearBulkVisk;
    double quadraticBulkVisk;
    int fourierMode, nodeOffset;
	double linearKinematicCtrl;
	double initialGapOpening;

private:
    cls_Uid m_ClsUid;
  
};

COW_COW_DECL(bme_ElemCtrlParams);



#endif // bme_ElemCtrlParams_h
