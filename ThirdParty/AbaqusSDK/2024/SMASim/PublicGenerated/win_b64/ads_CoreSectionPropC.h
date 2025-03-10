//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreSectionPropC_h
#define ads_CoreSectionPropC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment SectionProp of the latest level of form Core */

#define ads_Prop_CGeneral_Mass (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 0))

#define ads_Prop_CGeneral_Mass_anisotropicMass (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 1))

#define ads_Prop_CGeneral_Mass_centerOfMassOffset (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 2))

#define ads_Prop_CGeneral_Mass_mass (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 3))

#define ads_Prop_CGeneral_Mass_rotaryInertia (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 4))

#define ads_Prop_SGeneral_Mass_Density_massDensity (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 5))

#define ads_Prop_SMec_User_userProperties (ads_CoreFragmentTypeIndex(ads_CoreSectionPropFragment, 6))

/** Enum with association roles. */
enum ads_Prop_CGeneral_Mass_anisotropicMassRolesEnm
{
    ads_Prop_CGeneral_Mass_anisotropicMass_child,
    ads_Prop_CGeneral_Mass_anisotropicMass_parent
};

/** Enum with association roles. */
enum ads_Prop_CGeneral_Mass_centerOfMassOffsetRolesEnm
{
    ads_Prop_CGeneral_Mass_centerOfMassOffset_child,
    ads_Prop_CGeneral_Mass_centerOfMassOffset_parent
};

/** Enum with association roles. */
enum ads_Prop_CGeneral_Mass_massRolesEnm
{
    ads_Prop_CGeneral_Mass_mass_child,
    ads_Prop_CGeneral_Mass_mass_parent
};

/** Enum with association roles. */
enum ads_Prop_CGeneral_Mass_rotaryInertiaRolesEnm
{
    ads_Prop_CGeneral_Mass_rotaryInertia_child,
    ads_Prop_CGeneral_Mass_rotaryInertia_parent
};

/** Enum with association roles. */
enum ads_Prop_SGeneral_Mass_Density_massDensityRolesEnm
{
    ads_Prop_SGeneral_Mass_Density_massDensity_child,
    ads_Prop_SGeneral_Mass_Density_massDensity_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_User_userPropertiesRolesEnm
{
    ads_Prop_SMec_User_userProperties_child,
    ads_Prop_SMec_User_userProperties_parent
};

#endif
