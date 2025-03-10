//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreCurveC_h
#define ads_CoreCurveC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Curve of the latest level of form Core */

/** Curve base record. A curve represents a specific function F(t). Curve is an abstract base class. The derived types actual curve types. Currently, the supported curve types correspond to AMPLITUDE types in the ABAQUS input file; see the ABAQUS documentation for the meaning of these curves. */
#define ads_Curve (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 0))

/** Actuator curve record. */
#define ads_Curve_Actuator (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 1))

/** Decay curve record. */
#define ads_Curve_Decay (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 2))

/** Modulated curve record. */
#define ads_Curve_Modulated (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 3))

/** Periodic curve record. */
#define ads_Curve_Periodic (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 4))

/** Decay curve cosine */
#define ads_Curve_Periodic_cosines (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 5))

/** Decay curve sine */
#define ads_Curve_Periodic_sines (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 6))

/** Smooth step curve record. */
#define ads_Curve_SmoothStep (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 7))

/** Same as magnitudes (TimeGrid) except domain is FrequencyGrid. */
#define ads_Curve_SmoothStep_freqMagnitudes (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 8))

/** Smooth step curve magnitude. The distribution given by magnitudes is of the form Frames --> Value. The frames collection in question is a registered frames collection, so the 'time' value of the frames is available separately. */
#define ads_Curve_SmoothStep_magnitudes (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 9))

/** Tabular curve record. */
#define ads_Curve_Tabular (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 10))

/** Same as magnitudes (TimeGrid) except domain is FrequencyGrid. */
#define ads_Curve_Tabular_freqMagnitudes (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 11))

/** Tabular curve magnitudes. The distribution given by magnitudes is of the form Frames --> Value. The frames collection in question is a registered frames collection, so the 'time' value of the frames is available separately. */
#define ads_Curve_Tabular_magnitudes (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 12))

/** User curve record. */
#define ads_Curve_User (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 13))

/** To define user amplitude properties. */
#define ads_Curve_User_table (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 14))

/** Model curves which are used for AMPLITUDE records, in order to provide easy access to them and in order to name them. NOTE: THIS WILL PROBABLY BE REMOVED, or moved elsewhere, when Amplitudes are designed. It does not have to be used for curves which are not used for AMPLITUDES. */
#define ads_Model_amplitude (ads_CoreFragmentTypeIndex(ads_CoreCurveFragment, 15))

/** 
Enum with record members. */
enum ads_CurveMembersEnm
{
    ads_Curve_time
};

enum ads_Curve_timeEnm
{
    ads_Curve_time_STEP_TIME,
    ads_Curve_time_TOTAL_TIME
};

/** 
Enum with record members. */
enum ads_Curve_ActuatorMembersEnm
{
    ads_Curve_Actuator_time
};

enum ads_Curve_Actuator_timeEnm
{
    ads_Curve_Actuator_time_STEP_TIME,
    ads_Curve_Actuator_time_TOTAL_TIME
};

/** 
Enum with record members. */
enum ads_Curve_DecayMembersEnm
{
    ads_Curve_Decay_time,
    ads_Curve_Decay_a0,
    ads_Curve_Decay_magnitude,
    ads_Curve_Decay_t0,
    ads_Curve_Decay_td
};

enum ads_Curve_Decay_timeEnm
{
    ads_Curve_Decay_time_STEP_TIME,
    ads_Curve_Decay_time_TOTAL_TIME
};

/** 
Enum with record members. */
enum ads_Curve_ModulatedMembersEnm
{
    ads_Curve_Modulated_time,
    ads_Curve_Modulated_a0,
    ads_Curve_Modulated_magnitude,
    ads_Curve_Modulated_omega1,
    ads_Curve_Modulated_omega2,
    ads_Curve_Modulated_t0
};

enum ads_Curve_Modulated_timeEnm
{
    ads_Curve_Modulated_time_STEP_TIME,
    ads_Curve_Modulated_time_TOTAL_TIME
};

/** 
Enum with record members. */
enum ads_Curve_PeriodicMembersEnm
{
    ads_Curve_Periodic_time,
    ads_Curve_Periodic_a0,
    ads_Curve_Periodic_omega,
    ads_Curve_Periodic_t0
};

enum ads_Curve_Periodic_timeEnm
{
    ads_Curve_Periodic_time_STEP_TIME,
    ads_Curve_Periodic_time_TOTAL_TIME
};

/** 
Enum with association roles. */
enum ads_Curve_Periodic_cosinesRolesEnm
{
    ads_Curve_Periodic_cosines_child,
    ads_Curve_Periodic_cosines_parent
};

/** 
Enum with association roles. */
enum ads_Curve_Periodic_sinesRolesEnm
{
    ads_Curve_Periodic_sines_child,
    ads_Curve_Periodic_sines_parent
};

/** 
Enum with record members. */
enum ads_Curve_SmoothStepMembersEnm
{
    ads_Curve_SmoothStep_time
};

enum ads_Curve_SmoothStep_timeEnm
{
    ads_Curve_SmoothStep_time_STEP_TIME,
    ads_Curve_SmoothStep_time_TOTAL_TIME
};

/** 
Enum with association roles. */
enum ads_Curve_SmoothStep_freqMagnitudesRolesEnm
{
    ads_Curve_SmoothStep_freqMagnitudes_child,
    ads_Curve_SmoothStep_freqMagnitudes_parent
};

/** 
Enum with association roles. */
enum ads_Curve_SmoothStep_magnitudesRolesEnm
{
    ads_Curve_SmoothStep_magnitudes_child,
    ads_Curve_SmoothStep_magnitudes_parent
};

/** 
Enum with record members. */
enum ads_Curve_TabularMembersEnm
{
    ads_Curve_Tabular_time,
    ads_Curve_Tabular_equallySpaced,
    ads_Curve_Tabular_smoothingFactor
};

enum ads_Curve_Tabular_timeEnm
{
    ads_Curve_Tabular_time_STEP_TIME,
    ads_Curve_Tabular_time_TOTAL_TIME
};

/** 
Enum with association roles. */
enum ads_Curve_Tabular_freqMagnitudesRolesEnm
{
    ads_Curve_Tabular_freqMagnitudes_child,
    ads_Curve_Tabular_freqMagnitudes_parent
};

/** 
Enum with association roles. */
enum ads_Curve_Tabular_magnitudesRolesEnm
{
    ads_Curve_Tabular_magnitudes_child,
    ads_Curve_Tabular_magnitudes_parent
};

/** 
Enum with record members. */
enum ads_Curve_UserMembersEnm
{
    ads_Curve_User_time,
    ads_Curve_User_numVariables
};

enum ads_Curve_User_timeEnm
{
    ads_Curve_User_time_STEP_TIME,
    ads_Curve_User_time_TOTAL_TIME
};

/** 
Enum with association roles. */
enum ads_Curve_User_tableRolesEnm
{
    ads_Curve_User_table_child,
    ads_Curve_User_table_parent
};

/** 
Enum with association roles. */
enum ads_Model_amplitudeRolesEnm
{
    ads_Model_amplitude_child,
    ads_Model_amplitude_parent
};

#endif
