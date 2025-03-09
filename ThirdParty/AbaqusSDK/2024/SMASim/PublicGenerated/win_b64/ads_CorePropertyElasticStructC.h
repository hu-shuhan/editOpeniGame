//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyElasticStructC_h
#define ads_CorePropertyElasticStructC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyElasticStruct of the latest level of form Core */

#define ads_BMecElasticLinearAxialTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 0))

/** *M1, *M2, *TORQUE, LINEAR */
#define ads_BMecElasticLinearRotationalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 1))

#define ads_BMecElasticLinearTransverseShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 2))

#define ads_BMecFullCoupledSectionStiffnessTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 3))

#define ads_BMecSectionInertiaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 4))

#define ads_CMecElasticHyperRotationalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 5))

#define ads_CMecElasticHyperTranslationalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 6))

#define ads_CMecElasticLinearCoupledSymmTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 7))

#define ads_CMecElasticLinearCoupledUnsymmTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 8))

#define ads_CMecElasticLinearRotationalUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 9))

#define ads_CMecElasticLinearTranslationalUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 10))

#define ads_CMecElasticLinearTransverseShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 11))

#define ads_IMecElasticLinearCoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 12))

#define ads_IMecElasticLinearMembraneTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 13))

#define ads_IMecElasticLinearTransverseShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 14))

#define ads_IMecElasticLinearUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 15))

#define ads_Prop_BMec_Elastic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 16))

#define ads_Prop_BMec_Elastic_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 17))

#define ads_Prop_BMec_Elastic_Linear_Axial (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 18))

#define ads_Prop_BMec_Elastic_Linear_Axial_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 19))

#define ads_Prop_BMec_Elastic_Linear_FullCoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 20))

#define ads_Prop_BMec_Elastic_Linear_FullCoupled_SectionStiffness (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 21))

#define ads_Prop_BMec_Elastic_Linear_FullCoupled_SectionStiffness_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 22))

#define ads_Prop_BMec_Elastic_Linear_General (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 23))

#define ads_Prop_BMec_Elastic_Linear_General_centroid (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 24))

#define ads_Prop_BMec_Elastic_Linear_General_ea (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 25))

#define ads_Prop_BMec_Elastic_Linear_General_ei11 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 26))

#define ads_Prop_BMec_Elastic_Linear_General_ei12 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 27))

#define ads_Prop_BMec_Elastic_Linear_General_ei22 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 28))

#define ads_Prop_BMec_Elastic_Linear_General_gj (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 29))

#define ads_Prop_BMec_Elastic_Linear_General_shearCenter (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 30))

#define ads_Prop_BMec_Elastic_Linear_Rotational (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 31))

#define ads_Prop_BMec_Elastic_Linear_Rotational_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 32))

#define ads_Prop_BMec_Elastic_Linear_TransverseShear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 33))

#define ads_Prop_BMec_Elastic_Linear_TransverseShear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 34))

#define ads_Prop_BMec_Inertia (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 35))

#define ads_Prop_BMec_Inertia_SectionInertia (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 36))

#define ads_Prop_BMec_Inertia_SectionInertia_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 37))

#define ads_Prop_CMec_Elastic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 38))

#define ads_Prop_CMec_Elastic_Hyper (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 39))

#define ads_Prop_CMec_Elastic_Hyper_Rotational (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 40))

#define ads_Prop_CMec_Elastic_Hyper_Rotational_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 41))

#define ads_Prop_CMec_Elastic_Hyper_Translational (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 42))

#define ads_Prop_CMec_Elastic_Hyper_Translational_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 43))

#define ads_Prop_CMec_Elastic_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 44))

#define ads_Prop_CMec_Elastic_Linear_CoupledSymm (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 45))

#define ads_Prop_CMec_Elastic_Linear_CoupledSymm_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 46))

#define ads_Prop_CMec_Elastic_Linear_CoupledUnsymm (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 47))

#define ads_Prop_CMec_Elastic_Linear_CoupledUnsymm_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 48))

#define ads_Prop_CMec_Elastic_Linear_RotationalUncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 49))

#define ads_Prop_CMec_Elastic_Linear_RotationalUncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 50))

#define ads_Prop_CMec_Elastic_Linear_TranslationalUncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 51))

#define ads_Prop_CMec_Elastic_Linear_TranslationalUncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 52))

#define ads_Prop_CMec_Elastic_Linear_TransverseShear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 53))

#define ads_Prop_CMec_Elastic_Linear_TransverseShear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 54))

#define ads_Prop_IMec_Elastic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 55))

#define ads_Prop_IMec_Elastic_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 56))

#define ads_Prop_IMec_Elastic_Linear_Coupled (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 57))

#define ads_Prop_IMec_Elastic_Linear_Coupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 58))

/** Specify elastic properties for the membrane behavior of a gasket. */
#define ads_Prop_IMec_Elastic_Linear_Membrane (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 59))

#define ads_Prop_IMec_Elastic_Linear_Membrane_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 60))

/** Specify elastic properties for the transverse shear behavior of a gasket. */
#define ads_Prop_IMec_Elastic_Linear_TransverseShear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 61))

#define ads_Prop_IMec_Elastic_Linear_TransverseShear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 62))

#define ads_Prop_IMec_Elastic_Linear_Uncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 63))

#define ads_Prop_IMec_Elastic_Linear_Uncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 64))

#define ads_Prop_SMec_Elastic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 65))

#define ads_Prop_SMec_Elastic_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 66))

#define ads_Prop_SMec_Elastic_Linear_General (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 67))

#define ads_Prop_SMec_Elastic_Linear_General_bendingBending (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 68))

#define ads_Prop_SMec_Elastic_Linear_General_membraneBending (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 69))

#define ads_Prop_SMec_Elastic_Linear_General_membraneMembrane (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 70))

#define ads_Prop_SMec_Elastic_Linear_General_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 71))

#define ads_Prop_SMec_Elastic_Linear_Thickness (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 72))

#define ads_Prop_SMec_Elastic_Linear_Thickness_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 73))

#define ads_Prop_SMec_Elastic_Linear_TransverseShear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 74))

#define ads_Prop_SMec_Elastic_Linear_TransverseShear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 75))

/** *SHELL GENERAL SECTION */
#define ads_SMecElasticLinearGeneralTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 76))

/** *SHELL GENERAL SECTION, THICKNESS MODULUS */
#define ads_SMecElasticLinearThicknessTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 77))

#define ads_SMecElasticLinearTransverseShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticStructFragment, 78))

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_Axial_tableRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_Axial_table_child,
    ads_Prop_BMec_Elastic_Linear_Axial_table_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_FullCoupled_SectionStiffness_tableRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_FullCoupled_SectionStiffness_table_child,
    ads_Prop_BMec_Elastic_Linear_FullCoupled_SectionStiffness_table_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_centroidRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_centroid_child,
    ads_Prop_BMec_Elastic_Linear_General_centroid_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_eaRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_ea_child,
    ads_Prop_BMec_Elastic_Linear_General_ea_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_ei11RolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_ei11_child,
    ads_Prop_BMec_Elastic_Linear_General_ei11_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_ei12RolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_ei12_child,
    ads_Prop_BMec_Elastic_Linear_General_ei12_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_ei22RolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_ei22_child,
    ads_Prop_BMec_Elastic_Linear_General_ei22_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_gjRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_gj_child,
    ads_Prop_BMec_Elastic_Linear_General_gj_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_General_shearCenterRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_General_shearCenter_child,
    ads_Prop_BMec_Elastic_Linear_General_shearCenter_parent
};

/** Enum with record members. */
enum ads_Prop_BMec_Elastic_Linear_RotationalMembersEnm
{
    ads_Prop_BMec_Elastic_Linear_Rotational_component
};

enum ads_Prop_BMec_Elastic_Linear_Rotational_componentEnm
{
    ads_Prop_BMec_Elastic_Linear_Rotational_component_M1,
    ads_Prop_BMec_Elastic_Linear_Rotational_component_M2,
    ads_Prop_BMec_Elastic_Linear_Rotational_component_TORQUE
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_Rotational_tableRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_Rotational_table_child,
    ads_Prop_BMec_Elastic_Linear_Rotational_table_parent
};

/** Enum with record members. */
enum ads_Prop_BMec_Elastic_Linear_TransverseShearMembersEnm
{
    ads_Prop_BMec_Elastic_Linear_TransverseShear_solverComputedSCF
};

/** Enum with association roles. */
enum ads_Prop_BMec_Elastic_Linear_TransverseShear_tableRolesEnm
{
    ads_Prop_BMec_Elastic_Linear_TransverseShear_table_child,
    ads_Prop_BMec_Elastic_Linear_TransverseShear_table_parent
};

/** Enum with association roles. */
enum ads_Prop_BMec_Inertia_SectionInertia_tableRolesEnm
{
    ads_Prop_BMec_Inertia_SectionInertia_table_child,
    ads_Prop_BMec_Inertia_SectionInertia_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_Elastic_HyperMembersEnm
{
    ads_Prop_CMec_Elastic_Hyper_indepCompEnm
};

enum ads_Prop_CMec_Elastic_Hyper_indepCompEnmEnm
{
    ads_Prop_CMec_Elastic_Hyper_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Elastic_Hyper_indepCompEnm_NONE,
    ads_Prop_CMec_Elastic_Hyper_indepCompEnm_POSITION
};

/** Enum with record members. */
enum ads_Prop_CMec_Elastic_Hyper_RotationalMembersEnm
{
    ads_Prop_CMec_Elastic_Hyper_Rotational_indepCompEnm
};

enum ads_Prop_CMec_Elastic_Hyper_Rotational_indepCompEnmEnm
{
    ads_Prop_CMec_Elastic_Hyper_Rotational_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Elastic_Hyper_Rotational_indepCompEnm_NONE,
    ads_Prop_CMec_Elastic_Hyper_Rotational_indepCompEnm_POSITION
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Hyper_Rotational_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Hyper_Rotational_table_child,
    ads_Prop_CMec_Elastic_Hyper_Rotational_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_Elastic_Hyper_TranslationalMembersEnm
{
    ads_Prop_CMec_Elastic_Hyper_Translational_indepCompEnm
};

enum ads_Prop_CMec_Elastic_Hyper_Translational_indepCompEnmEnm
{
    ads_Prop_CMec_Elastic_Hyper_Translational_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Elastic_Hyper_Translational_indepCompEnm_NONE,
    ads_Prop_CMec_Elastic_Hyper_Translational_indepCompEnm_POSITION
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Hyper_Translational_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Hyper_Translational_table_child,
    ads_Prop_CMec_Elastic_Hyper_Translational_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Linear_CoupledSymm_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Linear_CoupledSymm_table_child,
    ads_Prop_CMec_Elastic_Linear_CoupledSymm_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Linear_CoupledUnsymm_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Linear_CoupledUnsymm_table_child,
    ads_Prop_CMec_Elastic_Linear_CoupledUnsymm_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Linear_RotationalUncoupled_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Linear_RotationalUncoupled_table_child,
    ads_Prop_CMec_Elastic_Linear_RotationalUncoupled_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Linear_TranslationalUncoupled_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Linear_TranslationalUncoupled_table_child,
    ads_Prop_CMec_Elastic_Linear_TranslationalUncoupled_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Elastic_Linear_TransverseShear_tableRolesEnm
{
    ads_Prop_CMec_Elastic_Linear_TransverseShear_table_child,
    ads_Prop_CMec_Elastic_Linear_TransverseShear_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_ElasticMembersEnm
{
    ads_Prop_IMec_Elastic_moduli
};

enum ads_Prop_IMec_Elastic_moduliEnm
{
    ads_Prop_IMec_Elastic_moduli_INSTANTANEOUS,
    ads_Prop_IMec_Elastic_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_IMec_Elastic_LinearMembersEnm
{
    ads_Prop_IMec_Elastic_Linear_moduli,
    ads_Prop_IMec_Elastic_Linear_noCompression,
    ads_Prop_IMec_Elastic_Linear_noTension
};

enum ads_Prop_IMec_Elastic_Linear_moduliEnm
{
    ads_Prop_IMec_Elastic_Linear_moduli_INSTANTANEOUS,
    ads_Prop_IMec_Elastic_Linear_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_IMec_Elastic_Linear_CoupledMembersEnm
{
    ads_Prop_IMec_Elastic_Linear_Coupled_moduli,
    ads_Prop_IMec_Elastic_Linear_Coupled_noCompression,
    ads_Prop_IMec_Elastic_Linear_Coupled_noTension
};

enum ads_Prop_IMec_Elastic_Linear_Coupled_moduliEnm
{
    ads_Prop_IMec_Elastic_Linear_Coupled_moduli_INSTANTANEOUS,
    ads_Prop_IMec_Elastic_Linear_Coupled_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_IMec_Elastic_Linear_Coupled_tableRolesEnm
{
    ads_Prop_IMec_Elastic_Linear_Coupled_table_child,
    ads_Prop_IMec_Elastic_Linear_Coupled_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_IMec_Elastic_Linear_MembraneMembersEnm
{
    ads_Prop_IMec_Elastic_Linear_Membrane_moduli,
    ads_Prop_IMec_Elastic_Linear_Membrane_noCompression,
    ads_Prop_IMec_Elastic_Linear_Membrane_noTension
};

enum ads_Prop_IMec_Elastic_Linear_Membrane_moduliEnm
{
    ads_Prop_IMec_Elastic_Linear_Membrane_moduli_INSTANTANEOUS,
    ads_Prop_IMec_Elastic_Linear_Membrane_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_IMec_Elastic_Linear_Membrane_tableRolesEnm
{
    ads_Prop_IMec_Elastic_Linear_Membrane_table_child,
    ads_Prop_IMec_Elastic_Linear_Membrane_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_IMec_Elastic_Linear_TransverseShearMembersEnm
{
    ads_Prop_IMec_Elastic_Linear_TransverseShear_moduli,
    ads_Prop_IMec_Elastic_Linear_TransverseShear_noCompression,
    ads_Prop_IMec_Elastic_Linear_TransverseShear_noTension,
    ads_Prop_IMec_Elastic_Linear_TransverseShear_variable
};

enum ads_Prop_IMec_Elastic_Linear_TransverseShear_moduliEnm
{
    ads_Prop_IMec_Elastic_Linear_TransverseShear_moduli_INSTANTANEOUS,
    ads_Prop_IMec_Elastic_Linear_TransverseShear_moduli_LONG_TERM
};

enum ads_Prop_IMec_Elastic_Linear_TransverseShear_variableEnm
{
    ads_Prop_IMec_Elastic_Linear_TransverseShear_variable_FORCE,
    ads_Prop_IMec_Elastic_Linear_TransverseShear_variable_STRESS
};

/** Enum with association roles. */
enum ads_Prop_IMec_Elastic_Linear_TransverseShear_tableRolesEnm
{
    ads_Prop_IMec_Elastic_Linear_TransverseShear_table_child,
    ads_Prop_IMec_Elastic_Linear_TransverseShear_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Elastic_Linear_UncoupledMembersEnm
{
    ads_Prop_IMec_Elastic_Linear_Uncoupled_moduli,
    ads_Prop_IMec_Elastic_Linear_Uncoupled_noCompression,
    ads_Prop_IMec_Elastic_Linear_Uncoupled_noTension
};

enum ads_Prop_IMec_Elastic_Linear_Uncoupled_moduliEnm
{
    ads_Prop_IMec_Elastic_Linear_Uncoupled_moduli_INSTANTANEOUS,
    ads_Prop_IMec_Elastic_Linear_Uncoupled_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_IMec_Elastic_Linear_Uncoupled_tableRolesEnm
{
    ads_Prop_IMec_Elastic_Linear_Uncoupled_table_child,
    ads_Prop_IMec_Elastic_Linear_Uncoupled_table_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Elastic_Linear_General_bendingBendingRolesEnm
{
    ads_Prop_SMec_Elastic_Linear_General_bendingBending_child,
    ads_Prop_SMec_Elastic_Linear_General_bendingBending_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Elastic_Linear_General_membraneBendingRolesEnm
{
    ads_Prop_SMec_Elastic_Linear_General_membraneBending_child,
    ads_Prop_SMec_Elastic_Linear_General_membraneBending_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Elastic_Linear_General_membraneMembraneRolesEnm
{
    ads_Prop_SMec_Elastic_Linear_General_membraneMembrane_child,
    ads_Prop_SMec_Elastic_Linear_General_membraneMembrane_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Elastic_Linear_General_tableRolesEnm
{
    ads_Prop_SMec_Elastic_Linear_General_table_child,
    ads_Prop_SMec_Elastic_Linear_General_table_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Elastic_Linear_Thickness_tableRolesEnm
{
    ads_Prop_SMec_Elastic_Linear_Thickness_table_child,
    ads_Prop_SMec_Elastic_Linear_Thickness_table_parent
};

/** Enum with association roles. */
enum ads_Prop_SMec_Elastic_Linear_TransverseShear_tableRolesEnm
{
    ads_Prop_SMec_Elastic_Linear_TransverseShear_table_child,
    ads_Prop_SMec_Elastic_Linear_TransverseShear_table_parent
};

#endif
