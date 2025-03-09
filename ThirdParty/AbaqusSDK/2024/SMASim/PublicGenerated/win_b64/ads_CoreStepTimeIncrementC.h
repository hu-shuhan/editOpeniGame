//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreStepTimeIncrementC_h
#define ads_CoreStepTimeIncrementC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment StepTimeIncrement of the latest level of form Core */

/** Time incrementation for a general analysis step. */
#define ads_Step_Gen_timeIncrementation (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 0))

/** Time incrementation base type */
#define ads_TimeIncrementation (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 1))

/** Static step automatic time incrementation control */
#define ads_TimeIncrementation_Automatic (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 2))

/** Automatic incrementation with explicit integration. */
#define ads_TimeIncrementation_Automatic_ExplicitDynamic (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 3))

/** Heat transfer step automatic incrementation control */
#define ads_TimeIncrementation_Automatic_HeatTransfer (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 4))

/** Automatic incrementation with implicit integration. */
#define ads_TimeIncrementation_Automatic_ImplicitDynamic (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 5))

/** Automatic incrementation with maximum normalized concentration change. */
#define ads_TimeIncrementation_Automatic_MassDiffusion (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 6))

/** Automatic incrementation with creep strain rate and maximum pore pressure change control. */
#define ads_TimeIncrementation_Automatic_Soils (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 7))

/** Static RIKS step automatic incrementation control */
#define ads_TimeIncrementation_Automatic_StaticRiks (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 8))

/** Automatic incrementation with creep strain rate and maximum temperature change control. */
#define ads_TimeIncrementation_Automatic_ThermallyAdjusted (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 9))

/** Automatic incrementation with creep strain rate control. */
#define ads_TimeIncrementation_Automatic_Visco (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 10))

/** Direct incrementation control */
#define ads_TimeIncrementation_Fixed (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 11))

/** Heat transfer step fixed incrementation control */
#define ads_TimeIncrementation_Fixed_HeatTransfer (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 12))

/** Fixed incrementation with implicit integration. */
#define ads_TimeIncrementation_Fixed_ImplicitDynamic (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 13))

/** Fixed incrementation with a scaled computed increment value. */
#define ads_TimeIncrementation_Fixed_Scaled (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 14))

/** Static RIKS step direct incrementation control */
#define ads_TimeIncrementation_Fixed_StaticRiks (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 15))

/** Fixed incrementation for Static procedure. */
#define ads_TimeIncrementation_Fixed_Stop (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 16))

#define ads_TimeIncrementation_Fixed_amplitude (ads_CoreFragmentTypeIndex(ads_CoreStepTimeIncrementFragment, 17))

/** 
Enum with association roles. */
enum ads_Step_Gen_timeIncrementationRolesEnm
{
    ads_Step_Gen_timeIncrementation_child,
    ads_Step_Gen_timeIncrementation_parent
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_AutomaticMembersEnm
{
    ads_TimeIncrementation_Automatic_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_minimumTimeIncrement
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_ExplicitDynamicMembersEnm
{
    ads_TimeIncrementation_Automatic_ExplicitDynamic_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_ExplicitDynamic_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_ExplicitDynamic_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_ExplicitDynamic_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_ExplicitDynamic_elementByElement,
    ads_TimeIncrementation_Automatic_ExplicitDynamic_improvedDTMethod,
    ads_TimeIncrementation_Automatic_ExplicitDynamic_scaleFactor
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_HeatTransferMembersEnm
{
    ads_TimeIncrementation_Automatic_HeatTransfer_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_HeatTransfer_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_HeatTransfer_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_HeatTransfer_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_HeatTransfer_endAtSteadyState,
    ads_TimeIncrementation_Automatic_HeatTransfer_maximumEmissivityChange,
    ads_TimeIncrementation_Automatic_HeatTransfer_maximumTemperatureChange,
    ads_TimeIncrementation_Automatic_HeatTransfer_steadyStateTemperatureChangeRate
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_ImplicitDynamicMembersEnm
{
    ads_TimeIncrementation_Automatic_ImplicitDynamic_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_halfIncScaleFactor,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_halfStepResidualTolerance,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_incrementation
};

enum ads_TimeIncrementation_Automatic_ImplicitDynamic_incrementationEnm
{
    ads_TimeIncrementation_Automatic_ImplicitDynamic_incrementation_AGGRESSIVE,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_incrementation_CONSERVATIVE,
    ads_TimeIncrementation_Automatic_ImplicitDynamic_incrementation_SOLVER_DEFAULT
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_MassDiffusionMembersEnm
{
    ads_TimeIncrementation_Automatic_MassDiffusion_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_MassDiffusion_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_MassDiffusion_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_MassDiffusion_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_MassDiffusion_endAtSteadyState,
    ads_TimeIncrementation_Automatic_MassDiffusion_maximumConcentrationChangeIncrement,
    ads_TimeIncrementation_Automatic_MassDiffusion_steadyConcentrationChangeRate
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_SoilsMembersEnm
{
    ads_TimeIncrementation_Automatic_Soils_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_Soils_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_Soils_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_Soils_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_Soils_endAtSteadyState,
    ads_TimeIncrementation_Automatic_Soils_maximumCreepStrainRateIncrement,
    ads_TimeIncrementation_Automatic_Soils_maximumPorePressureIncrement,
    ads_TimeIncrementation_Automatic_Soils_steadyStatePorePressureChangeRate
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_StaticRiksMembersEnm
{
    ads_TimeIncrementation_Automatic_StaticRiks_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_StaticRiks_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_StaticRiks_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_StaticRiks_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_StaticRiks_initialArcLengthIncrement,
    ads_TimeIncrementation_Automatic_StaticRiks_maximumArcLengthIncrement,
    ads_TimeIncrementation_Automatic_StaticRiks_maximumLoadProportionalityFactor,
    ads_TimeIncrementation_Automatic_StaticRiks_minimumArcLengthIncrement,
    ads_TimeIncrementation_Automatic_StaticRiks_totalArcLengthScaleFactor
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_ThermallyAdjustedMembersEnm
{
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_allsdtol,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_dampingFactor,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_maximumCreepStrainRateIncrement,
    ads_TimeIncrementation_Automatic_ThermallyAdjusted_maximumTemperatureIncrement
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Automatic_ViscoMembersEnm
{
    ads_TimeIncrementation_Automatic_Visco_initialTimeIncrement,
    ads_TimeIncrementation_Automatic_Visco_maximumNumberIncrements,
    ads_TimeIncrementation_Automatic_Visco_maximumTimeIncrement,
    ads_TimeIncrementation_Automatic_Visco_minimumTimeIncrement,
    ads_TimeIncrementation_Automatic_Visco_maximumCreepStrainRateIncrement
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_FixedMembersEnm
{
    ads_TimeIncrementation_Fixed_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_maximumNumberIncrements
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_HeatTransferMembersEnm
{
    ads_TimeIncrementation_Fixed_HeatTransfer_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_HeatTransfer_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_HeatTransfer_endAtSteadyState,
    ads_TimeIncrementation_Fixed_HeatTransfer_steadyStateTemperatureChangeRate
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_ImplicitDynamicMembersEnm
{
    ads_TimeIncrementation_Fixed_ImplicitDynamic_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_ImplicitDynamic_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_ImplicitDynamic_stopWithoutEquilibrium
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_ScaledMembersEnm
{
    ads_TimeIncrementation_Fixed_Scaled_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_Scaled_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_Scaled_improvedDTMethod,
    ads_TimeIncrementation_Fixed_Scaled_scaleFactor
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_StaticRiksMembersEnm
{
    ads_TimeIncrementation_Fixed_StaticRiks_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_StaticRiks_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_StaticRiks_initialArcLengthIncrement,
    ads_TimeIncrementation_Fixed_StaticRiks_maximumLoadProportionalityFactor,
    ads_TimeIncrementation_Fixed_StaticRiks_stopWithoutEquilibrium,
    ads_TimeIncrementation_Fixed_StaticRiks_totalArcLengthScaleFactor
};

/** 
Enum with record members. */
enum ads_TimeIncrementation_Fixed_StopMembersEnm
{
    ads_TimeIncrementation_Fixed_Stop_initialTimeIncrement,
    ads_TimeIncrementation_Fixed_Stop_maximumNumberIncrements,
    ads_TimeIncrementation_Fixed_Stop_stopWithoutEquilibrium
};

/** Enum with association roles. */
enum ads_TimeIncrementation_Fixed_amplitudeRolesEnm
{
    ads_TimeIncrementation_Fixed_amplitude_referent,
    ads_TimeIncrementation_Fixed_amplitude_referrer
};

#endif
