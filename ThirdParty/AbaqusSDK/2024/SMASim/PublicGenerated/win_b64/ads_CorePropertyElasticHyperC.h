//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyElasticHyperC_h
#define ads_CorePropertyElasticHyperC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyElasticHyper of the latest level of form Core */

/** Data to define arruda boyce material */
#define ads_MMecElasticHyperArrudaBoyceTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 0))

#define ads_MMecElasticHyperFungAnisotopicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 1))

#define ads_MMecElasticHyperFungOrthotropicTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 2))

#define ads_MMecElasticHyperHolzapfelOgdenTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 3))

#define ads_MMecElasticHyperHolzapfelTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 4))

#define ads_MMecElasticHyperKaliskeSchmidtTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 5))

/** Data to define ogden material */
#define ads_MMecElasticHyperOgden1Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 6))

/** Data to define ogden material */
#define ads_MMecElasticHyperOgden2Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 7))

/** Data to define ogden material */
#define ads_MMecElasticHyperOgden3Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 8))

/** Data to define ogden material */
#define ads_MMecElasticHyperOgden4Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 9))

/** Data to define ogden material */
#define ads_MMecElasticHyperOgden5Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 10))

/** Data to define ogden material */
#define ads_MMecElasticHyperOgden6Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 11))

#define ads_MMecElasticHyperOgdenFoam1Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 12))

#define ads_MMecElasticHyperOgdenFoam2Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 13))

#define ads_MMecElasticHyperOgdenFoam3Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 14))

#define ads_MMecElasticHyperOgdenFoam4Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 15))

#define ads_MMecElasticHyperOgdenFoam5Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 16))

#define ads_MMecElasticHyperOgdenFoam6Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 17))

/** Data to define ParameterMullinsEffect */
#define ads_MMecElasticHyperOptionMullinsOgdenRoxburghTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 18))

/** Data to define polynomial material */
#define ads_MMecElasticHyperPoly1Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 19))

/** Data to define polynomial material */
#define ads_MMecElasticHyperPoly2Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 20))

/** Data to define polynomial material */
#define ads_MMecElasticHyperPoly3Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 21))

/** Data to define polynomial material */
#define ads_MMecElasticHyperPoly4Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 22))

/** Data to define polynomial material */
#define ads_MMecElasticHyperPoly5Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 23))

/** Data to define polynomial material */
#define ads_MMecElasticHyperPoly6Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 24))

/** Data to define reduced polynomial material */
#define ads_MMecElasticHyperReducedPoly1Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 25))

/** Data to define reduced polynomial material */
#define ads_MMecElasticHyperReducedPoly2Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 26))

/** Data to define reduced polynomial material */
#define ads_MMecElasticHyperReducedPoly3Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 27))

/** Data to define reduced polynomial material */
#define ads_MMecElasticHyperReducedPoly4Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 28))

/** Data to define reduced polynomial material */
#define ads_MMecElasticHyperReducedPoly5Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 29))

/** Data to define reduced polynomial material */
#define ads_MMecElasticHyperReducedPoly6Table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 30))

/** Data to define reduced VanDerWaalsHyperelastic material */
#define ads_MMecElasticHyperVanDerWaalsTable (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 31))

/** An abstraction of all available hyperelastic materials. */
#define ads_Prop_MMec_Elastic_Hyper (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 32))

#define ads_Prop_MMec_Elastic_HyperOption (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 33))

#define ads_Prop_MMec_Elastic_HyperOption_Mullins (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 34))

/** Specify parameters to describe mullins effect in a material. */
#define ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 35))

#define ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 36))

/** user defined mullins effect. */
#define ads_Prop_MMec_Elastic_HyperOption_Mullins_User (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 37))

/** User defined anisotropic hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Anisotropic_User (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 38))

/** Arruda-Boyce hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 39))

#define ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 40))

#define ads_Prop_MMec_Elastic_Hyper_FungAnisotopic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 41))

#define ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 42))

#define ads_Prop_MMec_Elastic_Hyper_FungOrthotropic (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 43))

#define ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 44))

#define ads_Prop_MMec_Elastic_Hyper_Holzapfel (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 45))

#define ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 46))

#define ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 47))

#define ads_Prop_MMec_Elastic_Hyper_Holzapfel_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 48))

#define ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 49))

#define ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 50))

#define ads_Prop_MMec_Elastic_Hyper_LowDensity (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 51))

/** MarlowHyperelastic hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Marlow (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 52))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 53))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 54))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 55))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 56))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 57))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 58))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 59))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 60))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 61))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 62))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 63))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 64))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 65))

#define ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 66))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden_1 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 67))

#define ads_Prop_MMec_Elastic_Hyper_Ogden_1_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 68))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden_2 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 69))

#define ads_Prop_MMec_Elastic_Hyper_Ogden_2_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 70))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden_3 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 71))

#define ads_Prop_MMec_Elastic_Hyper_Ogden_3_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 72))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden_4 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 73))

#define ads_Prop_MMec_Elastic_Hyper_Ogden_4_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 74))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden_5 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 75))

#define ads_Prop_MMec_Elastic_Hyper_Ogden_5_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 76))

/** Ogden hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Ogden_6 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 77))

#define ads_Prop_MMec_Elastic_Hyper_Ogden_6_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 78))

/** MooneyRivlin, Polynomial hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Poly1 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 79))

#define ads_Prop_MMec_Elastic_Hyper_Poly1_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 80))

/** Polynomial hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Poly2 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 81))

#define ads_Prop_MMec_Elastic_Hyper_Poly2_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 82))

/** Polynomial hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Poly3 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 83))

#define ads_Prop_MMec_Elastic_Hyper_Poly3_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 84))

/** Polynomial hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Poly4 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 85))

#define ads_Prop_MMec_Elastic_Hyper_Poly4_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 86))

/** Polynomial hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Poly5 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 87))

#define ads_Prop_MMec_Elastic_Hyper_Poly5_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 88))

/** Polynomial hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_Poly6 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 89))

#define ads_Prop_MMec_Elastic_Hyper_Poly6_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 90))

/** NeoHooke, ReducedPolynomial hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly1 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 91))

#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 92))

/** ReducedPolynomial hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly2 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 93))

#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 94))

/** Yeoh, ReducedPolynomial hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly3 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 95))

#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 96))

/** ReducedPolynomial hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly4 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 97))

#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 98))

/** ReducedPolynomial hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly5 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 99))

#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 100))

/** ReducedPolynomial hyperelasticity model record */
#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly6 (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 101))

#define ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 102))

/** user defined hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_User (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 103))

/** ValanisLandelHyperelastic hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_ValanisLandel (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 104))

/** van der waals hyperelasticity model record. */
#define ads_Prop_MMec_Elastic_Hyper_VanDerWaals (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 105))

#define ads_Prop_MMec_Elastic_Hyper_VanDerWaals_table (ads_CoreFragmentTypeIndex(ads_CorePropertyElasticHyperFragment, 106))

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_HyperMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_moduli,
    ads_Prop_MMec_Elastic_Hyper_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_poissonType_USER_INPUT
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_HyperOptionMembersEnm
{
    ads_Prop_MMec_Elastic_HyperOption_moduli
};

enum ads_Prop_MMec_Elastic_HyperOption_moduliEnm
{
    ads_Prop_MMec_Elastic_HyperOption_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_HyperOption_moduli_LONG_TERM
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_HyperOption_MullinsMembersEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_moduli
};

enum ads_Prop_MMec_Elastic_HyperOption_Mullins_moduliEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_HyperOption_Mullins_moduli_LONG_TERM
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburghMembersEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_moduli
};

enum ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_moduliEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_moduli_LONG_TERM
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_tableRolesEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_table_child,
    ads_Prop_MMec_Elastic_HyperOption_Mullins_OgdenRoxburgh_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_HyperOption_Mullins_UserMembersEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_User_moduli
};

enum ads_Prop_MMec_Elastic_HyperOption_Mullins_User_moduliEnm
{
    ads_Prop_MMec_Elastic_HyperOption_Mullins_User_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_HyperOption_Mullins_User_moduli_LONG_TERM
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Anisotropic_UserMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_moduli,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_poissonType,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_formulation,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_localDirections,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_type
};

enum ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_poissonType_USER_INPUT
};

enum ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_formulationEnm
{
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_formulation_INVARIANT,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_formulation_STRAIN
};

enum ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_typeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_type_COMPRESSIBLE,
    ads_Prop_MMec_Elastic_Hyper_Anisotropic_User_type_INCOMPRESSIBLE
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ArrudaBoyceMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_moduli,
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_table_child,
    ads_Prop_MMec_Elastic_Hyper_ArrudaBoyce_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_FungAnisotopicMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_moduli,
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_table_child,
    ads_Prop_MMec_Elastic_Hyper_FungAnisotopic_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_FungOrthotropicMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_moduli,
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_table_child,
    ads_Prop_MMec_Elastic_Hyper_FungOrthotropic_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_HolzapfelMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_moduli,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_poissonType,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_localDirections
};

enum ads_Prop_MMec_Elastic_Hyper_Holzapfel_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Holzapfel_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_poissonType_USER_INPUT
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_HolzapfelOgdenMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_moduli,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_poissonType,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_localDirections
};

enum ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_table_child,
    ads_Prop_MMec_Elastic_Hyper_HolzapfelOgden_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Holzapfel_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_table_child,
    ads_Prop_MMec_Elastic_Hyper_Holzapfel_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidtMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_moduli,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_poissonType,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_localDirections
};

enum ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_table_child,
    ads_Prop_MMec_Elastic_Hyper_KaliskeSchmidt_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_LowDensityMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_LowDensity_moduli,
    ads_Prop_MMec_Elastic_Hyper_LowDensity_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_LowDensity_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_LowDensity_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_LowDensity_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_LowDensity_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_LowDensity_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_LowDensity_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_LowDensity_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_LowDensity_poissonType_USER_INPUT
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_MarlowMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Marlow_moduli,
    ads_Prop_MMec_Elastic_Hyper_Marlow_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Marlow_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Marlow_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Marlow_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Marlow_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Marlow_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Marlow_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Marlow_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Marlow_poissonType_USER_INPUT
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_poissonType_USER_INPUT
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoamMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_poissonType_USER_INPUT
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_table_child,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_1_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_table_child,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_2_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_table_child,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_3_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_table_child,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_4_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_table_child,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_5_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_moduli,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_table_child,
    ads_Prop_MMec_Elastic_Hyper_OgdenFoam_6_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_1MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_1_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_1_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_1_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_table_child,
    ads_Prop_MMec_Elastic_Hyper_Ogden_1_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_2MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_2_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_2_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_2_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_table_child,
    ads_Prop_MMec_Elastic_Hyper_Ogden_2_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_3MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_3_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_3_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_3_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_table_child,
    ads_Prop_MMec_Elastic_Hyper_Ogden_3_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_4MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_4_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_4_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_4_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_table_child,
    ads_Prop_MMec_Elastic_Hyper_Ogden_4_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_5MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_5_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_5_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_5_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_table_child,
    ads_Prop_MMec_Elastic_Hyper_Ogden_5_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_6MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_moduli,
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_6_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Ogden_6_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Ogden_6_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_table_child,
    ads_Prop_MMec_Elastic_Hyper_Ogden_6_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Poly1MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly1_moduli,
    ads_Prop_MMec_Elastic_Hyper_Poly1_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Poly1_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Poly1_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Poly1_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly1_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Poly1_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Poly1_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly1_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Poly1_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Poly1_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly1_table_child,
    ads_Prop_MMec_Elastic_Hyper_Poly1_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Poly2MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly2_moduli,
    ads_Prop_MMec_Elastic_Hyper_Poly2_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Poly2_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Poly2_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Poly2_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly2_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Poly2_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Poly2_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly2_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Poly2_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Poly2_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly2_table_child,
    ads_Prop_MMec_Elastic_Hyper_Poly2_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Poly3MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly3_moduli,
    ads_Prop_MMec_Elastic_Hyper_Poly3_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Poly3_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Poly3_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Poly3_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly3_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Poly3_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Poly3_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly3_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Poly3_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Poly3_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly3_table_child,
    ads_Prop_MMec_Elastic_Hyper_Poly3_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Poly4MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly4_moduli,
    ads_Prop_MMec_Elastic_Hyper_Poly4_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Poly4_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Poly4_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Poly4_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly4_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Poly4_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Poly4_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly4_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Poly4_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Poly4_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly4_table_child,
    ads_Prop_MMec_Elastic_Hyper_Poly4_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Poly5MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly5_moduli,
    ads_Prop_MMec_Elastic_Hyper_Poly5_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Poly5_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Poly5_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Poly5_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly5_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Poly5_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Poly5_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly5_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Poly5_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Poly5_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly5_table_child,
    ads_Prop_MMec_Elastic_Hyper_Poly5_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_Poly6MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly6_moduli,
    ads_Prop_MMec_Elastic_Hyper_Poly6_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_Poly6_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_Poly6_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_Poly6_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly6_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_Poly6_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_Poly6_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly6_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_Poly6_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_Poly6_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_Poly6_table_child,
    ads_Prop_MMec_Elastic_Hyper_Poly6_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly1MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_moduli,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_table_child,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly1_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly2MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_moduli,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_table_child,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly2_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly3MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_moduli,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_table_child,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly3_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly4MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_moduli,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_table_child,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly4_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly5MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_moduli,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_table_child,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly5_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly6MembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_moduli,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_poissonType_USER_INPUT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_table_child,
    ads_Prop_MMec_Elastic_Hyper_ReducedPoly6_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_UserMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_User_moduli,
    ads_Prop_MMec_Elastic_Hyper_User_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_User_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_User_poissonType,
    ads_Prop_MMec_Elastic_Hyper_User_type
};

enum ads_Prop_MMec_Elastic_Hyper_User_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_User_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_User_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_User_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_User_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_User_poissonType_USER_INPUT
};

enum ads_Prop_MMec_Elastic_Hyper_User_typeEnm
{
    ads_Prop_MMec_Elastic_Hyper_User_type_COMPRESSIBLE,
    ads_Prop_MMec_Elastic_Hyper_User_type_INCOMPRESSIBLE
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_ValanisLandelMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_moduli,
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_poissonType
};

enum ads_Prop_MMec_Elastic_Hyper_ValanisLandel_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_ValanisLandel_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_ValanisLandel_poissonType_USER_INPUT
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Elastic_Hyper_VanDerWaalsMembersEnm
{
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_moduli,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_linearizationStretch,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_poissonRatio,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_poissonType,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_beta,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_betaEnm
};

enum ads_Prop_MMec_Elastic_Hyper_VanDerWaals_moduliEnm
{
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_moduli_INSTANTANEOUS,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_moduli_LONG_TERM
};

enum ads_Prop_MMec_Elastic_Hyper_VanDerWaals_poissonTypeEnm
{
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_poissonType_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_poissonType_USER_INPUT
};

enum ads_Prop_MMec_Elastic_Hyper_VanDerWaals_betaEnmEnm
{
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_betaEnm_FIT_FROM_TEST_DATA,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_betaEnm_USER_SPECIFIED
};

/** Enum with association roles. */
enum ads_Prop_MMec_Elastic_Hyper_VanDerWaals_tableRolesEnm
{
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_table_child,
    ads_Prop_MMec_Elastic_Hyper_VanDerWaals_table_parent
};

#endif
