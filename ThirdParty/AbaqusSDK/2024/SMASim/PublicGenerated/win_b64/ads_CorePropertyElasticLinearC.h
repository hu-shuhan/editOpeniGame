//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyElasticLinearC_h
#define ads_CorePropertyElasticLinearC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyElasticLinear of the latest level of form Core */

#define ads_MMecElasticLinearAnisotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 0))

/** *BEAM GENERAL SECTION */
#define ads_MMecElasticLinearBeamTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 1))

#define ads_MMecElasticLinearBiLaminaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 2))

#define ads_MMecElasticLinearBulkTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 3))

#define ads_MMecElasticLinearEngineeringTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 4))

#define ads_MMecElasticLinearIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 5))

#define ads_MMecElasticLinearLaminaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 6))

#define ads_MMecElasticLinearOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 7))

#define ads_MMecElasticLinearShearTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 8))

#define ads_MMecElasticLinearTransIsotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 9))

/** *ELASTIC, TYPE=WARPING */
#define ads_MMecElasticLinearWarpingTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 10))

#define ads_Prop_MMec_Elastic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 11))

#define ads_Prop_MMec_Elastic_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 12))

/** Anisotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Anisotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 13))

#define ads_Prop_MMec_Elastic_Linear_Anisotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 14))

/** Anisotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Beam (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 15))

#define ads_Prop_MMec_Elastic_Linear_Beam_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 16))

/** BiLamina elasticity record */
#define ads_Prop_MMec_Elastic_Linear_BiLamina (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 17))

#define ads_Prop_MMec_Elastic_Linear_BiLamina_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 18))

#define ads_Prop_MMec_Elastic_Linear_Bulk (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 19))

#define ads_Prop_MMec_Elastic_Linear_Bulk_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 20))

/** EngineeringConstants elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Engineering (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 21))

#define ads_Prop_MMec_Elastic_Linear_Engineering_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 22))

/** Isotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Isotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 23))

#define ads_Prop_MMec_Elastic_Linear_Isotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 24))

/** Lamina elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Lamina (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 25))

#define ads_Prop_MMec_Elastic_Linear_Lamina_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 26))

/** Orthotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Orthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 27))

#define ads_Prop_MMec_Elastic_Linear_Orthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 28))

/** Specify shear behavior for an equation of state material. */
#define ads_Prop_MMec_Elastic_Linear_Shear (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 29))

#define ads_Prop_MMec_Elastic_Linear_Shear_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 30))

/** Anisotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_ShortFiber (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 31))

/** Transversely Isotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_TransIsotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 32))

#define ads_Prop_MMec_Elastic_Linear_TransIsotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 33))

/** Anisotropic elasticity record */
#define ads_Prop_MMec_Elastic_Linear_Warping (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 34))

#define ads_Prop_MMec_Elastic_Linear_Warping_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticLinearFragment, 35))

/** Enum with record members. */
enum ads_Prop_MMec_ElasticMembersEnm
{
    ads_Prop_MMec_Elastic_moduli
};

enum ads_Prop_MMec_Elastic_moduliEnm
{
    ads_Prop_MMec_Elastic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_LinearMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_moduli,
    ads_Prop_MMec_Elastic_Linear_noCompression,
    ads_Prop_MMec_Elastic_Linear_noTension
};

enum ads_Prop_MMec_Elastic_Linear_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_moduli_LONG_TERM
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_AnisotropicMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Anisotropic_moduli,
    ads_Prop_MMec_Elastic_Linear_Anisotropic_noCompression,
    ads_Prop_MMec_Elastic_Linear_Anisotropic_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Anisotropic_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Anisotropic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Anisotropic_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Anisotropic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Anisotropic_table_child,
    ads_Prop_MMec_Elastic_Linear_Anisotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_BeamMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Beam_moduli,
    ads_Prop_MMec_Elastic_Linear_Beam_noCompression,
    ads_Prop_MMec_Elastic_Linear_Beam_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Beam_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Beam_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Beam_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Beam_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Beam_table_child,
    ads_Prop_MMec_Elastic_Linear_Beam_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_BiLaminaMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_BiLamina_moduli,
    ads_Prop_MMec_Elastic_Linear_BiLamina_noCompression,
    ads_Prop_MMec_Elastic_Linear_BiLamina_noTension
};

enum ads_Prop_MMec_Elastic_Linear_BiLamina_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_BiLamina_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_BiLamina_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_BiLamina_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_BiLamina_table_child,
    ads_Prop_MMec_Elastic_Linear_BiLamina_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_BulkMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Bulk_moduli,
    ads_Prop_MMec_Elastic_Linear_Bulk_noCompression,
    ads_Prop_MMec_Elastic_Linear_Bulk_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Bulk_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Bulk_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Bulk_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Bulk_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Bulk_table_child,
    ads_Prop_MMec_Elastic_Linear_Bulk_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_EngineeringMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Engineering_moduli,
    ads_Prop_MMec_Elastic_Linear_Engineering_noCompression,
    ads_Prop_MMec_Elastic_Linear_Engineering_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Engineering_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Engineering_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Engineering_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Engineering_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Engineering_table_child,
    ads_Prop_MMec_Elastic_Linear_Engineering_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_IsotropicMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Isotropic_moduli,
    ads_Prop_MMec_Elastic_Linear_Isotropic_noCompression,
    ads_Prop_MMec_Elastic_Linear_Isotropic_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Isotropic_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Isotropic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Isotropic_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Isotropic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Isotropic_table_child,
    ads_Prop_MMec_Elastic_Linear_Isotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_LaminaMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Lamina_moduli,
    ads_Prop_MMec_Elastic_Linear_Lamina_noCompression,
    ads_Prop_MMec_Elastic_Linear_Lamina_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Lamina_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Lamina_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Lamina_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Lamina_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Lamina_table_child,
    ads_Prop_MMec_Elastic_Linear_Lamina_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_OrthotropicMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Orthotropic_moduli,
    ads_Prop_MMec_Elastic_Linear_Orthotropic_noCompression,
    ads_Prop_MMec_Elastic_Linear_Orthotropic_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Orthotropic_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Orthotropic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Orthotropic_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Orthotropic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Orthotropic_table_child,
    ads_Prop_MMec_Elastic_Linear_Orthotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_ShearMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Shear_moduli,
    ads_Prop_MMec_Elastic_Linear_Shear_noCompression,
    ads_Prop_MMec_Elastic_Linear_Shear_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Shear_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Shear_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Shear_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Shear_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Shear_table_child,
    ads_Prop_MMec_Elastic_Linear_Shear_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_ShortFiberMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_ShortFiber_moduli,
    ads_Prop_MMec_Elastic_Linear_ShortFiber_noCompression,
    ads_Prop_MMec_Elastic_Linear_ShortFiber_noTension
};

enum ads_Prop_MMec_Elastic_Linear_ShortFiber_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_ShortFiber_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_ShortFiber_moduli_LONG_TERM
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_TransIsotropicMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_moduli,
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_noCompression,
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_noTension
};

enum ads_Prop_MMec_Elastic_Linear_TransIsotropic_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_TransIsotropic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_table_child,
    ads_Prop_MMec_Elastic_Linear_TransIsotropic_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Linear_WarpingMembersEnm
{
    ads_Prop_MMec_Elastic_Linear_Warping_moduli,
    ads_Prop_MMec_Elastic_Linear_Warping_noCompression,
    ads_Prop_MMec_Elastic_Linear_Warping_noTension
};

enum ads_Prop_MMec_Elastic_Linear_Warping_moduliEnm
{
    ads_Prop_MMec_Elastic_Linear_Warping_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Linear_Warping_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Linear_Warping_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Linear_Warping_table_child,
    ads_Prop_MMec_Elastic_Linear_Warping_table_parent
};

#endif
