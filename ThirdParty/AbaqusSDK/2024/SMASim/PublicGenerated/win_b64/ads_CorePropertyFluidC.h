//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyFluidC_h
#define ads_CorePropertyFluidC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyFluid of the latest level of form Core */

#define ads_PropIFluidPolynomialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 0))

#define ads_Prop_Fluid (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 1))

#define ads_Prop_Fluid_Test (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 2))

#define ads_Prop_Fluid_fields (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 3))

#define ads_Prop_IFluid (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 4))

#define ads_Prop_IFluid_PressureJump (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 5))

#define ads_Prop_IFluid_PressureJump_Constant (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 6))

#define ads_Prop_IFluid_PressureJump_MassAvgVel (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 7))

#define ads_Prop_IFluid_PressureJump_MassAvgVel_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 8))

#define ads_Prop_IFluid_PressureJump_VolFlowRate (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 9))

#define ads_Prop_IFluid_PressureJump_VolFlowRate_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 10))

#define ads_Prop_IFluid_RadialVelocity (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 11))

#define ads_Prop_IFluid_RadialVelocity_Constant (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 12))

#define ads_Prop_IFluid_RadialVelocity_Polynomial (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 13))

#define ads_Prop_IFluid_RadialVelocity_Polynomial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 14))

#define ads_Prop_IFluid_TangentialVelocity (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 15))

#define ads_Prop_IFluid_TangentialVelocity_Constant (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 16))

#define ads_Prop_IFluid_TangentialVelocity_Polynomial (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 17))

#define ads_Prop_IFluid_TangentialVelocity_Polynomial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyFluidFragment, 18))

/** Enum with association roles. */
enum ads_Prop_Fluid_fieldsRolesEnm
{
    ads_Prop_Fluid_fields_child,
    ads_Prop_Fluid_fields_parent
};

/** Enum with record members. */
enum ads_Prop_IFluid_PressureJump_ConstantMembersEnm
{
    ads_Prop_IFluid_PressureJump_Constant_pressureJump
};

/** Enum with record members. */
enum ads_Prop_IFluid_PressureJump_MassAvgVelMembersEnm
{
    ads_Prop_IFluid_PressureJump_MassAvgVel_maxVelocity,
    ads_Prop_IFluid_PressureJump_MassAvgVel_minVelocity
};

/** Enum with association roles. */
enum ads_Prop_IFluid_PressureJump_MassAvgVel_tableRolesEnm
{
    ads_Prop_IFluid_PressureJump_MassAvgVel_table_child,
    ads_Prop_IFluid_PressureJump_MassAvgVel_table_parent
};

/** Enum with record members. */
enum ads_Prop_IFluid_PressureJump_VolFlowRateMembersEnm
{
    ads_Prop_IFluid_PressureJump_VolFlowRate_maxFlowRate,
    ads_Prop_IFluid_PressureJump_VolFlowRate_minFlowRate
};

/** Enum with association roles. */
enum ads_Prop_IFluid_PressureJump_VolFlowRate_tableRolesEnm
{
    ads_Prop_IFluid_PressureJump_VolFlowRate_table_child,
    ads_Prop_IFluid_PressureJump_VolFlowRate_table_parent
};

/** Enum with record members. */
enum ads_Prop_IFluid_RadialVelocity_ConstantMembersEnm
{
    ads_Prop_IFluid_RadialVelocity_Constant_radialVelocity
};

/** Enum with association roles. */
enum ads_Prop_IFluid_RadialVelocity_Polynomial_tableRolesEnm
{
    ads_Prop_IFluid_RadialVelocity_Polynomial_table_child,
    ads_Prop_IFluid_RadialVelocity_Polynomial_table_parent
};

/** Enum with record members. */
enum ads_Prop_IFluid_TangentialVelocity_ConstantMembersEnm
{
    ads_Prop_IFluid_TangentialVelocity_Constant_tangentialVelocity
};

/** Enum with association roles. */
enum ads_Prop_IFluid_TangentialVelocity_Polynomial_tableRolesEnm
{
    ads_Prop_IFluid_TangentialVelocity_Polynomial_table_child,
    ads_Prop_IFluid_TangentialVelocity_Polynomial_table_parent
};

#endif
