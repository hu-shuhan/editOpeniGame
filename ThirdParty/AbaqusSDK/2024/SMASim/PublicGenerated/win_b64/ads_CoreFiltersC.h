//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreFiltersC_h
#define ads_CoreFiltersC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Filters of the latest level of form Core */

/** This data record describes a Butterworth type filter. */
#define ads_ButterworthFilter (ads_CoreFragmentTypeIndex(ads_CoreFiltersFragment, 0))

/** This data record describes a Chebyshev Type I filter. */
#define ads_Chebyshev1Filter (ads_CoreFragmentTypeIndex(ads_CoreFiltersFragment, 1))

/** This data record describes a Chebyshev Type II filter. */
#define ads_Chebyshev2Filter (ads_CoreFragmentTypeIndex(ads_CoreFiltersFragment, 2))

/** An abstraction of all available low-pass filters. */
#define ads_Filter (ads_CoreFragmentTypeIndex(ads_CoreFiltersFragment, 3))

/** The model record is in a 1-to-many composition with the filters in order to provide easy access to them and in order to name them. */
#define ads_Model_filter (ads_CoreFragmentTypeIndex(ads_CoreFiltersFragment, 4))

#define ads_OutputRequest_filter (ads_CoreFragmentTypeIndex(ads_CoreFiltersFragment, 5))

/** 
Enum with record members. */
enum ads_ButterworthFilterMembersEnm
{
    ads_ButterworthFilter_cutoffFrequency,
    ads_ButterworthFilter_extraOutputFrame,
    ads_ButterworthFilter_halt,
    ads_ButterworthFilter_invariant,
    ads_ButterworthFilter_limit,
    ads_ButterworthFilter_operator,
    ads_ButterworthFilter_order,
    ads_ButterworthFilter_startCondition,
    ads_ButterworthFilter_startValue
};

enum ads_ButterworthFilter_haltEnm
{
    ads_ButterworthFilter_halt_ANALYSIS,
    ads_ButterworthFilter_halt_DEFAULT,
    ads_ButterworthFilter_halt_STEP
};

enum ads_ButterworthFilter_invariantEnm
{
    ads_ButterworthFilter_invariant_DEFAULT,
    ads_ButterworthFilter_invariant_FIRST,
    ads_ButterworthFilter_invariant_INTERMP,
    ads_ButterworthFilter_invariant_MAXP,
    ads_ButterworthFilter_invariant_MINP,
    ads_ButterworthFilter_invariant_SECOND
};

enum ads_ButterworthFilter_operatorEnm
{
    ads_ButterworthFilter_operator_ABSMAX,
    ads_ButterworthFilter_operator_DEFAULT,
    ads_ButterworthFilter_operator_MAX,
    ads_ButterworthFilter_operator_MIN
};

enum ads_ButterworthFilter_startConditionEnm
{
    ads_ButterworthFilter_startCondition_DC,
    ads_ButterworthFilter_startCondition_USER
};

/** 
Enum with record members. */
enum ads_Chebyshev1FilterMembersEnm
{
    ads_Chebyshev1Filter_cutoffFrequency,
    ads_Chebyshev1Filter_extraOutputFrame,
    ads_Chebyshev1Filter_halt,
    ads_Chebyshev1Filter_invariant,
    ads_Chebyshev1Filter_limit,
    ads_Chebyshev1Filter_operator,
    ads_Chebyshev1Filter_order,
    ads_Chebyshev1Filter_startCondition,
    ads_Chebyshev1Filter_startValue,
    ads_Chebyshev1Filter_rippleFactor
};

enum ads_Chebyshev1Filter_haltEnm
{
    ads_Chebyshev1Filter_halt_ANALYSIS,
    ads_Chebyshev1Filter_halt_DEFAULT,
    ads_Chebyshev1Filter_halt_STEP
};

enum ads_Chebyshev1Filter_invariantEnm
{
    ads_Chebyshev1Filter_invariant_DEFAULT,
    ads_Chebyshev1Filter_invariant_FIRST,
    ads_Chebyshev1Filter_invariant_INTERMP,
    ads_Chebyshev1Filter_invariant_MAXP,
    ads_Chebyshev1Filter_invariant_MINP,
    ads_Chebyshev1Filter_invariant_SECOND
};

enum ads_Chebyshev1Filter_operatorEnm
{
    ads_Chebyshev1Filter_operator_ABSMAX,
    ads_Chebyshev1Filter_operator_DEFAULT,
    ads_Chebyshev1Filter_operator_MAX,
    ads_Chebyshev1Filter_operator_MIN
};

enum ads_Chebyshev1Filter_startConditionEnm
{
    ads_Chebyshev1Filter_startCondition_DC,
    ads_Chebyshev1Filter_startCondition_USER
};

/** 
Enum with record members. */
enum ads_Chebyshev2FilterMembersEnm
{
    ads_Chebyshev2Filter_cutoffFrequency,
    ads_Chebyshev2Filter_extraOutputFrame,
    ads_Chebyshev2Filter_halt,
    ads_Chebyshev2Filter_invariant,
    ads_Chebyshev2Filter_limit,
    ads_Chebyshev2Filter_operator,
    ads_Chebyshev2Filter_order,
    ads_Chebyshev2Filter_startCondition,
    ads_Chebyshev2Filter_startValue,
    ads_Chebyshev2Filter_rippleFactor
};

enum ads_Chebyshev2Filter_haltEnm
{
    ads_Chebyshev2Filter_halt_ANALYSIS,
    ads_Chebyshev2Filter_halt_DEFAULT,
    ads_Chebyshev2Filter_halt_STEP
};

enum ads_Chebyshev2Filter_invariantEnm
{
    ads_Chebyshev2Filter_invariant_DEFAULT,
    ads_Chebyshev2Filter_invariant_FIRST,
    ads_Chebyshev2Filter_invariant_INTERMP,
    ads_Chebyshev2Filter_invariant_MAXP,
    ads_Chebyshev2Filter_invariant_MINP,
    ads_Chebyshev2Filter_invariant_SECOND
};

enum ads_Chebyshev2Filter_operatorEnm
{
    ads_Chebyshev2Filter_operator_ABSMAX,
    ads_Chebyshev2Filter_operator_DEFAULT,
    ads_Chebyshev2Filter_operator_MAX,
    ads_Chebyshev2Filter_operator_MIN
};

enum ads_Chebyshev2Filter_startConditionEnm
{
    ads_Chebyshev2Filter_startCondition_DC,
    ads_Chebyshev2Filter_startCondition_USER
};

/** 
Enum with record members. */
enum ads_FilterMembersEnm
{
    ads_Filter_cutoffFrequency,
    ads_Filter_extraOutputFrame,
    ads_Filter_halt,
    ads_Filter_invariant,
    ads_Filter_limit,
    ads_Filter_operator,
    ads_Filter_order,
    ads_Filter_startCondition,
    ads_Filter_startValue
};

enum ads_Filter_haltEnm
{
    ads_Filter_halt_ANALYSIS,
    ads_Filter_halt_DEFAULT,
    ads_Filter_halt_STEP
};

enum ads_Filter_invariantEnm
{
    ads_Filter_invariant_DEFAULT,
    ads_Filter_invariant_FIRST,
    ads_Filter_invariant_INTERMP,
    ads_Filter_invariant_MAXP,
    ads_Filter_invariant_MINP,
    ads_Filter_invariant_SECOND
};

enum ads_Filter_operatorEnm
{
    ads_Filter_operator_ABSMAX,
    ads_Filter_operator_DEFAULT,
    ads_Filter_operator_MAX,
    ads_Filter_operator_MIN
};

enum ads_Filter_startConditionEnm
{
    ads_Filter_startCondition_DC,
    ads_Filter_startCondition_USER
};

/** 
Enum with association roles. */
enum ads_Model_filterRolesEnm
{
    ads_Model_filter_child,
    ads_Model_filter_parent
};

/** Enum with association roles. */
enum ads_OutputRequest_filterRolesEnm
{
    ads_OutputRequest_filter_referent,
    ads_OutputRequest_filter_referrer
};

#endif
