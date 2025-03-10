//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreStepIMC_h
#define ads_CoreStepIMC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment StepIM of the latest level of form Core */

/** Captures the location at which molten plastic is injected into a mold. The plastic may be injected through a surface or through a point. */
#define ads_InjectionLocation (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 0))

/** Diameter for POINT injection locations. */
#define ads_InjectionLocation_diameter (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 1))

/** Injection Location Normal. */
#define ads_InjectionLocation_normal (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 2))

/** Injection Location Position. */
#define ads_InjectionLocation_position (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 3))

/** For the SURFACE type of InjectionLocation, the region must be a Surface. For the POINT type, the region must be a Region that has a DiscreteRegion with a CSet consisting of a single node. */
#define ads_InjectionLocation_region (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 4))

/** Intermediate base type for Injection Molding steps. */
#define ads_Step_Gen_IM (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 5))

/** Injection Molding Cooling step */
#define ads_Step_Gen_IM_Cool (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 6))

/** Injection Molding Fill step */
#define ads_Step_Gen_IM_Fill (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 7))

/** Injection Molding Pack step */
#define ads_Step_Gen_IM_Pack (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 8))

/** Injection Molding Warp step */
#define ads_Step_Gen_IM_Warp (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 9))

/** Used to capture the input fields for any Injection Molding step. The different fields are distinguished via the string key, e.g. "MachineMeltTemperature". Most input fields are likely to be of type Field_Scalar, but the client code should be designed to handle any subtype of Field, e.g. Field_Vector, MeshedField, etc. */
#define ads_Step_Gen_IM_inputFields (ads_CoreFragmentTypeIndex(ads_CoreStepIMFragment, 10))

/** 
Enum with record members. */
enum ads_InjectionLocationMembersEnm
{
    ads_InjectionLocation_type
};

enum ads_InjectionLocation_typeEnm
{
    ads_InjectionLocation_type_POINT,
    ads_InjectionLocation_type_SURFACE
};

/** 
Enum with association roles. */
enum ads_InjectionLocation_diameterRolesEnm
{
    ads_InjectionLocation_diameter_child,
    ads_InjectionLocation_diameter_parent
};

/** 
Enum with association roles. */
enum ads_InjectionLocation_normalRolesEnm
{
    ads_InjectionLocation_normal_child,
    ads_InjectionLocation_normal_parent
};

/** 
Enum with association roles. */
enum ads_InjectionLocation_positionRolesEnm
{
    ads_InjectionLocation_position_child,
    ads_InjectionLocation_position_parent
};

/** 
Enum with association roles. */
enum ads_InjectionLocation_regionRolesEnm
{
    ads_InjectionLocation_region_referent,
    ads_InjectionLocation_region_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_IMMembersEnm
{
    ads_Step_Gen_IM_designSensitivity,
    ads_Step_Gen_IM_dsa,
    ads_Step_Gen_IM_beginningTime
};

enum ads_Step_Gen_IM_designSensitivityEnm
{
    ads_Step_Gen_IM_designSensitivity_ADJOINT,
    ads_Step_Gen_IM_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_IM_CoolMembersEnm
{
    ads_Step_Gen_IM_Cool_designSensitivity,
    ads_Step_Gen_IM_Cool_dsa,
    ads_Step_Gen_IM_Cool_beginningTime
};

enum ads_Step_Gen_IM_Cool_designSensitivityEnm
{
    ads_Step_Gen_IM_Cool_designSensitivity_ADJOINT,
    ads_Step_Gen_IM_Cool_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_IM_FillMembersEnm
{
    ads_Step_Gen_IM_Fill_designSensitivity,
    ads_Step_Gen_IM_Fill_dsa,
    ads_Step_Gen_IM_Fill_beginningTime
};

enum ads_Step_Gen_IM_Fill_designSensitivityEnm
{
    ads_Step_Gen_IM_Fill_designSensitivity_ADJOINT,
    ads_Step_Gen_IM_Fill_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_IM_PackMembersEnm
{
    ads_Step_Gen_IM_Pack_designSensitivity,
    ads_Step_Gen_IM_Pack_dsa,
    ads_Step_Gen_IM_Pack_beginningTime
};

enum ads_Step_Gen_IM_Pack_designSensitivityEnm
{
    ads_Step_Gen_IM_Pack_designSensitivity_ADJOINT,
    ads_Step_Gen_IM_Pack_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_IM_WarpMembersEnm
{
    ads_Step_Gen_IM_Warp_designSensitivity,
    ads_Step_Gen_IM_Warp_dsa,
    ads_Step_Gen_IM_Warp_beginningTime
};

enum ads_Step_Gen_IM_Warp_designSensitivityEnm
{
    ads_Step_Gen_IM_Warp_designSensitivity_ADJOINT,
    ads_Step_Gen_IM_Warp_designSensitivity_NONE
};

/** 
Enum with association roles. */
enum ads_Step_Gen_IM_inputFieldsRolesEnm
{
    ads_Step_Gen_IM_inputFields_child,
    ads_Step_Gen_IM_inputFields_parent
};

#endif
