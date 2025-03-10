//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef  kprC_typeUtils_h
#define  kprC_typeUtils_h

// Begin local includes
#include <kprC_Step.h>
#include <kprC_StepNameProcedurePair.h>
#include <omu_PrimEnum.h>
#include <typ_typeTag.h>

enum kprC_AnalysisProductEnm { kprC_STANDARD_ANALYSIS, kprC_EXPLICIT_ANALYSIS,
                               kprC_FLOW_ANALYSIS, kprC_UNDEFINED_ANALYSIS };
enum kprC_TurbulenceModelEnm { TURB_NONE, SPALART, KEPS_RNG, VMS };
enum kprC_EnergyEquationEnm { ENERGY_NONE, TEMPERATURE };

typedef omu_PrimEnum<kprC_EnergyEquationEnm,1> kprC_FlowStepEnergyEquationConst;
typedef omu_PrimEnum<kprC_TurbulenceModelEnm,1> kprC_FlowStepTurbulenceModelConst;
typedef omu_PrimEnum<kprC_AnalysisProductEnm,1> kprC_AnalysisProductConst;

bool kprC_IsExplicit(typ_typeTag);
bool kprC_IsFlow(typ_typeTag);
bool kprC_IsEmag(typ_typeTag);
bool kprC_IsFrequencyBuckle(typ_typeTag);
bool kprC_IsComplexFrequencyBuckle(typ_typeTag);
bool kprC_IsImplicitModal(typ_typeTag);
bool kprC_IsCoupledStep(typ_typeTag);
bool kprC_HasRotationControls(typ_typeTag);
bool kprC_HasDisplacementControls(typ_typeTag);
bool kprC_HasHydrostaticControls(typ_typeTag);
bool kprC_HasTemperatureControls(typ_typeTag);
bool kprC_HasPoreControls(typ_typeTag);
bool kprC_HasConcentrationControls(typ_typeTag);
bool kprC_HasElectricalControls(typ_typeTag);
bool kprC_LoadCaseIsAllowed(typ_typeTag type);

bool kprC_TimeIntervalIsAllowed(typ_typeTag type);
bool kprC_TimePointIsAllowed(typ_typeTag type, bool isField);
bool kprC_OutputFrequencyIsAllowed(typ_typeTag type, bool isField);
bool kprC_ModesIsAllowed(typ_typeTag type);

bool kprC_NumberIntervalIsAllowed(typ_typeTag type);
bool kprC_NumberIntervalIsAllowed(const kprC_Step& step );
bool kprC_NumberIntervalConsistency(const kprC_Step& step );

// If field/history are not applicable, pass isField==true
bool kprC_TimeMarksIsAllowed(typ_typeTag type, bool isField);
bool kprC_TimeMarksIsAllowed(const kprC_Step&, bool isField );
bool kprC_TimeMarksConsistency(const kprC_Step&, bool isField );

kprC_AnalysisProductEnm kprC_AnalysisProduct(const kprC_StepNameProcedurePairList& pairs);
typ_typeTag kprC_FirstGeneralAnalysisType(const kprC_StepNameProcedurePairList& pairs);

bool kprC_validProcedureInOptimization(typ_typeTag type);
bool kprC_validProcedureInFillInLevel(typ_typeTag type);
bool kprC_validProcedureForIntegratedOutput(typ_typeTag type);
#endif /*kprC_typeUtils_h*/

