//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreConnectorDofC_h
#define ads_CoreConnectorDofC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment ConnectorDof of the latest level of form Core */

#define ads_Prop_CMec_Failure_releaseComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorDofFragment, 0))

#define ads_Prop_CMec_Friction_UserDefined_contactForceCDCComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorDofFragment, 1))

#define ads_Prop_CMec_Friction_UserDefined_contactForceDofComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorDofFragment, 2))

#define ads_Prop_CMec_Friction_UserDefined_slipDirectionComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorDofFragment, 3))

#define ads_Prop_CMec_Lock_lockedComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorDofFragment, 4))

#define ads_Prop_CMec_component (ads_CoreFragmentTypeIndex(ads_CoreConnectorDofFragment, 5))

/** Enum with association roles. */
enum ads_Prop_CMec_Failure_releaseComponentRolesEnm
{
    ads_Prop_CMec_Failure_releaseComponent_referent,
    ads_Prop_CMec_Failure_releaseComponent_referrer
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_UserDefined_contactForceCDCComponentRolesEnm
{
    ads_Prop_CMec_Friction_UserDefined_contactForceCDCComponent_referent,
    ads_Prop_CMec_Friction_UserDefined_contactForceCDCComponent_referrer
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_UserDefined_contactForceDofComponentRolesEnm
{
    ads_Prop_CMec_Friction_UserDefined_contactForceDofComponent_referent,
    ads_Prop_CMec_Friction_UserDefined_contactForceDofComponent_referrer
};

/** Enum with association roles. */
enum ads_Prop_CMec_Friction_UserDefined_slipDirectionComponentRolesEnm
{
    ads_Prop_CMec_Friction_UserDefined_slipDirectionComponent_referent,
    ads_Prop_CMec_Friction_UserDefined_slipDirectionComponent_referrer
};

/** Enum with association roles. */
enum ads_Prop_CMec_Lock_lockedComponentRolesEnm
{
    ads_Prop_CMec_Lock_lockedComponent_referent,
    ads_Prop_CMec_Lock_lockedComponent_referrer
};

/** Enum with association roles. */
enum ads_Prop_CMec_componentRolesEnm
{
    ads_Prop_CMec_component_referent,
    ads_Prop_CMec_component_referrer
};

#endif
