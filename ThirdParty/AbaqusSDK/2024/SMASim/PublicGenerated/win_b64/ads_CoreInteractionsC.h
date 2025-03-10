//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreInteractionsC_h
#define ads_CoreInteractionsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Interactions of the latest level of form Core */

/** A cavity definition used for thermal radiation heat transfer. This can currently only be used in conjunction with element-based surfaces. */
#define ads_Cavity (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 0))

/** An association between the cavity and the surfaces that make it up. Note: The association should use the Surface type instead Region types. However using the more abstract Region type allows to use composite regions in that association. The SIM Reader has to ensure dynamically that the Region a Surface. */
#define ads_Cavity_surfaces (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 1))

#define ads_FluidCavityMixture (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 2))

#define ads_FluidCavityMixture_fraction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 3))

#define ads_FluidCavityMixture_property (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 4))

/** Base class for all interactions */
#define ads_Interaction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 5))

/** This collection holds interactions to allow the creation of interaction sets for use in output requests and field distributions */
#define ads_InteractionCollection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 6))

/** Base class for all interaction properties */
#define ads_InteractionProperty (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 7))

#define ads_InteractionProperty_properties (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 8))

#define ads_InteractionRelay (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 9))

/** This record is used for Change Friction Interaction. */
#define ads_InteractionRelay_ChangeFriction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 10))

#define ads_InteractionRelay_ChangeFriction_newProperty (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 11))

#define ads_InteractionRelay_FluidExchange (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 12))

#define ads_InteractionRelay_interaction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 13))

#define ads_InteractionRelay_relayModulations (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 14))

/** An interaction linking two surfaces in a CFD simulation */
#define ads_Interaction_CFD (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 15))

/** CFD Conduction Baffle */
#define ads_Interaction_CFD_ConductionBaffle (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 16))

/** A contact interaction linking two surfaces in a CFD simulation */
#define ads_Interaction_CFD_Contact (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 17))

/** A CFD fluid interaction */
#define ads_Interaction_CFD_Fluid (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 18))

#define ads_Interaction_CFD_Fluid_Blower (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 19))

#define ads_Interaction_CFD_Fluid_Blower_table (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 20))

/** CFD Fluid FAN */
#define ads_Interaction_CFD_Fluid_FAN (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 21))

/** CFD Fluid FAN3D */
#define ads_Interaction_CFD_Fluid_FAN3D (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 22))

/** Inlet center. */
#define ads_Interaction_CFD_Fluid_FAN3D_inletCenter (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 23))

/** Outlet center. */
#define ads_Interaction_CFD_Fluid_FAN3D_outletCenter (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 24))

/** Fan center. */
#define ads_Interaction_CFD_Fluid_FAN_center (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 25))

/** CFD Fluid Multiple Rotating Frames (MRF) */
#define ads_Interaction_CFD_Fluid_MRF (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 26))

/** A CFD periodic interaction */
#define ads_Interaction_CFD_Periodic (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 27))

/** A CFD Porous Baffle */
#define ads_Interaction_CFD_PorousBaffle (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 28))

/** A CFD thermal interaction */
#define ads_Interaction_CFD_ThermalContact (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 29))

#define ads_Interaction_ChangeFriction (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 30))

#define ads_Interaction_ChangeFriction_newProperty (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 31))

#define ads_Interaction_ChangeFriction_oldProperty (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 32))

/** This record is used for concentrated Film Condition Interaction. */
#define ads_Interaction_ConcentratedFilmCondition (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 33))

/** This record is used for surface Film Condition Interaction. */
#define ads_Interaction_FilmCondition (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 34))

/** A surface based fluid cavityinteraction */
#define ads_Interaction_FluidCavity (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 35))

#define ads_Interaction_FluidCavity_mixtures (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 36))

/** This record is used for Fluid Exchange Interaction. */
#define ads_Interaction_FluidExchange (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 37))

/** The user defined effective area and is valid only when effectiveAreaSpec is set to SPECIFIED. */
#define ads_Interaction_FluidExchange_effectiveArea (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 38))

/** The fluid exchange constants to define the effective area for fluid exchange. */
#define ads_Interaction_FluidExchange_exchangeConstants (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 39))

/** The surface on the fluid cavity over which fluid and/or heat energy may be exchanged. Note: The association should use the Surface type instead Region types. However using the more abstract Region type allows to use composite regions in that association. The SIM Reader has to ensure dynamically that the Region a Surface. */
#define ads_Interaction_FluidExchange_surface (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 40))

/** This record is used to provide seepage coefficients and sink pore pressures to control pore fluid flow normal to the surface. */
#define ads_Interaction_FluidFlow (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 41))

/** This record is used for Impedance Interaction. */
#define ads_Interaction_Impedance (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 42))

/** The semimajor axis, a, of the ellipse or prolate spheroid defining the surface. a is 1/2 of the maximum distance between two points on the ellipse or spheroid, analogous to the radius of a circle or sphere. */
#define ads_Interaction_Impedance_a (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 43))

/** The eccentricity of the ellipse or prolate spheroid. The eccentricity is the square root of one minus the square of the ratio of the minor axis, b, to the major axis. */
#define ads_Interaction_Impedance_eccentricity (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 44))

/** The radiating surface center. */
#define ads_Interaction_Impedance_radiatingSurfaceCenter (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 45))

/** The direction cosines of the major axis of the radiating surface. */
#define ads_Interaction_Impedance_radiatingSurfaceDirection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 46))

/** The radius of the circle or sphere defining the absorbing boundary surface. */
#define ads_Interaction_Impedance_radius (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 47))

/** This record is used for surface radiation. Base class composition to the fields should be used for storing temperature and emissivity. Field type should be used to identify temperature or emissivity. */
#define ads_Interaction_SurfaceRadiation (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 48))

#define ads_Interaction_fields (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 49))

#define ads_Interaction_orientation (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 50))

#define ads_Interaction_property (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 51))

#define ads_Interaction_regions (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 52))

#define ads_Interaction_subroutine (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 53))

/** The list of cavities */
#define ads_Model_cavity (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 54))

#define ads_Model_interactionCollection (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 55))

#define ads_Model_interactionProperties (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 56))

#define ads_Model_interactions (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 57))

/** The order is important only in case of predefined fields where last one wins rule applies in case of overlaping nodes/elements. */
#define ads_Task_interactions (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 58))

/** Data to define fan curve table */
#define ads_fanCurveBlowerTable (ads_CoreFragmentTypeIndex(ads_CoreInteractionsFragment, 59))

/** 
Enum with record members. */
enum ads_CavityMembersEnm
{
    ads_Cavity_ambientTemperature,
    ads_Cavity_closed
};

/** 
Enum with association roles. */
enum ads_Cavity_surfacesRolesEnm
{
    ads_Cavity_surfaces_referent,
    ads_Cavity_surfaces_referrer
};

/** Enum with association roles. */
enum ads_FluidCavityMixture_fractionRolesEnm
{
    ads_FluidCavityMixture_fraction_child,
    ads_FluidCavityMixture_fraction_parent
};

/** Enum with association roles. */
enum ads_FluidCavityMixture_propertyRolesEnm
{
    ads_FluidCavityMixture_property_referent,
    ads_FluidCavityMixture_property_referrer
};

/** 
Enum with record members. */
enum ads_InteractionMembersEnm
{
    ads_Interaction_autoPropagate
};

/** Enum with association roles. */
enum ads_InteractionProperty_propertiesRolesEnm
{
    ads_InteractionProperty_properties_child,
    ads_InteractionProperty_properties_parent
};

/** Enum with record members. */
enum ads_InteractionRelayMembersEnm
{
    ads_InteractionRelay_autoPropogated
};

/** 
Enum with record members. */
enum ads_InteractionRelay_ChangeFrictionMembersEnm
{
    ads_InteractionRelay_ChangeFriction_autoPropogated,
    ads_InteractionRelay_ChangeFriction_reset
};

/** Enum with association roles. */
enum ads_InteractionRelay_ChangeFriction_newPropertyRolesEnm
{
    ads_InteractionRelay_ChangeFriction_newProperty_child,
    ads_InteractionRelay_ChangeFriction_newProperty_parent
};

/** Enum with record members. */
enum ads_InteractionRelay_FluidExchangeMembersEnm
{
    ads_InteractionRelay_FluidExchange_autoPropogated,
    ads_InteractionRelay_FluidExchange_blockage,
    ads_InteractionRelay_FluidExchange_deltaLeakageArea,
    ads_InteractionRelay_FluidExchange_outflowOnly
};

/** Enum with association roles. */
enum ads_InteractionRelay_interactionRolesEnm
{
    ads_InteractionRelay_interaction_referent,
    ads_InteractionRelay_interaction_referrer
};

/** Enum with association roles. */
enum ads_InteractionRelay_relayModulationsRolesEnm
{
    ads_InteractionRelay_relayModulations_child,
    ads_InteractionRelay_relayModulations_parent
};

/** 
Enum with record members. */
enum ads_Interaction_CFDMembersEnm
{
    ads_Interaction_CFD_autoPropagate,
    ads_Interaction_CFD_absoluteNormalTolerance,
    ads_Interaction_CFD_contactType,
    ads_Interaction_CFD_isConformal,
    ads_Interaction_CFD_isExclusive,
    ads_Interaction_CFD_isSurfaceBasedInterface,
    ads_Interaction_CFD_normalAngleTolerance,
    ads_Interaction_CFD_relativeNormalTolerance
};

enum ads_Interaction_CFD_contactTypeEnm
{
    ads_Interaction_CFD_contactType_AUTO,
    ads_Interaction_CFD_contactType_REGION,
    ads_Interaction_CFD_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_ConductionBaffleMembersEnm
{
    ads_Interaction_CFD_ConductionBaffle_autoPropagate,
    ads_Interaction_CFD_ConductionBaffle_absoluteNormalTolerance,
    ads_Interaction_CFD_ConductionBaffle_contactType,
    ads_Interaction_CFD_ConductionBaffle_isConformal,
    ads_Interaction_CFD_ConductionBaffle_isExclusive,
    ads_Interaction_CFD_ConductionBaffle_isSurfaceBasedInterface,
    ads_Interaction_CFD_ConductionBaffle_normalAngleTolerance,
    ads_Interaction_CFD_ConductionBaffle_relativeNormalTolerance
};

enum ads_Interaction_CFD_ConductionBaffle_contactTypeEnm
{
    ads_Interaction_CFD_ConductionBaffle_contactType_AUTO,
    ads_Interaction_CFD_ConductionBaffle_contactType_REGION,
    ads_Interaction_CFD_ConductionBaffle_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_ContactMembersEnm
{
    ads_Interaction_CFD_Contact_autoPropagate,
    ads_Interaction_CFD_Contact_absoluteNormalTolerance,
    ads_Interaction_CFD_Contact_contactType,
    ads_Interaction_CFD_Contact_isConformal,
    ads_Interaction_CFD_Contact_isExclusive,
    ads_Interaction_CFD_Contact_isSurfaceBasedInterface,
    ads_Interaction_CFD_Contact_normalAngleTolerance,
    ads_Interaction_CFD_Contact_relativeNormalTolerance
};

enum ads_Interaction_CFD_Contact_contactTypeEnm
{
    ads_Interaction_CFD_Contact_contactType_AUTO,
    ads_Interaction_CFD_Contact_contactType_REGION,
    ads_Interaction_CFD_Contact_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_FluidMembersEnm
{
    ads_Interaction_CFD_Fluid_autoPropagate,
    ads_Interaction_CFD_Fluid_absoluteNormalTolerance,
    ads_Interaction_CFD_Fluid_contactType,
    ads_Interaction_CFD_Fluid_isConformal,
    ads_Interaction_CFD_Fluid_isExclusive,
    ads_Interaction_CFD_Fluid_isSurfaceBasedInterface,
    ads_Interaction_CFD_Fluid_normalAngleTolerance,
    ads_Interaction_CFD_Fluid_relativeNormalTolerance
};

enum ads_Interaction_CFD_Fluid_contactTypeEnm
{
    ads_Interaction_CFD_Fluid_contactType_AUTO,
    ads_Interaction_CFD_Fluid_contactType_REGION,
    ads_Interaction_CFD_Fluid_contactType_SURFACE
};

/** Enum with record members. */
enum ads_Interaction_CFD_Fluid_BlowerMembersEnm
{
    ads_Interaction_CFD_Fluid_Blower_autoPropagate,
    ads_Interaction_CFD_Fluid_Blower_absoluteNormalTolerance,
    ads_Interaction_CFD_Fluid_Blower_contactType,
    ads_Interaction_CFD_Fluid_Blower_isConformal,
    ads_Interaction_CFD_Fluid_Blower_isExclusive,
    ads_Interaction_CFD_Fluid_Blower_isSurfaceBasedInterface,
    ads_Interaction_CFD_Fluid_Blower_normalAngleTolerance,
    ads_Interaction_CFD_Fluid_Blower_relativeNormalTolerance,
    ads_Interaction_CFD_Fluid_Blower_operatingRPM
};

enum ads_Interaction_CFD_Fluid_Blower_contactTypeEnm
{
    ads_Interaction_CFD_Fluid_Blower_contactType_AUTO,
    ads_Interaction_CFD_Fluid_Blower_contactType_REGION,
    ads_Interaction_CFD_Fluid_Blower_contactType_SURFACE
};

/** Enum with association roles. */
enum ads_Interaction_CFD_Fluid_Blower_tableRolesEnm
{
    ads_Interaction_CFD_Fluid_Blower_table_child,
    ads_Interaction_CFD_Fluid_Blower_table_parent
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_Fluid_FANMembersEnm
{
    ads_Interaction_CFD_Fluid_FAN_autoPropagate,
    ads_Interaction_CFD_Fluid_FAN_absoluteNormalTolerance,
    ads_Interaction_CFD_Fluid_FAN_contactType,
    ads_Interaction_CFD_Fluid_FAN_isConformal,
    ads_Interaction_CFD_Fluid_FAN_isExclusive,
    ads_Interaction_CFD_Fluid_FAN_isSurfaceBasedInterface,
    ads_Interaction_CFD_Fluid_FAN_normalAngleTolerance,
    ads_Interaction_CFD_Fluid_FAN_relativeNormalTolerance,
    ads_Interaction_CFD_Fluid_FAN_hubRadius
};

enum ads_Interaction_CFD_Fluid_FAN_contactTypeEnm
{
    ads_Interaction_CFD_Fluid_FAN_contactType_AUTO,
    ads_Interaction_CFD_Fluid_FAN_contactType_REGION,
    ads_Interaction_CFD_Fluid_FAN_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_Fluid_FAN3DMembersEnm
{
    ads_Interaction_CFD_Fluid_FAN3D_autoPropagate,
    ads_Interaction_CFD_Fluid_FAN3D_absoluteNormalTolerance,
    ads_Interaction_CFD_Fluid_FAN3D_contactType,
    ads_Interaction_CFD_Fluid_FAN3D_isConformal,
    ads_Interaction_CFD_Fluid_FAN3D_isExclusive,
    ads_Interaction_CFD_Fluid_FAN3D_isSurfaceBasedInterface,
    ads_Interaction_CFD_Fluid_FAN3D_normalAngleTolerance,
    ads_Interaction_CFD_Fluid_FAN3D_relativeNormalTolerance,
    ads_Interaction_CFD_Fluid_FAN3D_hubRadius
};

enum ads_Interaction_CFD_Fluid_FAN3D_contactTypeEnm
{
    ads_Interaction_CFD_Fluid_FAN3D_contactType_AUTO,
    ads_Interaction_CFD_Fluid_FAN3D_contactType_REGION,
    ads_Interaction_CFD_Fluid_FAN3D_contactType_SURFACE
};

/** 
Enum with association roles. */
enum ads_Interaction_CFD_Fluid_FAN3D_inletCenterRolesEnm
{
    ads_Interaction_CFD_Fluid_FAN3D_inletCenter_child,
    ads_Interaction_CFD_Fluid_FAN3D_inletCenter_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_CFD_Fluid_FAN3D_outletCenterRolesEnm
{
    ads_Interaction_CFD_Fluid_FAN3D_outletCenter_child,
    ads_Interaction_CFD_Fluid_FAN3D_outletCenter_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_CFD_Fluid_FAN_centerRolesEnm
{
    ads_Interaction_CFD_Fluid_FAN_center_child,
    ads_Interaction_CFD_Fluid_FAN_center_parent
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_Fluid_MRFMembersEnm
{
    ads_Interaction_CFD_Fluid_MRF_autoPropagate,
    ads_Interaction_CFD_Fluid_MRF_absoluteNormalTolerance,
    ads_Interaction_CFD_Fluid_MRF_contactType,
    ads_Interaction_CFD_Fluid_MRF_isConformal,
    ads_Interaction_CFD_Fluid_MRF_isExclusive,
    ads_Interaction_CFD_Fluid_MRF_isSurfaceBasedInterface,
    ads_Interaction_CFD_Fluid_MRF_normalAngleTolerance,
    ads_Interaction_CFD_Fluid_MRF_relativeNormalTolerance
};

enum ads_Interaction_CFD_Fluid_MRF_contactTypeEnm
{
    ads_Interaction_CFD_Fluid_MRF_contactType_AUTO,
    ads_Interaction_CFD_Fluid_MRF_contactType_REGION,
    ads_Interaction_CFD_Fluid_MRF_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_PeriodicMembersEnm
{
    ads_Interaction_CFD_Periodic_autoPropagate,
    ads_Interaction_CFD_Periodic_absoluteNormalTolerance,
    ads_Interaction_CFD_Periodic_contactType,
    ads_Interaction_CFD_Periodic_isConformal,
    ads_Interaction_CFD_Periodic_isExclusive,
    ads_Interaction_CFD_Periodic_isSurfaceBasedInterface,
    ads_Interaction_CFD_Periodic_normalAngleTolerance,
    ads_Interaction_CFD_Periodic_relativeNormalTolerance
};

enum ads_Interaction_CFD_Periodic_contactTypeEnm
{
    ads_Interaction_CFD_Periodic_contactType_AUTO,
    ads_Interaction_CFD_Periodic_contactType_REGION,
    ads_Interaction_CFD_Periodic_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_PorousBaffleMembersEnm
{
    ads_Interaction_CFD_PorousBaffle_autoPropagate,
    ads_Interaction_CFD_PorousBaffle_absoluteNormalTolerance,
    ads_Interaction_CFD_PorousBaffle_contactType,
    ads_Interaction_CFD_PorousBaffle_isConformal,
    ads_Interaction_CFD_PorousBaffle_isExclusive,
    ads_Interaction_CFD_PorousBaffle_isSurfaceBasedInterface,
    ads_Interaction_CFD_PorousBaffle_normalAngleTolerance,
    ads_Interaction_CFD_PorousBaffle_relativeNormalTolerance
};

enum ads_Interaction_CFD_PorousBaffle_contactTypeEnm
{
    ads_Interaction_CFD_PorousBaffle_contactType_AUTO,
    ads_Interaction_CFD_PorousBaffle_contactType_REGION,
    ads_Interaction_CFD_PorousBaffle_contactType_SURFACE
};

/** 
Enum with record members. */
enum ads_Interaction_CFD_ThermalContactMembersEnm
{
    ads_Interaction_CFD_ThermalContact_autoPropagate,
    ads_Interaction_CFD_ThermalContact_absoluteNormalTolerance,
    ads_Interaction_CFD_ThermalContact_contactType,
    ads_Interaction_CFD_ThermalContact_isConformal,
    ads_Interaction_CFD_ThermalContact_isExclusive,
    ads_Interaction_CFD_ThermalContact_isSurfaceBasedInterface,
    ads_Interaction_CFD_ThermalContact_normalAngleTolerance,
    ads_Interaction_CFD_ThermalContact_relativeNormalTolerance
};

enum ads_Interaction_CFD_ThermalContact_contactTypeEnm
{
    ads_Interaction_CFD_ThermalContact_contactType_AUTO,
    ads_Interaction_CFD_ThermalContact_contactType_REGION,
    ads_Interaction_CFD_ThermalContact_contactType_SURFACE
};

/** Enum with record members. */
enum ads_Interaction_ChangeFrictionMembersEnm
{
    ads_Interaction_ChangeFriction_autoPropagate
};

/** Enum with association roles. */
enum ads_Interaction_ChangeFriction_newPropertyRolesEnm
{
    ads_Interaction_ChangeFriction_newProperty_child,
    ads_Interaction_ChangeFriction_newProperty_parent
};

/** Enum with association roles. */
enum ads_Interaction_ChangeFriction_oldPropertyRolesEnm
{
    ads_Interaction_ChangeFriction_oldProperty_referent,
    ads_Interaction_ChangeFriction_oldProperty_referrer
};

/** 
Enum with record members. */
enum ads_Interaction_ConcentratedFilmConditionMembersEnm
{
    ads_Interaction_ConcentratedFilmCondition_autoPropagate
};

/** 
Enum with record members. */
enum ads_Interaction_FilmConditionMembersEnm
{
    ads_Interaction_FilmCondition_autoPropagate,
    ads_Interaction_FilmCondition_freeSurface,
    ads_Interaction_FilmCondition_usub
};

/** 
Enum with record members. */
enum ads_Interaction_FluidCavityMembersEnm
{
    ads_Interaction_FluidCavity_autoPropagate,
    ads_Interaction_FluidCavity_addedVolume,
    ads_Interaction_FluidCavity_adiabatic,
    ads_Interaction_FluidCavity_ambientPressure,
    ads_Interaction_FluidCavity_ambientTemperature,
    ads_Interaction_FluidCavity_checkNormals,
    ads_Interaction_FluidCavity_initialVolumeIsMinimumVolume,
    ads_Interaction_FluidCavity_minimumVolume,
    ads_Interaction_FluidCavity_switchTime,
    ads_Interaction_FluidCavity_thickness
};

/** Enum with association roles. */
enum ads_Interaction_FluidCavity_mixturesRolesEnm
{
    ads_Interaction_FluidCavity_mixtures_child,
    ads_Interaction_FluidCavity_mixtures_parent
};

/** 
Enum with record members. */
enum ads_Interaction_FluidExchangeMembersEnm
{
    ads_Interaction_FluidExchange_autoPropagate,
    ads_Interaction_FluidExchange_cavityPressure,
    ads_Interaction_FluidExchange_effectiveAreaSpec,
    ads_Interaction_FluidExchange_maxRuptureAreaRatio,
    ads_Interaction_FluidExchange_numConstants,
    ads_Interaction_FluidExchange_rupture
};

enum ads_Interaction_FluidExchange_cavityPressureEnm
{
    ads_Interaction_FluidExchange_cavityPressure_NONE,
    ads_Interaction_FluidExchange_cavityPressure_PERIMETER,
    ads_Interaction_FluidExchange_cavityPressure_SURFACE
};

enum ads_Interaction_FluidExchange_effectiveAreaSpecEnm
{
    ads_Interaction_FluidExchange_effectiveAreaSpec_SOLVER_DEFAULT,
    ads_Interaction_FluidExchange_effectiveAreaSpec_SPECIFIED,
    ads_Interaction_FluidExchange_effectiveAreaSpec_USER
};

/** 
Enum with association roles. */
enum ads_Interaction_FluidExchange_effectiveAreaRolesEnm
{
    ads_Interaction_FluidExchange_effectiveArea_child,
    ads_Interaction_FluidExchange_effectiveArea_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_FluidExchange_exchangeConstantsRolesEnm
{
    ads_Interaction_FluidExchange_exchangeConstants_child,
    ads_Interaction_FluidExchange_exchangeConstants_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_FluidExchange_surfaceRolesEnm
{
    ads_Interaction_FluidExchange_surface_referent,
    ads_Interaction_FluidExchange_surface_referrer
};

/** 
Enum with record members. */
enum ads_Interaction_FluidFlowMembersEnm
{
    ads_Interaction_FluidFlow_autoPropagate,
    ads_Interaction_FluidFlow_freeSurface,
    ads_Interaction_FluidFlow_seepageType
};

enum ads_Interaction_FluidFlow_seepageTypeEnm
{
    ads_Interaction_FluidFlow_seepageType_DRAINAGE_ONLY,
    ads_Interaction_FluidFlow_seepageType_NONE,
    ads_Interaction_FluidFlow_seepageType_NONUNIFORM,
    ads_Interaction_FluidFlow_seepageType_UNIFORM
};

/** 
Enum with record members. */
enum ads_Interaction_ImpedanceMembersEnm
{
    ads_Interaction_Impedance_autoPropagate,
    ads_Interaction_Impedance_nonReflectingType
};

enum ads_Interaction_Impedance_nonReflectingTypeEnm
{
    ads_Interaction_Impedance_nonReflectingType_CIRCULAR,
    ads_Interaction_Impedance_nonReflectingType_ELLIPTICAL,
    ads_Interaction_Impedance_nonReflectingType_IMPROVED,
    ads_Interaction_Impedance_nonReflectingType_NONE,
    ads_Interaction_Impedance_nonReflectingType_PLANAR,
    ads_Interaction_Impedance_nonReflectingType_PROLATE_SPHEROIDAL,
    ads_Interaction_Impedance_nonReflectingType_SPHERICAL
};

/** 
Enum with association roles. */
enum ads_Interaction_Impedance_aRolesEnm
{
    ads_Interaction_Impedance_a_child,
    ads_Interaction_Impedance_a_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Impedance_eccentricityRolesEnm
{
    ads_Interaction_Impedance_eccentricity_child,
    ads_Interaction_Impedance_eccentricity_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Impedance_radiatingSurfaceCenterRolesEnm
{
    ads_Interaction_Impedance_radiatingSurfaceCenter_child,
    ads_Interaction_Impedance_radiatingSurfaceCenter_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Impedance_radiatingSurfaceDirectionRolesEnm
{
    ads_Interaction_Impedance_radiatingSurfaceDirection_child,
    ads_Interaction_Impedance_radiatingSurfaceDirection_parent
};

/** 
Enum with association roles. */
enum ads_Interaction_Impedance_radiusRolesEnm
{
    ads_Interaction_Impedance_radius_child,
    ads_Interaction_Impedance_radius_parent
};

/** 
Enum with record members. */
enum ads_Interaction_SurfaceRadiationMembersEnm
{
    ads_Interaction_SurfaceRadiation_autoPropagate,
    ads_Interaction_SurfaceRadiation_freeSurface,
    ads_Interaction_SurfaceRadiation_type
};

enum ads_Interaction_SurfaceRadiation_typeEnm
{
    ads_Interaction_SurfaceRadiation_type_AMBIENT,
    ads_Interaction_SurfaceRadiation_type_CAVITY
};

/** Enum with association roles. */
enum ads_Interaction_fieldsRolesEnm
{
    ads_Interaction_fields_child,
    ads_Interaction_fields_parent
};

/** Enum with association roles. */
enum ads_Interaction_orientationRolesEnm
{
    ads_Interaction_orientation_referent,
    ads_Interaction_orientation_referrer
};

/** Enum with association roles. */
enum ads_Interaction_propertyRolesEnm
{
    ads_Interaction_property_referent,
    ads_Interaction_property_referrer
};

/** Enum with association roles. */
enum ads_Interaction_regionsRolesEnm
{
    ads_Interaction_regions_referent,
    ads_Interaction_regions_referrer
};

/** Enum with association roles. */
enum ads_Interaction_subroutineRolesEnm
{
    ads_Interaction_subroutine_referent,
    ads_Interaction_subroutine_referrer
};

/** 
Enum with association roles. */
enum ads_Model_cavityRolesEnm
{
    ads_Model_cavity_child,
    ads_Model_cavity_parent
};

/** Enum with association roles. */
enum ads_Model_interactionCollectionRolesEnm
{
    ads_Model_interactionCollection_child,
    ads_Model_interactionCollection_parent
};

/** Enum with association roles. */
enum ads_Model_interactionPropertiesRolesEnm
{
    ads_Model_interactionProperties_child,
    ads_Model_interactionProperties_parent
};

/** Enum with association roles. */
enum ads_Model_interactionsRolesEnm
{
    ads_Model_interactions_child,
    ads_Model_interactions_parent
};

/** 
Enum with association roles. */
enum ads_Task_interactionsRolesEnm
{
    ads_Task_interactions_child,
    ads_Task_interactions_parent
};

#endif
