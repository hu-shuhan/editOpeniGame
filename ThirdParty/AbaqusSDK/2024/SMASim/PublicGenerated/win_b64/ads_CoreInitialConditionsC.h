//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreInitialConditionsC_h
#define ads_CoreInitialConditionsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment InitialConditions of the latest level of form Core */

#define ads_CFDImport (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 0))

#define ads_ContactSurfaces (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 1))

/** The first is secondary surface, and second is main surface. */
#define ads_ContactSurfaces_contactPairs (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 2))

#define ads_ContactSurfaces_nodeSet (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 3))

/** Base class for all intial conditions */
#define ads_InitCondition (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 4))

#define ads_InitCondition_Activation (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 5))

#define ads_InitCondition_CFD (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 6))

#define ads_InitCondition_CFD_Density (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 7))

#define ads_InitCondition_CFD_ElectricPotential (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 8))

#define ads_InitCondition_CFD_Enthalpy (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 9))

#define ads_InitCondition_CFD_FilmThickness (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 10))

#define ads_InitCondition_CFD_Pressure (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 11))

/** Multi-species initial fraction */
#define ads_InitCondition_CFD_SpeciesFraction (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 12))

#define ads_InitCondition_CFD_Temperature (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 13))

#define ads_InitCondition_CFD_TurbulentDissipation (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 14))

#define ads_InitCondition_CFD_TurbulentIntensity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 15))

#define ads_InitCondition_CFD_TurbulentKineticEnergy (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 16))

#define ads_InitCondition_CFD_TurbulentLengthScale (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 17))

#define ads_InitCondition_CFD_TurbulentProduction (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 18))

#define ads_InitCondition_CFD_TurbulentTimeScale (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 19))

#define ads_InitCondition_CFD_TurbulentVelocityRatio (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 20))

#define ads_InitCondition_CFD_TurbulentVelocityScale (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 21))

#define ads_InitCondition_CFD_TurbulentViscosity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 22))

#define ads_InitCondition_CFD_TurbulentViscosityRatio (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 23))

#define ads_InitCondition_CFD_Velocity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 24))

/** This reference relationship is used only in case of scalar field. In case of meshed field, component is used as one of the dimention of distribution. */
#define ads_InitCondition_CFD_Velocity_component (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 25))

#define ads_InitCondition_CFD_Volume (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 26))

#define ads_InitCondition_CFD_VolumeFraction (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 27))

#define ads_InitCondition_CFD_WaveInlet (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 28))

#define ads_InitCondition_CFD_fields (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 29))

#define ads_InitCondition_CFD_import (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 30))

#define ads_InitCondition_ClearanceContactPair (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 31))

#define ads_InitCondition_ClearanceContactPair_boltHalfThreadAngle (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 32))

#define ads_InitCondition_ClearanceContactPair_boltMajorThreadDiameter (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 33))

#define ads_InitCondition_ClearanceContactPair_boltMeanThreadDiameter (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 34))

#define ads_InitCondition_ClearanceContactPair_boltPitch (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 35))

#define ads_InitCondition_ClearanceContactPair_boltPointA (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 36))

#define ads_InitCondition_ClearanceContactPair_boltPointB (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 37))

#define ads_InitCondition_ClearanceContactPair_clearance (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 38))

#define ads_InitCondition_ClearanceContactPair_contactPairs (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 39))

#define ads_InitCondition_ClearanceContactPair_normal (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 40))

#define ads_InitCondition_Concentration (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 41))

/** To specify initial bonded contact conditions on part of the secondary surface identified by a node set in an Abaqus/Standard analysis. */
#define ads_InitCondition_Contact (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 42))

#define ads_InitCondition_Contact_contactSurfaces (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 43))

#define ads_InitCondition_Cure (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 44))

#define ads_InitCondition_FluidElectricPotential (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 45))

#define ads_InitCondition_InternalEnergy (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 46))

#define ads_InitCondition_IonConcentration (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 47))

#define ads_InitCondition_PorePressure (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 48))

#define ads_InitCondition_PorePressure_p2 (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 49))

#define ads_InitCondition_PorePressure_v1 (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 50))

#define ads_InitCondition_PorePressure_v2 (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 51))

#define ads_InitCondition_Porosity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 52))

#define ads_InitCondition_Pressure (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 53))

#define ads_InitCondition_PressureStress (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 54))

#define ads_InitCondition_Rebar (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 55))

#define ads_InitCondition_Rebar_Stress (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 56))

#define ads_InitCondition_Rebar_rebar (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 57))

#define ads_InitCondition_Rebar_rebars (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 58))

#define ads_InitCondition_RelativeDensity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 59))

/** Angular velocity is defined by link to the field from base class. inp2sim will create a new record for each pair of keyword data lines parsed. If this ever becomes a performance issue then a seperate schema to store bulkified data should be added. */
#define ads_InitCondition_RotatingVelocity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 60))

/** Nodes representing pointA and pointB on axis of rotation. This should be an ordered cset with first node representing pointA and second node representing pointB. */
#define ads_InitCondition_RotatingVelocity_nodesAB (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 61))

/** Coordinates of pointA on axis of rotation. This and pointB or reference to nodes should be present. */
#define ads_InitCondition_RotatingVelocity_pointA (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 62))

/** Coordinates of pointB on axis of rotation. This and pointA or reference to nodes should be present. */
#define ads_InitCondition_RotatingVelocity_pointB (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 63))

/** Translational velocity in terms of global components. */
#define ads_InitCondition_RotatingVelocity_translationalVelocity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 64))

#define ads_InitCondition_Saturation (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 65))

#define ads_InitCondition_SolidElectricPotential (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 66))

#define ads_InitCondition_SpudEmbedment (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 67))

#define ads_InitCondition_SpudPreload (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 68))

#define ads_InitCondition_Stress (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 69))

#define ads_InitCondition_Stress_Geostatic (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 70))

#define ads_InitCondition_Stress_Geostatic_verticalCoordinates (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 71))

#define ads_InitCondition_Temperature (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 72))

#define ads_InitCondition_Unfold (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 73))

#define ads_InitCondition_Unfold_coordinates (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 74))

#define ads_InitCondition_UserField (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 75))

/** First field captures the gradient in the n1 direction for beams or gradient through the thickness for shells. Second field captures the gradient in the n2 direction for beams. */
#define ads_InitCondition_UserField_gradientFields (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 76))

#define ads_InitCondition_Velocity (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 77))

#define ads_InitCondition_Velocity_dofType (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 78))

/** Initial condition for void ratio. */
#define ads_InitCondition_VoidRatio (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 79))

#define ads_InitCondition_VoidRatio_r2 (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 80))

#define ads_InitCondition_VoidRatio_v1 (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 81))

#define ads_InitCondition_VoidRatio_v2 (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 82))

#define ads_InitCondition_field (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 83))

#define ads_InitCondition_orientation (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 84))

#define ads_InitCondition_region (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 85))

#define ads_Model_initialConditions (ads_CoreFragmentTypeIndex(ads_CoreInitialConditionsFragment, 86))

/** Enum with record members. */
enum ads_CFDImportMembersEnm
{
    ads_CFDImport_stepName
};

/** 
Enum with association roles. */
enum ads_ContactSurfaces_contactPairsRolesEnm
{
    ads_ContactSurfaces_contactPairs_referent,
    ads_ContactSurfaces_contactPairs_referrer
};

/** Enum with association roles. */
enum ads_ContactSurfaces_nodeSetRolesEnm
{
    ads_ContactSurfaces_nodeSet_referent,
    ads_ContactSurfaces_nodeSet_referrer
};

/** 
Enum with association roles. */
enum ads_InitCondition_CFD_Velocity_componentRolesEnm
{
    ads_InitCondition_CFD_Velocity_component_referent,
    ads_InitCondition_CFD_Velocity_component_referrer
};

/** Enum with record members. */
enum ads_InitCondition_CFD_WaveInletMembersEnm
{
    ads_InitCondition_CFD_WaveInlet_waveHeight,
    ads_InitCondition_CFD_WaveInlet_waveOrder
};

/** Enum with association roles. */
enum ads_InitCondition_CFD_fieldsRolesEnm
{
    ads_InitCondition_CFD_fields_referent,
    ads_InitCondition_CFD_fields_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_CFD_importRolesEnm
{
    ads_InitCondition_CFD_import_child,
    ads_InitCondition_CFD_import_parent
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_boltHalfThreadAngleRolesEnm
{
    ads_InitCondition_ClearanceContactPair_boltHalfThreadAngle_child,
    ads_InitCondition_ClearanceContactPair_boltHalfThreadAngle_parent
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_boltMajorThreadDiameterRolesEnm
{
    ads_InitCondition_ClearanceContactPair_boltMajorThreadDiameter_child,
    ads_InitCondition_ClearanceContactPair_boltMajorThreadDiameter_parent
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_boltMeanThreadDiameterRolesEnm
{
    ads_InitCondition_ClearanceContactPair_boltMeanThreadDiameter_child,
    ads_InitCondition_ClearanceContactPair_boltMeanThreadDiameter_parent
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_boltPitchRolesEnm
{
    ads_InitCondition_ClearanceContactPair_boltPitch_child,
    ads_InitCondition_ClearanceContactPair_boltPitch_parent
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_boltPointARolesEnm
{
    ads_InitCondition_ClearanceContactPair_boltPointA_referent,
    ads_InitCondition_ClearanceContactPair_boltPointA_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_boltPointBRolesEnm
{
    ads_InitCondition_ClearanceContactPair_boltPointB_referent,
    ads_InitCondition_ClearanceContactPair_boltPointB_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_clearanceRolesEnm
{
    ads_InitCondition_ClearanceContactPair_clearance_referent,
    ads_InitCondition_ClearanceContactPair_clearance_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_contactPairsRolesEnm
{
    ads_InitCondition_ClearanceContactPair_contactPairs_referent,
    ads_InitCondition_ClearanceContactPair_contactPairs_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_ClearanceContactPair_normalRolesEnm
{
    ads_InitCondition_ClearanceContactPair_normal_referent,
    ads_InitCondition_ClearanceContactPair_normal_referrer
};

/** 
Enum with record members. */
enum ads_InitCondition_ContactMembersEnm
{
    ads_InitCondition_Contact_normal
};

/** Enum with association roles. */
enum ads_InitCondition_Contact_contactSurfacesRolesEnm
{
    ads_InitCondition_Contact_contactSurfaces_child,
    ads_InitCondition_Contact_contactSurfaces_parent
};

/** Enum with association roles. */
enum ads_InitCondition_PorePressure_p2RolesEnm
{
    ads_InitCondition_PorePressure_p2_referent,
    ads_InitCondition_PorePressure_p2_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_PorePressure_v1RolesEnm
{
    ads_InitCondition_PorePressure_v1_referent,
    ads_InitCondition_PorePressure_v1_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_PorePressure_v2RolesEnm
{
    ads_InitCondition_PorePressure_v2_referent,
    ads_InitCondition_PorePressure_v2_referrer
};

/** Enum with record members. */
enum ads_InitCondition_RebarMembersEnm
{
    ads_InitCondition_Rebar_all
};

/** Enum with record members. */
enum ads_InitCondition_Rebar_StressMembersEnm
{
    ads_InitCondition_Rebar_Stress_all
};

/** Enum with association roles. */
enum ads_InitCondition_Rebar_rebarRolesEnm
{
    ads_InitCondition_Rebar_rebar_referent,
    ads_InitCondition_Rebar_rebar_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_Rebar_rebarsRolesEnm
{
    ads_InitCondition_Rebar_rebars_referent,
    ads_InitCondition_Rebar_rebars_referrer
};

/** 
Enum with association roles. */
enum ads_InitCondition_RotatingVelocity_nodesABRolesEnm
{
    ads_InitCondition_RotatingVelocity_nodesAB_referent,
    ads_InitCondition_RotatingVelocity_nodesAB_referrer
};

/** 
Enum with association roles. */
enum ads_InitCondition_RotatingVelocity_pointARolesEnm
{
    ads_InitCondition_RotatingVelocity_pointA_child,
    ads_InitCondition_RotatingVelocity_pointA_parent
};

/** 
Enum with association roles. */
enum ads_InitCondition_RotatingVelocity_pointBRolesEnm
{
    ads_InitCondition_RotatingVelocity_pointB_child,
    ads_InitCondition_RotatingVelocity_pointB_parent
};

/** 
Enum with association roles. */
enum ads_InitCondition_RotatingVelocity_translationalVelocityRolesEnm
{
    ads_InitCondition_RotatingVelocity_translationalVelocity_child,
    ads_InitCondition_RotatingVelocity_translationalVelocity_parent
};

/** Enum with record members. */
enum ads_InitCondition_Stress_GeostaticMembersEnm
{
    ads_InitCondition_Stress_Geostatic_firstLateralStress,
    ads_InitCondition_Stress_Geostatic_secondLateralStress
};

/** Enum with association roles. */
enum ads_InitCondition_Stress_Geostatic_verticalCoordinatesRolesEnm
{
    ads_InitCondition_Stress_Geostatic_verticalCoordinates_child,
    ads_InitCondition_Stress_Geostatic_verticalCoordinates_parent
};

/** Enum with association roles. */
enum ads_InitCondition_Unfold_coordinatesRolesEnm
{
    ads_InitCondition_Unfold_coordinates_child,
    ads_InitCondition_Unfold_coordinates_parent
};

/** 
Enum with association roles. */
enum ads_InitCondition_UserField_gradientFieldsRolesEnm
{
    ads_InitCondition_UserField_gradientFields_child,
    ads_InitCondition_UserField_gradientFields_parent
};

/** Enum with association roles. */
enum ads_InitCondition_Velocity_dofTypeRolesEnm
{
    ads_InitCondition_Velocity_dofType_referent,
    ads_InitCondition_Velocity_dofType_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_VoidRatio_r2RolesEnm
{
    ads_InitCondition_VoidRatio_r2_referent,
    ads_InitCondition_VoidRatio_r2_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_VoidRatio_v1RolesEnm
{
    ads_InitCondition_VoidRatio_v1_referent,
    ads_InitCondition_VoidRatio_v1_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_VoidRatio_v2RolesEnm
{
    ads_InitCondition_VoidRatio_v2_referent,
    ads_InitCondition_VoidRatio_v2_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_fieldRolesEnm
{
    ads_InitCondition_field_referent,
    ads_InitCondition_field_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_orientationRolesEnm
{
    ads_InitCondition_orientation_referent,
    ads_InitCondition_orientation_referrer
};

/** Enum with association roles. */
enum ads_InitCondition_regionRolesEnm
{
    ads_InitCondition_region_referent,
    ads_InitCondition_region_referrer
};

/** Enum with association roles. */
enum ads_Model_initialConditionsRolesEnm
{
    ads_Model_initialConditions_child,
    ads_Model_initialConditions_parent
};

#endif
