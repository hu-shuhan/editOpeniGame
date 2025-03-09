//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyHighLevelC_h
#define ads_CorePropertyHighLevelC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyHighLevel of the latest level of form Core */

#define ads_Prop_BGeneral (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 0))

#define ads_Prop_BMec (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 1))

#define ads_Prop_CGeneral (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 2))

#define ads_Prop_CMec (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 3))

#define ads_Prop_CThermal (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 4))

#define ads_Prop_EMec (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 5))

#define ads_Prop_General (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 6))

#define ads_Prop_IDiffusion (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 7))

#define ads_Prop_IElectric (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 8))

#define ads_Prop_IGeneral (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 9))

#define ads_Prop_IMec (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 10))

#define ads_Prop_IPoreFluid (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 11))

#define ads_Prop_IThermal (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 12))

#define ads_Prop_MAcoustic (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 13))

#define ads_Prop_MDiffusion (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 14))

#define ads_Prop_MElectric (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 15))

#define ads_Prop_MGeneral (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 16))

#define ads_Prop_MMec (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 17))

#define ads_Prop_MPiezoelectric (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 18))

#define ads_Prop_MPoreFluid (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 19))

#define ads_Prop_MThermal (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 20))

#define ads_Prop_SGeneral (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 21))

#define ads_Prop_SMec (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 22))

#define ads_Property (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 23))

#define ads_Property_subproperties (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 24))

#define ads_Property_subroutine (ads_CoreFragmentTypeIndex(ads_CorePropertyHighLevelFragment, 25))

/** Enum with association roles. */
enum ads_Property_subpropertiesRolesEnm
{
    ads_Property_subproperties_child,
    ads_Property_subproperties_parent
};

/** Enum with association roles. */
enum ads_Property_subroutineRolesEnm
{
    ads_Property_subroutine_referent,
    ads_Property_subroutine_referrer
};

#endif
