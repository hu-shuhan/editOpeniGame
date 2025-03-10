//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyViscoC_h
#define ads_CorePropertyViscoC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyVisco of the latest level of form Core */

#define ads_CMecViscosityLinearCoupledSymmTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 0))

#define ads_CMecViscosityLinearCoupledUnsymmTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 1))

#define ads_CMecViscosityLinearRotationalUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 2))

#define ads_CMecViscosityLinearTranslationalUncoupledTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 3))

#define ads_CMecViscosityNonlinearRotationalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 4))

#define ads_CMecViscosityNonlinearTranslationalTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 5))

/** Data to define GasketUniaxialPreloadFrequencyTabularViscoelastic */
#define ads_IMecViscoelasticLinearFrequencyClosureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 6))

/** Data to define GasketFrequencyTabularViscoelastic */
#define ads_IMecViscoelasticLinearFrequencyTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 7))

#define ads_IMecViscosityClosureTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 8))

#define ads_IPoreFluidViscosityNewtonianTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 9))

#define ads_IPoreFluidViscosityPowerLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 10))

#define ads_MMecAnandTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 11))

#define ads_MMecDarveauxTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 12))

#define ads_MMecDoublePowerTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 13))

#define ads_MMecPowerLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 14))

#define ads_MMecViscoOptionTRSArrheniusTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 15))

#define ads_MMecViscoOptionTRSTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 16))

#define ads_MMecViscoOptionTRSWLFTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 17))

/** Data to define FrequencyTabularViscoelastic */
#define ads_MMecViscoelasticLinearFrequencyTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 18))

/** Data to define UniaxialPreloadFrequencyTabularViscoelastic */
#define ads_MMecViscoelasticLinearFrequencyUniaxialStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 19))

/** Data to define VolumetricPreloadFrequencyTabularViscoelastic */
#define ads_MMecViscoelasticLinearFrequencyVolumetricStrainTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 20))

#define ads_MMecViscoelasticNonlinearBBPowerLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 21))

#define ads_MMecViscoelasticNonlinearBergstromBoyceLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 22))

#define ads_MMecViscoelasticNonlinearHyperbLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 23))

#define ads_MMecViscoelasticNonlinearPowerLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 24))

#define ads_MMecViscoelasticNonlinearStrainLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 25))

#define ads_MMecViscoelasticNonlinearUserTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 26))

#define ads_MMecViscoelasticViscousTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 27))

#define ads_MMecViscoelasticViscousUserTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 28))

#define ads_MMecViscosityCarreauYasudaTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 29))

#define ads_MMecViscosityCrossTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 30))

#define ads_MMecViscosityEllisMeterTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 31))

#define ads_MMecViscosityHerschelBulkeyTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 32))

#define ads_MMecViscosityNewtonianTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 33))

#define ads_MMecViscosityPowellEyringTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 34))

#define ads_MMecViscosityPowerLawTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 35))

#define ads_MMecViscosityTabularTable (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 36))

/** A single prony series parameter to define the frequency domain response. */
#define ads_PronySeriesTerm (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 37))

#define ads_Prop_CMec_Viscosity (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 38))

#define ads_Prop_CMec_Viscosity_Linear (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 39))

#define ads_Prop_CMec_Viscosity_Linear_CoupledSymm (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 40))

#define ads_Prop_CMec_Viscosity_Linear_CoupledSymm_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 41))

#define ads_Prop_CMec_Viscosity_Linear_CoupledUnsymm (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 42))

#define ads_Prop_CMec_Viscosity_Linear_CoupledUnsymm_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 43))

#define ads_Prop_CMec_Viscosity_Linear_RotationalUncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 44))

#define ads_Prop_CMec_Viscosity_Linear_RotationalUncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 45))

#define ads_Prop_CMec_Viscosity_Linear_TranslationalUncoupled (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 46))

#define ads_Prop_CMec_Viscosity_Linear_TranslationalUncoupled_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 47))

#define ads_Prop_CMec_Viscosity_Nonlinear (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 48))

#define ads_Prop_CMec_Viscosity_Nonlinear_Rotational (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 49))

#define ads_Prop_CMec_Viscosity_Nonlinear_Rotational_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 50))

#define ads_Prop_CMec_Viscosity_Nonlinear_Translational (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 51))

#define ads_Prop_CMec_Viscosity_Nonlinear_Translational_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 52))

#define ads_Prop_IMec_Viscoelastic (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 53))

#define ads_Prop_IMec_Viscoelastic_LinearFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 54))

/** Specify dissipative behavior using tabular definition of the frequency domain response for gaskets. The frequency-domain viscoelastic material properties correspond to a uniaxial test. */
#define ads_Prop_IMec_Viscoelastic_LinearFrequency_Closure (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 55))

#define ads_Prop_IMec_Viscoelastic_LinearFrequency_Closure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 56))

/** Specify dissipative behavior using tabular definition of the frequency domain response for gaskets. */
#define ads_Prop_IMec_Viscoelastic_LinearFrequency_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 57))

#define ads_Prop_IMec_Viscoelastic_LinearFrequency_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 58))

#define ads_Prop_IMec_Viscosity (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 59))

#define ads_Prop_IMec_Viscosity_Closure (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 60))

#define ads_Prop_IMec_Viscosity_Closure_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 61))

#define ads_Prop_IPoreFluid_Viscosity (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 62))

/** This option defines tangential flow consititutive parameters for pore pressure cohesive elements. */
#define ads_Prop_IPoreFluid_Viscosity_Newtonian (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 63))

#define ads_Prop_IPoreFluid_Viscosity_Newtonian_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 64))

/** This option defines tangential flow consititutive parameters for pore pressure cohesive elements. */
#define ads_Prop_IPoreFluid_Viscosity_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 65))

#define ads_Prop_IPoreFluid_Viscosity_PowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 66))

#define ads_Prop_MMec_CreepOption_Anand_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 67))

#define ads_Prop_MMec_CreepOption_Darveaux_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 68))

#define ads_Prop_MMec_CreepOption_DoublePower_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 69))

#define ads_Prop_MMec_CreepOption_PowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 70))

#define ads_Prop_MMec_CreepOption_TimePowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 71))

#define ads_Prop_MMec_ViscoOption (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 72))

#define ads_Prop_MMec_ViscoOption_TRS (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 73))

#define ads_Prop_MMec_ViscoOption_TRS_Arrhenius (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 74))

#define ads_Prop_MMec_ViscoOption_TRS_Arrhenius_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 75))

#define ads_Prop_MMec_ViscoOption_TRS_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 76))

#define ads_Prop_MMec_ViscoOption_TRS_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 77))

#define ads_Prop_MMec_ViscoOption_TRS_User (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 78))

#define ads_Prop_MMec_ViscoOption_TRS_WLF (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 79))

#define ads_Prop_MMec_ViscoOption_TRS_WLF_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 80))

#define ads_Prop_MMec_Viscoelastic (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 81))

#define ads_Prop_MMec_Viscoelastic_LinearFrequency (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 82))

/** Define the dissipative material parameters by the power law formulae for continuum elements. */
#define ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 83))

/** Gathering datatype for frequency prony series terms */
#define ads_Prop_MMec_Viscoelastic_LinearFrequency_Prony (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 84))

#define ads_Prop_MMec_Viscoelastic_LinearFrequency_Prony_pronySeriesTerm (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 85))

/** Specify dissipative behavior using tabular definition of the frequency domain response for continuum elements. An alternative approach for specifying the viscoelastic properties of hyperelastic and hyperfoam materials involves the direct (tabular) specification of storage and loss moduli from uniaxial and volumetric tests, as functions of excitation frequency and a measure of the level of pre-strain. */
#define ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 86))

#define ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 87))

/** Specify dissipative behavior using tabular definition of the frequency domain response for continuum elements. The frequency-domain viscoelastic material properties correspond to a uniaxial test. */
#define ads_Prop_MMec_Viscoelastic_LinearFrequency_UniaxialStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 88))

#define ads_Prop_MMec_Viscoelastic_LinearFrequency_UniaxialStrain_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 89))

/** Specify dissipative behavior using tabular definition of the frequency domain response for continuum elements. The frequency-domain viscoelastic material properties correspond to a volumetric test. This setting is not meaningful when used with gasket elements to define effective thickness-direction properties. */
#define ads_Prop_MMec_Viscoelastic_LinearFrequency_VolumetricStrain (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 90))

#define ads_Prop_MMec_Viscoelastic_LinearFrequency_VolumetricStrain_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 91))

#define ads_Prop_MMec_Viscoelastic_LinearTime (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 92))

/** Gathering datatype for the prony series terms */
#define ads_Prop_MMec_Viscoelastic_LinearTime_Prony (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 93))

#define ads_Prop_MMec_Viscoelastic_LinearTime_Prony_pronySeriesTerm (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 94))

#define ads_Prop_MMec_Viscoelastic_Nonlinear (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 95))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 96))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 97))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 98))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 99))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 100))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 101))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 102))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 103))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 104))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 105))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_User (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 106))

#define ads_Prop_MMec_Viscoelastic_Nonlinear_User_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 107))

#define ads_Prop_MMec_Viscoelastic_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 108))

#define ads_Prop_MMec_Viscoelastic_PowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 109))

#define ads_Prop_MMec_Viscoelastic_TimePowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 110))

#define ads_Prop_MMec_Viscoelastic_TimePowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 111))

#define ads_Prop_MMec_Viscoelastic_Viscous (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 112))

#define ads_Prop_MMec_Viscoelastic_ViscousAnand (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 113))

#define ads_Prop_MMec_Viscoelastic_ViscousAnand_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 114))

#define ads_Prop_MMec_Viscoelastic_ViscousDarveaux (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 115))

#define ads_Prop_MMec_Viscoelastic_ViscousDarveaux_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 116))

#define ads_Prop_MMec_Viscoelastic_ViscousDoublePower (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 117))

#define ads_Prop_MMec_Viscoelastic_ViscousDoublePower_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 118))

#define ads_Prop_MMec_Viscoelastic_ViscousUser (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 119))

#define ads_Prop_MMec_Viscoelastic_ViscousUser_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 120))

#define ads_Prop_MMec_Viscoelastic_Viscous_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 121))

#define ads_Prop_MMec_Viscosity (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 122))

/** Carreau-Yasuda viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_CarreauYasuda (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 123))

#define ads_Prop_MMec_Viscosity_CarreauYasuda_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 124))

/** Cross viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_Cross (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 125))

#define ads_Prop_MMec_Viscosity_Cross_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 126))

/** Ellis-Meter viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_EllisMeter (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 127))

#define ads_Prop_MMec_Viscosity_EllisMeter_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 128))

/** Herschel-Bulkey viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_HerschelBulkey (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 129))

#define ads_Prop_MMec_Viscosity_HerschelBulkey_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 130))

/** Specify shear behavior for an equation of state material. */
#define ads_Prop_MMec_Viscosity_Newtonian (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 131))

#define ads_Prop_MMec_Viscosity_Newtonian_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 132))

/** Powell-Eyring viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_PowellEyring (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 133))

#define ads_Prop_MMec_Viscosity_PowellEyring_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 134))

/** Power law viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_PowerLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 135))

#define ads_Prop_MMec_Viscosity_PowerLaw_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 136))

/** Sutherlands law viscosity */
#define ads_Prop_MMec_Viscosity_SutherlandsLaw (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 137))

/** Tabular viscous shear behavior record */
#define ads_Prop_MMec_Viscosity_Tabular (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 138))

#define ads_Prop_MMec_Viscosity_Tabular_table (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 139))

#define ads_Prop_MMec_Viscosity_User (ads_CoreFragmentTypeIndex(ads_CorePropertyViscoFragment, 140))

/** 
Enum with record members. */
enum ads_PronySeriesTermMembersEnm
{
    ads_PronySeriesTerm_g1,
    ads_PronySeriesTerm_k1,
    ads_PronySeriesTerm_tau1
};

/** Enum with record members. */
enum ads_Prop_CMec_Viscosity_Linear_CoupledSymmMembersEnm
{
    ads_Prop_CMec_Viscosity_Linear_CoupledSymm_frequencyDependence
};

/** Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Linear_CoupledSymm_tableRolesEnm
{
    ads_Prop_CMec_Viscosity_Linear_CoupledSymm_table_child,
    ads_Prop_CMec_Viscosity_Linear_CoupledSymm_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_Viscosity_Linear_CoupledUnsymmMembersEnm
{
    ads_Prop_CMec_Viscosity_Linear_CoupledUnsymm_frequencyDependence
};

/** Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Linear_CoupledUnsymm_tableRolesEnm
{
    ads_Prop_CMec_Viscosity_Linear_CoupledUnsymm_table_child,
    ads_Prop_CMec_Viscosity_Linear_CoupledUnsymm_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Linear_RotationalUncoupled_tableRolesEnm
{
    ads_Prop_CMec_Viscosity_Linear_RotationalUncoupled_table_child,
    ads_Prop_CMec_Viscosity_Linear_RotationalUncoupled_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Linear_TranslationalUncoupled_tableRolesEnm
{
    ads_Prop_CMec_Viscosity_Linear_TranslationalUncoupled_table_child,
    ads_Prop_CMec_Viscosity_Linear_TranslationalUncoupled_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_Viscosity_NonlinearMembersEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_indepCompEnm
};

enum ads_Prop_CMec_Viscosity_Nonlinear_indepCompEnmEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Viscosity_Nonlinear_indepCompEnm_NONE,
    ads_Prop_CMec_Viscosity_Nonlinear_indepCompEnm_POSITION
};

/** Enum with record members. */
enum ads_Prop_CMec_Viscosity_Nonlinear_RotationalMembersEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_indepCompEnm
};

enum ads_Prop_CMec_Viscosity_Nonlinear_Rotational_indepCompEnmEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_indepCompEnm_NONE,
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_indepCompEnm_POSITION
};

/** Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Nonlinear_Rotational_tableRolesEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_table_child,
    ads_Prop_CMec_Viscosity_Nonlinear_Rotational_table_parent
};

/** Enum with record members. */
enum ads_Prop_CMec_Viscosity_Nonlinear_TranslationalMembersEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_indepCompEnm
};

enum ads_Prop_CMec_Viscosity_Nonlinear_Translational_indepCompEnmEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_indepCompEnm_CONSTITUTIVE_MOTION,
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_indepCompEnm_NONE,
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_indepCompEnm_POSITION
};

/** Enum with association roles. */
enum ads_Prop_CMec_Viscosity_Nonlinear_Translational_tableRolesEnm
{
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_table_child,
    ads_Prop_CMec_Viscosity_Nonlinear_Translational_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Viscoelastic_LinearFrequency_Closure_tableRolesEnm
{
    ads_Prop_IMec_Viscoelastic_LinearFrequency_Closure_table_child,
    ads_Prop_IMec_Viscoelastic_LinearFrequency_Closure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IMec_Viscoelastic_LinearFrequency_Tabular_tableRolesEnm
{
    ads_Prop_IMec_Viscoelastic_LinearFrequency_Tabular_table_child,
    ads_Prop_IMec_Viscoelastic_LinearFrequency_Tabular_table_parent
};

/** Enum with record members. */
enum ads_Prop_IMec_Viscosity_ClosureMembersEnm
{
    ads_Prop_IMec_Viscosity_Closure_tangentFraction
};

/** Enum with association roles. */
enum ads_Prop_IMec_Viscosity_Closure_tableRolesEnm
{
    ads_Prop_IMec_Viscosity_Closure_table_child,
    ads_Prop_IMec_Viscosity_Closure_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_IPoreFluid_Viscosity_NewtonianMembersEnm
{
    ads_Prop_IPoreFluid_Viscosity_Newtonian_kmax,
    ads_Prop_IPoreFluid_Viscosity_Newtonian_kmaxEnm
};

enum ads_Prop_IPoreFluid_Viscosity_Newtonian_kmaxEnmEnm
{
    ads_Prop_IPoreFluid_Viscosity_Newtonian_kmaxEnm_UNBOUNDED,
    ads_Prop_IPoreFluid_Viscosity_Newtonian_kmaxEnm_USER_SPECIFIED
};

/** Enum with association roles. */
enum ads_Prop_IPoreFluid_Viscosity_Newtonian_tableRolesEnm
{
    ads_Prop_IPoreFluid_Viscosity_Newtonian_table_child,
    ads_Prop_IPoreFluid_Viscosity_Newtonian_table_parent
};

/** Enum with association roles. */
enum ads_Prop_IPoreFluid_Viscosity_PowerLaw_tableRolesEnm
{
    ads_Prop_IPoreFluid_Viscosity_PowerLaw_table_child,
    ads_Prop_IPoreFluid_Viscosity_PowerLaw_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_Anand_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_Anand_table_child,
    ads_Prop_MMec_CreepOption_Anand_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_Darveaux_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_Darveaux_table_child,
    ads_Prop_MMec_CreepOption_Darveaux_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_DoublePower_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_DoublePower_table_child,
    ads_Prop_MMec_CreepOption_DoublePower_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_PowerLaw_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_PowerLaw_table_child,
    ads_Prop_MMec_CreepOption_PowerLaw_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_CreepOption_TimePowerLaw_tableRolesEnm
{
    ads_Prop_MMec_CreepOption_TimePowerLaw_table_child,
    ads_Prop_MMec_CreepOption_TimePowerLaw_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ViscoOption_TRS_Arrhenius_tableRolesEnm
{
    ads_Prop_MMec_ViscoOption_TRS_Arrhenius_table_child,
    ads_Prop_MMec_ViscoOption_TRS_Arrhenius_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ViscoOption_TRS_Tabular_tableRolesEnm
{
    ads_Prop_MMec_ViscoOption_TRS_Tabular_table_child,
    ads_Prop_MMec_ViscoOption_TRS_Tabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_ViscoOption_TRS_WLF_tableRolesEnm
{
    ads_Prop_MMec_ViscoOption_TRS_WLF_table_child,
    ads_Prop_MMec_ViscoOption_TRS_WLF_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_a,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_b,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_bType,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_imagK1,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_imagK1Type,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_imaginaryG1,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_realG1,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_realK1,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_realK1Type
};

enum ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_bTypeEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_bType_ABSENT,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_bType_PRESENT
};

enum ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_imagK1TypeEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_imagK1Type_ABSENT,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_imagK1Type_PRESENT
};

enum ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_realK1TypeEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_realK1Type_ABSENT,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_PowerLaw_realK1Type_PRESENT
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_LinearFrequency_Prony_pronySeriesTermRolesEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Prony_pronySeriesTerm_child,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Prony_pronySeriesTerm_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_LinearFrequency_TabularMembersEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_errtol,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_nmax,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_time
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_table_child,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_Tabular_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_LinearFrequency_UniaxialStrain_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_UniaxialStrain_table_child,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_UniaxialStrain_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_LinearFrequency_VolumetricStrain_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_LinearFrequency_VolumetricStrain_table_child,
    ads_Prop_MMec_Viscoelastic_LinearFrequency_VolumetricStrain_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_LinearTime_Prony_pronySeriesTermRolesEnm
{
    ads_Prop_MMec_Viscoelastic_LinearTime_Prony_pronySeriesTerm_child,
    ads_Prop_MMec_Viscoelastic_LinearTime_Prony_pronySeriesTerm_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_NonlinearMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_stiffnessRatio
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw_stiffnessRatio
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw_table_child,
    ads_Prop_MMec_Viscoelastic_Nonlinear_BBPowerLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw_stiffnessRatio
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw_table_child,
    ads_Prop_MMec_Viscoelastic_Nonlinear_BergstromBoyceLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw_stiffnessRatio
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw_table_child,
    ads_Prop_MMec_Viscoelastic_Nonlinear_HyperbLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw_stiffnessRatio
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw_table_child,
    ads_Prop_MMec_Viscoelastic_Nonlinear_PowerLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw_stiffnessRatio
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw_table_child,
    ads_Prop_MMec_Viscoelastic_Nonlinear_StrainLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_UserMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_User_networkID,
    ads_Prop_MMec_Viscoelastic_Nonlinear_User_stiffnessRatio
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Nonlinear_User_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Nonlinear_User_table_child,
    ads_Prop_MMec_Viscoelastic_Nonlinear_User_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_PowerLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_PowerLaw_table_child,
    ads_Prop_MMec_Viscoelastic_PowerLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_TimePowerLawMembersEnm
{
    ads_Prop_MMec_Viscoelastic_TimePowerLaw_time
};

enum ads_Prop_MMec_Viscoelastic_TimePowerLaw_timeEnm
{
    ads_Prop_MMec_Viscoelastic_TimePowerLaw_time_CREEP,
    ads_Prop_MMec_Viscoelastic_TimePowerLaw_time_TOTAL
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_TimePowerLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_TimePowerLaw_table_child,
    ads_Prop_MMec_Viscoelastic_TimePowerLaw_table_parent
};

/** Enum with record members. */
enum ads_Prop_MMec_Viscoelastic_ViscousMembersEnm
{
    ads_Prop_MMec_Viscoelastic_Viscous_lawEnm
};

enum ads_Prop_MMec_Viscoelastic_Viscous_lawEnmEnm
{
    ads_Prop_MMec_Viscoelastic_Viscous_lawEnm_STRAIN,
    ads_Prop_MMec_Viscoelastic_Viscous_lawEnm_TIME
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_ViscousAnand_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_ViscousAnand_table_child,
    ads_Prop_MMec_Viscoelastic_ViscousAnand_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_ViscousDarveaux_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_ViscousDarveaux_table_child,
    ads_Prop_MMec_Viscoelastic_ViscousDarveaux_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_ViscousDoublePower_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_ViscousDoublePower_table_child,
    ads_Prop_MMec_Viscoelastic_ViscousDoublePower_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_ViscousUser_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_ViscousUser_table_child,
    ads_Prop_MMec_Viscoelastic_ViscousUser_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscoelastic_Viscous_tableRolesEnm
{
    ads_Prop_MMec_Viscoelastic_Viscous_table_child,
    ads_Prop_MMec_Viscoelastic_Viscous_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_CarreauYasuda_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_CarreauYasuda_table_child,
    ads_Prop_MMec_Viscosity_CarreauYasuda_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_Cross_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_Cross_table_child,
    ads_Prop_MMec_Viscosity_Cross_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_EllisMeter_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_EllisMeter_table_child,
    ads_Prop_MMec_Viscosity_EllisMeter_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_HerschelBulkey_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_HerschelBulkey_table_child,
    ads_Prop_MMec_Viscosity_HerschelBulkey_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_Newtonian_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_Newtonian_table_child,
    ads_Prop_MMec_Viscosity_Newtonian_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_PowellEyring_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_PowellEyring_table_child,
    ads_Prop_MMec_Viscosity_PowellEyring_table_parent
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_PowerLaw_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_PowerLaw_table_child,
    ads_Prop_MMec_Viscosity_PowerLaw_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_MMec_Viscosity_SutherlandsLawMembersEnm
{
    ads_Prop_MMec_Viscosity_SutherlandsLaw_referenceTemperature,
    ads_Prop_MMec_Viscosity_SutherlandsLaw_referenceViscosity,
    ads_Prop_MMec_Viscosity_SutherlandsLaw_sutherlandTemperature
};

/** Enum with association roles. */
enum ads_Prop_MMec_Viscosity_Tabular_tableRolesEnm
{
    ads_Prop_MMec_Viscosity_Tabular_table_child,
    ads_Prop_MMec_Viscosity_Tabular_table_parent
};

#endif
