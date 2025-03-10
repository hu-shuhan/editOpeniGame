//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreConnectorLibraryC_h
#define ads_CoreConnectorLibraryC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment ConnectorLibrary of the latest level of form Core */

/** The optional dependent columns for the table will be scaling constant 1, scaling const 2, etc (= number of components), relative position 1, relative position 2, etc (= number of independentColumns. */
#define ads_CMecCDCVariableTable (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 0))

/** An abstraction of available connection types. A particular connection type implies a specific set of available C.O.R.M's(Components of Relative Motion). */
#define ads_ConnectionType (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 1))

/** A collection of connection types so that they may serve as d-set dimensions. */
#define ads_ConnectionTypeCollection (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 2))

/** A hub for assembling complex connector behavior options and other properties. */
#define ads_ConnectorBehavior (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 3))

/** A collection of connector behaviors so that they may serve as d-set dimensions. */
#define ads_ConnectorBehaviorCollection (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 4))

/** Sequence of ConnectorPotentialTerms constituting the ConnectorPotential. */
#define ads_ConnectorBehavior_connectorDerivedComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 5))

#define ads_ConnectorBehavior_properties (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 6))

#define ads_ConnectorBehavior_referenceGeometry (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 7))

/** tolerances for rate-dependent connector hardening data. */
#define ads_ConnectorHardeningRateDependentTolerance (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 8))

/** A term in the connector potential function for a connector. */
#define ads_ConnectorPotentialTerm (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 9))

#define ads_ConnectorPotentialTerm_cdcComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 10))

#define ads_ConnectorPotentialTerm_dofComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 11))

/** Data type to capture the connector constitutive reference geometry. */
#define ads_ConstitutiveReference (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 12))

/** predefined parameters for connector friction in a cylindrical connector.WIP(union types for optional parameters). */
#define ads_CylindricalPredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 13))

/** Collection of connection types. */
#define ads_GlobalCollections_connectionTypeCollection (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 14))

/** Collection of connector behaviors. */
#define ads_GlobalCollections_connectorBehaviorCollection (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 15))

/** predefined parameters for connector friction in a hinge connector.WIP(union types for optional parameters) */
#define ads_HingePredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 16))

#define ads_Model_connectorBehavior (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 17))

/** predefined parameters for connector friction in a planar connector.WIP(union types for optional parameters). */
#define ads_PlanarPredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 18))

/** Associate the components of relative motion that will be damaged. If this is omitted and a valid component is specified on the parent connector damage initiation, only the specified component will undergo damage. If this association is absent and a valid component is omitted on the parent connector damage initiation, only the components of relative motion involved in the associated connector potential will undergo damage. */
#define ads_Prop_CMec_DamageEvolution_Energy_affectedComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 19))

/** Associate the components of relative motion that will be damaged. If this is omitted and a valid component is specified on the parent connector damage initiation, only the specified component will undergo damage. If this association is absent and a valid component is omitted on the parent connector damage initiation, only the components of relative motion involved in the associated connector potential will undergo damage. */
#define ads_Prop_CMec_DamageEvolution_TransRot_Exponential_affectedComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 20))

/** Associate the components of relative motion that will be damaged. If this is omitted and a valid component is specified on the parent connector damage initiation, only the specified component will undergo damage. If this association is absent and a valid component is omitted on the parent connector damage initiation, only the components of relative motion involved in the associated connector potential will undergo damage. */
#define ads_Prop_CMec_DamageEvolution_TransRot_Linear_affectedComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 21))

/** Associate the components of relative motion that will be damaged. If this is omitted and a valid component is specified on the parent connector damage initiation, only the specified component will undergo damage. If this association is absent and a valid component is omitted on the parent connector damage initiation, only the components of relative motion involved in the associated connector potential will undergo damage. */
#define ads_Prop_CMec_DamageEvolution_TransRot_Tabular_affectedComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 22))

/** in connector elements beyond simple linear elasticity or damping often requires the definition of a resultant force involving several intrinsic (1 through 6) components or the definition of a "direction" not aligned with any of the intrinsic components. These user-defined resultants or directions are called derived components. The forces and motions associated with these derived components are functions of the forces and motions in the intrinsic relative components of motion in the connector element. */
#define ads_Prop_CMec_DerivedComponent (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 23))

/** components included in the derived component definition */
#define ads_Prop_CMec_DerivedComponent_components (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 24))

/** Specify dependencies on components of relative position or motion included in the derived component definition. */
#define ads_Prop_CMec_DerivedComponent_independentComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 25))

/** Scaling constant that multiplies the connector component. One table per component */
#define ads_Prop_CMec_DerivedComponent_table (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 26))

/** independent components associated with a connector behavior option. If this reference exists, it implies a dependency of properties on components of relative position or components of constitutive relative motion. */
#define ads_Prop_CMec_Elastic_Hyper_Rotational_independentComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 27))

/** independent components associated with a connector behavior option. If this reference exists, it implies a dependency of properties on components of relative position or components of constitutive relative motion. */
#define ads_Prop_CMec_Elastic_Hyper_Translational_independentComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 28))

/** Use Prop_CMec_Failure instead. */
#define ads_Prop_CMec_Failure (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 29))

/** independent components associated with a connector behavior option. If this reference exists, it implies a dependency of properties on components of relative position or components of constitutive relative motion. */
#define ads_Prop_CMec_Friction_UserDefined_independentComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 30))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_cylindricalPCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 31))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_hingePCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 32))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_planarPCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 33))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_slidePCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 34))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_slotPCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 35))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_translatorPCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 36))

/** Connector friction to the predefined connector friction parameter composition. */
#define ads_Prop_CMec_Friction_ujointPCFParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 37))

/** Specify connector lock for connector elements. */
#define ads_Prop_CMec_Lock (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 38))

/** Collection of connection types. */
#define ads_Prop_CMec_PlasticOption_rateDependentTolerance (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 39))

/** Sequence of ConnectorPotentialTerms constituting the Connector Potential. */
#define ads_Prop_CMec_Potential_connectorPotentialTerms (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 40))

#define ads_Prop_CMec_Rigid (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 41))

#define ads_Prop_CMec_Rigid_components (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 42))

/** independent components associated with a connector behavior option. If this reference exists, it implies a dependency of properties on components of relative position or components of constitutive relative motion. */
#define ads_Prop_CMec_Viscosity_Nonlinear_Rotational_independentComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 43))

/** independent components associated with a connector behavior option. If this reference exists, it implies a dependency of properties on components of relative position or components of constitutive relative motion. */
#define ads_Prop_CMec_Viscosity_Nonlinear_Translational_independentComponents (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 44))

/** predefined parameters for connector friction in a slide-plane connector.WIP(union types for optional parameters) */
#define ads_SlidePlanePredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 45))

/** predefined parameters for connector friction in a slot connector.WIP(union types for optional parameters). */
#define ads_SlotPredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 46))

#define ads_SymbolsLib_connectionType (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 47))

/** predefined parameters for connector friction in a translator connector.WIP(union types for optional parameters) */
#define ads_TranslatorPredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 48))

/** predefined parameters for connector friction in a u-joint connector.WIP(union types for optional parameters) */
#define ads_UJointPredefinedConnectorFrictionParameter (ads_CoreFragmentTypeIndex(ads_CoreConnectorLibraryFragment, 49))

/** Enum with the symbols of data type ConnectionType*/
enum ads_ConnectionTypeSymbolsEnm
{
    ads_ConnectionType_ACCELEROMETER,
    ads_ConnectionType_ALIGN,
    ads_ConnectionType_AXIAL,
    ads_ConnectionType_BEAM,
    ads_ConnectionType_BUSHING,
    ads_ConnectionType_CARDAN,
    ads_ConnectionType_CARTESIAN,
    ads_ConnectionType_CONSTANT_VELOCITY,
    ads_ConnectionType_CVJOINT,
    ads_ConnectionType_CYLINDRICAL,
    ads_ConnectionType_EULER,
    ads_ConnectionType_FLEXION_TORSION,
    ads_ConnectionType_FLOW_CONVERTER,
    ads_ConnectionType_HINGE,
    ads_ConnectionType_JOIN,
    ads_ConnectionType_LINK,
    ads_ConnectionType_PLANAR,
    ads_ConnectionType_PROJECTION_CARTESIAN,
    ads_ConnectionType_PROJECTION_FLEXION_TORSION,
    ads_ConnectionType_RADIAL_THRUST,
    ads_ConnectionType_RETRACTOR,
    ads_ConnectionType_REVOLUTE,
    ads_ConnectionType_ROTATION,
    ads_ConnectionType_ROTATION_ACCELEROMETER,
    ads_ConnectionType_SLIDE_PLANE,
    ads_ConnectionType_SLIPRING,
    ads_ConnectionType_SLOT,
    ads_ConnectionType_TRANSLATOR,
    ads_ConnectionType_UJOINT,
    ads_ConnectionType_UNIVERSAL,
    ads_ConnectionType_WELD
};

/** 
Enum with record members. */
enum ads_ConnectorBehaviorMembersEnm
{
    ads_ConnectorBehavior_integration
};

enum ads_ConnectorBehavior_integrationEnm
{
    ads_ConnectorBehavior_integration_EXPLICIT,
    ads_ConnectorBehavior_integration_IMPLICIT
};

/** 
Enum with association roles. */
enum ads_ConnectorBehavior_connectorDerivedComponentsRolesEnm
{
    ads_ConnectorBehavior_connectorDerivedComponents_child,
    ads_ConnectorBehavior_connectorDerivedComponents_parent
};

/** Enum with association roles. */
enum ads_ConnectorBehavior_propertiesRolesEnm
{
    ads_ConnectorBehavior_properties_child,
    ads_ConnectorBehavior_properties_parent
};

/** Enum with association roles. */
enum ads_ConnectorBehavior_referenceGeometryRolesEnm
{
    ads_ConnectorBehavior_referenceGeometry_child,
    ads_ConnectorBehavior_referenceGeometry_parent
};

/** 
Enum with record members. */
enum ads_ConnectorHardeningRateDependentToleranceMembersEnm
{
    ads_ConnectorHardeningRateDependentTolerance_rateFilterFactor,
    ads_ConnectorHardeningRateDependentTolerance_rateInterpolation
};

enum ads_ConnectorHardeningRateDependentTolerance_rateInterpolationEnm
{
    ads_ConnectorHardeningRateDependentTolerance_rateInterpolation_LINEAR,
    ads_ConnectorHardeningRateDependentTolerance_rateInterpolation_LOGARITHMIC
};

/** 
Enum with record members. */
enum ads_ConnectorPotentialTermMembersEnm
{
    ads_ConnectorPotentialTerm_alpha,
    ads_ConnectorPotentialTerm_function,
    ads_ConnectorPotentialTerm_scalingFactor,
    ads_ConnectorPotentialTerm_shiftFactor,
    ads_ConnectorPotentialTerm_sign
};

enum ads_ConnectorPotentialTerm_functionEnm
{
    ads_ConnectorPotentialTerm_function_ABS,
    ads_ConnectorPotentialTerm_function_IDENTITY,
    ads_ConnectorPotentialTerm_function_MACAULEY
};

enum ads_ConnectorPotentialTerm_signEnm
{
    ads_ConnectorPotentialTerm_sign_NEGATIVE,
    ads_ConnectorPotentialTerm_sign_POSITIVE
};

/** Enum with association roles. */
enum ads_ConnectorPotentialTerm_cdcComponentRolesEnm
{
    ads_ConnectorPotentialTerm_cdcComponent_referent,
    ads_ConnectorPotentialTerm_cdcComponent_referrer
};

/** Enum with association roles. */
enum ads_ConnectorPotentialTerm_dofComponentRolesEnm
{
    ads_ConnectorPotentialTerm_dofComponent_referent,
    ads_ConnectorPotentialTerm_dofComponent_referrer
};

/** 
Enum with record members. */
enum ads_ConstitutiveReferenceMembersEnm
{
    ads_ConstitutiveReference_refAngle1,
    ads_ConstitutiveReference_refAngle2,
    ads_ConstitutiveReference_refAngle3,
    ads_ConstitutiveReference_refLength1,
    ads_ConstitutiveReference_refLength2,
    ads_ConstitutiveReference_refLength3
};

/** 
Enum with record members. */
enum ads_CylindricalPredefinedConnectorFrictionParameterMembersEnm
{
    ads_CylindricalPredefinedConnectorFrictionParameter_internalForce,
    ads_CylindricalPredefinedConnectorFrictionParameter_internalForceType,
    ads_CylindricalPredefinedConnectorFrictionParameter_l,
    ads_CylindricalPredefinedConnectorFrictionParameter_lType,
    ads_CylindricalPredefinedConnectorFrictionParameter_r
};

enum ads_CylindricalPredefinedConnectorFrictionParameter_internalForceTypeEnm
{
    ads_CylindricalPredefinedConnectorFrictionParameter_internalForceType_ABSENT,
    ads_CylindricalPredefinedConnectorFrictionParameter_internalForceType_PRESENT
};

enum ads_CylindricalPredefinedConnectorFrictionParameter_lTypeEnm
{
    ads_CylindricalPredefinedConnectorFrictionParameter_lType_ABSENT,
    ads_CylindricalPredefinedConnectorFrictionParameter_lType_PRESENT
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_connectionTypeCollectionRolesEnm
{
    ads_GlobalCollections_connectionTypeCollection_child,
    ads_GlobalCollections_connectionTypeCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_connectorBehaviorCollectionRolesEnm
{
    ads_GlobalCollections_connectorBehaviorCollection_child,
    ads_GlobalCollections_connectorBehaviorCollection_parent
};

/** 
Enum with record members. */
enum ads_HingePredefinedConnectorFrictionParameterMembersEnm
{
    ads_HingePredefinedConnectorFrictionParameter_internalMoment,
    ads_HingePredefinedConnectorFrictionParameter_internalMomentType,
    ads_HingePredefinedConnectorFrictionParameter_ls,
    ads_HingePredefinedConnectorFrictionParameter_lsType,
    ads_HingePredefinedConnectorFrictionParameter_ra,
    ads_HingePredefinedConnectorFrictionParameter_raType,
    ads_HingePredefinedConnectorFrictionParameter_rp
};

enum ads_HingePredefinedConnectorFrictionParameter_internalMomentTypeEnm
{
    ads_HingePredefinedConnectorFrictionParameter_internalMomentType_ABSENT,
    ads_HingePredefinedConnectorFrictionParameter_internalMomentType_PRESENT
};

enum ads_HingePredefinedConnectorFrictionParameter_lsTypeEnm
{
    ads_HingePredefinedConnectorFrictionParameter_lsType_ABSENT,
    ads_HingePredefinedConnectorFrictionParameter_lsType_PRESENT
};

enum ads_HingePredefinedConnectorFrictionParameter_raTypeEnm
{
    ads_HingePredefinedConnectorFrictionParameter_raType_ABSENT,
    ads_HingePredefinedConnectorFrictionParameter_raType_PRESENT
};

/** Enum with association roles. */
enum ads_Model_connectorBehaviorRolesEnm
{
    ads_Model_connectorBehavior_child,
    ads_Model_connectorBehavior_parent
};

/** 
Enum with record members. */
enum ads_PlanarPredefinedConnectorFrictionParameterMembersEnm
{
    ads_PlanarPredefinedConnectorFrictionParameter_internalForce,
    ads_PlanarPredefinedConnectorFrictionParameter_internalForceType,
    ads_PlanarPredefinedConnectorFrictionParameter_internalMoment,
    ads_PlanarPredefinedConnectorFrictionParameter_internalMomentType,
    ads_PlanarPredefinedConnectorFrictionParameter_r,
    ads_PlanarPredefinedConnectorFrictionParameter_rType
};

enum ads_PlanarPredefinedConnectorFrictionParameter_internalForceTypeEnm
{
    ads_PlanarPredefinedConnectorFrictionParameter_internalForceType_ABSENT,
    ads_PlanarPredefinedConnectorFrictionParameter_internalForceType_PRESENT
};

enum ads_PlanarPredefinedConnectorFrictionParameter_internalMomentTypeEnm
{
    ads_PlanarPredefinedConnectorFrictionParameter_internalMomentType_ABSENT,
    ads_PlanarPredefinedConnectorFrictionParameter_internalMomentType_PRESENT
};

enum ads_PlanarPredefinedConnectorFrictionParameter_rTypeEnm
{
    ads_PlanarPredefinedConnectorFrictionParameter_rType_ABSENT,
    ads_PlanarPredefinedConnectorFrictionParameter_rType_PRESENT
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_Energy_affectedComponentsRolesEnm
{
    ads_Prop_CMec_DamageEvolution_Energy_affectedComponents_referent,
    ads_Prop_CMec_DamageEvolution_Energy_affectedComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_TransRot_Exponential_affectedComponentsRolesEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_affectedComponents_referent,
    ads_Prop_CMec_DamageEvolution_TransRot_Exponential_affectedComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_TransRot_Linear_affectedComponentsRolesEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_affectedComponents_referent,
    ads_Prop_CMec_DamageEvolution_TransRot_Linear_affectedComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DamageEvolution_TransRot_Tabular_affectedComponentsRolesEnm
{
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_affectedComponents_referent,
    ads_Prop_CMec_DamageEvolution_TransRot_Tabular_affectedComponents_referrer
};

/** 
Enum with record members. */
enum ads_Prop_CMec_DerivedComponentMembersEnm
{
    ads_Prop_CMec_DerivedComponent_indepCompType,
    ads_Prop_CMec_DerivedComponent_operator,
    ads_Prop_CMec_DerivedComponent_sign
};

enum ads_Prop_CMec_DerivedComponent_indepCompTypeEnm
{
    ads_Prop_CMec_DerivedComponent_indepCompType_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_DerivedComponent_indepCompType_NOT_SPECIFIED,
    ads_Prop_CMec_DerivedComponent_indepCompType_RELATIVE_POSITION
};

enum ads_Prop_CMec_DerivedComponent_operatorEnm
{
    ads_Prop_CMec_DerivedComponent_operator_MACAULEY_SUM,
    ads_Prop_CMec_DerivedComponent_operator_NORM,
    ads_Prop_CMec_DerivedComponent_operator_SUM
};

enum ads_Prop_CMec_DerivedComponent_signEnm
{
    ads_Prop_CMec_DerivedComponent_sign_NEGATIVE,
    ads_Prop_CMec_DerivedComponent_sign_POSITIVE
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DerivedComponent_componentsRolesEnm
{
    ads_Prop_CMec_DerivedComponent_components_referent,
    ads_Prop_CMec_DerivedComponent_components_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DerivedComponent_independentComponentsRolesEnm
{
    ads_Prop_CMec_DerivedComponent_independentComponents_referent,
    ads_Prop_CMec_DerivedComponent_independentComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_DerivedComponent_tableRolesEnm
{
    ads_Prop_CMec_DerivedComponent_table_child,
    ads_Prop_CMec_DerivedComponent_table_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Elastic_Hyper_Rotational_independentComponentsRolesEnm
{
    ads_Prop_CMec_Elastic_Hyper_Rotational_independentComponents_referent,
    ads_Prop_CMec_Elastic_Hyper_Rotational_independentComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Elastic_Hyper_Translational_independentComponentsRolesEnm
{
    ads_Prop_CMec_Elastic_Hyper_Translational_independentComponents_referent,
    ads_Prop_CMec_Elastic_Hyper_Translational_independentComponents_referrer
};

/** 
Enum with record members. */
enum ads_Prop_CMec_FailureMembersEnm
{
    ads_Prop_CMec_Failure_release
};

enum ads_Prop_CMec_Failure_releaseEnm
{
    ads_Prop_CMec_Failure_release_ALL,
    ads_Prop_CMec_Failure_release_SPECIFIC
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_UserDefined_independentComponentsRolesEnm
{
    ads_Prop_CMec_Friction_UserDefined_independentComponents_referent,
    ads_Prop_CMec_Friction_UserDefined_independentComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_cylindricalPCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_cylindricalPCFParameter_child,
    ads_Prop_CMec_Friction_cylindricalPCFParameter_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_hingePCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_hingePCFParameter_child,
    ads_Prop_CMec_Friction_hingePCFParameter_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_planarPCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_planarPCFParameter_child,
    ads_Prop_CMec_Friction_planarPCFParameter_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_slidePCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_slidePCFParameter_child,
    ads_Prop_CMec_Friction_slidePCFParameter_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_slotPCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_slotPCFParameter_child,
    ads_Prop_CMec_Friction_slotPCFParameter_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_translatorPCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_translatorPCFParameter_child,
    ads_Prop_CMec_Friction_translatorPCFParameter_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Friction_ujointPCFParameterRolesEnm
{
    ads_Prop_CMec_Friction_ujointPCFParameter_child,
    ads_Prop_CMec_Friction_ujointPCFParameter_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_LockMembersEnm
{
    ads_Prop_CMec_Lock_lock
};

enum ads_Prop_CMec_Lock_lockEnm
{
    ads_Prop_CMec_Lock_lock_ALL,
    ads_Prop_CMec_Lock_lock_SPECIFIC
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_PlasticOption_rateDependentToleranceRolesEnm
{
    ads_Prop_CMec_PlasticOption_rateDependentTolerance_child,
    ads_Prop_CMec_PlasticOption_rateDependentTolerance_parent
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Potential_connectorPotentialTermsRolesEnm
{
    ads_Prop_CMec_Potential_connectorPotentialTerms_child,
    ads_Prop_CMec_Potential_connectorPotentialTerms_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_RigidMembersEnm
{
    ads_Prop_CMec_Rigid_component
};

enum ads_Prop_CMec_Rigid_componentEnm
{
    ads_Prop_CMec_Rigid_component_ALL,
    ads_Prop_CMec_Rigid_component_SPECIFIC
};

/** Enum with association roles. */
enum ads_Prop_CMec_Rigid_componentsRolesEnm
{
    ads_Prop_CMec_Rigid_components_referent,
    ads_Prop_CMec_Rigid_components_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Nonlinear_Rotational_independentComponentsRolesEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_independentComponents_referent,
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_independentComponents_referrer
};

/** 
Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Nonlinear_Translational_independentComponentsRolesEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_independentComponents_referent,
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_independentComponents_referrer
};

/** 
Enum with record members. */
enum ads_SlidePlanePredefinedConnectorFrictionParameterMembersEnm
{
    ads_SlidePlanePredefinedConnectorFrictionParameter_internalForce,
    ads_SlidePlanePredefinedConnectorFrictionParameter_internalForceType
};

enum ads_SlidePlanePredefinedConnectorFrictionParameter_internalForceTypeEnm
{
    ads_SlidePlanePredefinedConnectorFrictionParameter_internalForceType_ABSENT,
    ads_SlidePlanePredefinedConnectorFrictionParameter_internalForceType_PRESENT
};

/** 
Enum with record members. */
enum ads_SlotPredefinedConnectorFrictionParameterMembersEnm
{
    ads_SlotPredefinedConnectorFrictionParameter_internalForce,
    ads_SlotPredefinedConnectorFrictionParameter_internalForceType
};

enum ads_SlotPredefinedConnectorFrictionParameter_internalForceTypeEnm
{
    ads_SlotPredefinedConnectorFrictionParameter_internalForceType_ABSENT,
    ads_SlotPredefinedConnectorFrictionParameter_internalForceType_PRESENT
};

/** Enum with association roles. */
enum ads_SymbolsLib_connectionTypeRolesEnm
{
    ads_SymbolsLib_connectionType_child,
    ads_SymbolsLib_connectionType_parent
};

/** 
Enum with record members. */
enum ads_TranslatorPredefinedConnectorFrictionParameterMembersEnm
{
    ads_TranslatorPredefinedConnectorFrictionParameter_internalForce,
    ads_TranslatorPredefinedConnectorFrictionParameter_internalForceType,
    ads_TranslatorPredefinedConnectorFrictionParameter_l,
    ads_TranslatorPredefinedConnectorFrictionParameter_lType,
    ads_TranslatorPredefinedConnectorFrictionParameter_r,
    ads_TranslatorPredefinedConnectorFrictionParameter_rType
};

enum ads_TranslatorPredefinedConnectorFrictionParameter_internalForceTypeEnm
{
    ads_TranslatorPredefinedConnectorFrictionParameter_internalForceType_ABSENT,
    ads_TranslatorPredefinedConnectorFrictionParameter_internalForceType_PRESENT
};

enum ads_TranslatorPredefinedConnectorFrictionParameter_lTypeEnm
{
    ads_TranslatorPredefinedConnectorFrictionParameter_lType_ABSENT,
    ads_TranslatorPredefinedConnectorFrictionParameter_lType_PRESENT
};

enum ads_TranslatorPredefinedConnectorFrictionParameter_rTypeEnm
{
    ads_TranslatorPredefinedConnectorFrictionParameter_rType_ABSENT,
    ads_TranslatorPredefinedConnectorFrictionParameter_rType_PRESENT
};

/** 
Enum with record members. */
enum ads_UJointPredefinedConnectorFrictionParameterMembersEnm
{
    ads_UJointPredefinedConnectorFrictionParameter_la,
    ads_UJointPredefinedConnectorFrictionParameter_ls,
    ads_UJointPredefinedConnectorFrictionParameter_lsType,
    ads_UJointPredefinedConnectorFrictionParameter_mc1,
    ads_UJointPredefinedConnectorFrictionParameter_mc1Type,
    ads_UJointPredefinedConnectorFrictionParameter_mc3,
    ads_UJointPredefinedConnectorFrictionParameter_mc3Type,
    ads_UJointPredefinedConnectorFrictionParameter_ra,
    ads_UJointPredefinedConnectorFrictionParameter_raType,
    ads_UJointPredefinedConnectorFrictionParameter_rp
};

enum ads_UJointPredefinedConnectorFrictionParameter_lsTypeEnm
{
    ads_UJointPredefinedConnectorFrictionParameter_lsType_ABSENT,
    ads_UJointPredefinedConnectorFrictionParameter_lsType_PRESENT
};

enum ads_UJointPredefinedConnectorFrictionParameter_mc1TypeEnm
{
    ads_UJointPredefinedConnectorFrictionParameter_mc1Type_ABSENT,
    ads_UJointPredefinedConnectorFrictionParameter_mc1Type_PRESENT
};

enum ads_UJointPredefinedConnectorFrictionParameter_mc3TypeEnm
{
    ads_UJointPredefinedConnectorFrictionParameter_mc3Type_ABSENT,
    ads_UJointPredefinedConnectorFrictionParameter_mc3Type_PRESENT
};

enum ads_UJointPredefinedConnectorFrictionParameter_raTypeEnm
{
    ads_UJointPredefinedConnectorFrictionParameter_raType_ABSENT,
    ads_UJointPredefinedConnectorFrictionParameter_raType_PRESENT
};

#endif
