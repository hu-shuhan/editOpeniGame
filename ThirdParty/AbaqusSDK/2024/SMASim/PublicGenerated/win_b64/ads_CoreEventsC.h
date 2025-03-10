//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreEventsC_h
#define ads_CoreEventsC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Events of the latest level of form Core */

/** Acoustic contribution factors represent portions of the acoustic response. */
#define ads_AcousticContribution (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 0))

/** A set of the acoustic nodes (response nodes); default value (if not specified) - all acoustic nodes. */
#define ads_AcousticContribution_acousticNodes (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 1))

/** The natural frequency range bounds. */
#define ads_AcousticContribution_frequencyRange (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 2))

/** A set of the structural nodes for the panel or grid contribution factors. Default value (if not specified) - all structural interface nodes. */
#define ads_AcousticContribution_structuralNodes (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 3))

/** Number of points in the frequency range at which results should be given. */
#define ads_FrequencyRangePoints (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 4))

/** Defining a spectrum using values of S as a function of frequency and damping. */
#define ads_Model_spectrum (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 5))

/** A collection of response spectrums. */
#define ads_Model_spectrumCollection (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 6))

/** A record representing a response spectrum. */
#define ads_Spectrum (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 7))

/** A collection of response spectrums */
#define ads_SpectrumCollection (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 8))

#define ads_SpectrumSasFunctionAccelerationTable (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 9))

#define ads_SpectrumSasFunctionDisplacementTable (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 10))

#define ads_SpectrumSasFunctionGTable (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 11))

#define ads_SpectrumSasFunctionVelocityTable (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 12))

/** Usage of a spectrum in response spectrum event */
#define ads_SpectrumUsage (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 13))

/** Response spectrum to be used. */
#define ads_SpectrumUsage_spectrum (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 14))

#define ads_Spectrum_SasFunction (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 15))

/** To use to define the spectrum in acceleration units. */
#define ads_Spectrum_SasFunction_Acceleration (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 16))

#define ads_Spectrum_SasFunction_Acceleration_table (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 17))

/** To use to define the spectrum in displacement units. */
#define ads_Spectrum_SasFunction_Displacement (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 18))

#define ads_Spectrum_SasFunction_Displacement_table (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 19))

/** To use to define the spectrum in acceleration of gravity units. */
#define ads_Spectrum_SasFunction_G (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 20))

#define ads_Spectrum_SasFunction_G_table (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 21))

/** To use to define the spectrum in velocity units. */
#define ads_Spectrum_SasFunction_Velocity (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 22))

#define ads_Spectrum_SasFunction_Velocity_table (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 23))

/** This option is used to compute the steady-state harmonic response directly in terms of the physical degrees of freedom of the model. */
#define ads_Step_Lin_DirectHarmonic (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 24))

/** The list of system dofs retained. May contain internal dofs in addition to user dofs. */
#define ads_Step_Lin_DirectHarmonic_retainedNodalDofs (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 25))

/** The list of the nodes retained by the user (connection points). */
#define ads_Step_Lin_DirectHarmonic_retainedNodes (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 26))

/** A modal harmonic event is used to calculate the steady-state dynamic response of a system to harmonic excitation as a linear perturbation procedure using modal superposition. */
#define ads_Step_Lin_ModalHarmonic (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 27))

/** Acoustic contribution factors of the event. */
#define ads_Step_Lin_ModalHarmonic_acousticContributions (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 28))

/** Link a ModalHarmonicEvent record to a SubspaceProjectionOptions record to compute the steady-state harmonic response on the basis of the subspace projection method. In this case a direct solution is obtained for the model projected onto the eigenvectors obtained in the preceding frequency task. */
#define ads_Step_Lin_ModalHarmonic_subspaceProjectionOptions (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 29))

/** This option is used to give the linearized response of a model to random excitation. */
#define ads_Step_Lin_ModalRandomResponse (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 30))

/** This event is used to provide linear transient history response as a linear perturbation procedure using modal superposition. */
#define ads_Step_Lin_ModalTransient (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 31))

/** This option is used to calculate estimates of peak values of displacements and stresses based on user-supplied response spectra and on the natural modes of the system. */
#define ads_Step_Lin_ResponseSpectrum (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 32))

/** Response spectrum usage parameters. */
#define ads_Step_Lin_ResponseSpectrum_useSpectrum (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 33))

/** A static linear perturbation event. */
#define ads_Step_Lin_Static (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 34))

/** The set of modes to be used for the event. Step_Lin should be Step_Lin_ModalHarmonic, Step_Lin_ModalRandomResponse, Step_Lin_ModalTransient, or Step_Lin_ResponseSpectrum. */
#define ads_Step_Lin_activeModes (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 35))

/** This reference is used to connect a step to an associated linear perturbation. A step can be associated with many events that can be superimposed. */
#define ads_Step_Lin_baseStateStep (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 36))

/** This reference provides the set of frequency frames which were created by running the harmonic event. */
#define ads_Step_Lin_frequencies (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 37))

/** For certain events, the user can specify the frequency ranges of interest and the number of frequencies at which results are required in each range. The association from the Event to the range is ordered because ABAQUS requires frequency ranges to be specified before single frequency points. */
#define ads_Step_Lin_frequencyRangePoints (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 38))

/** Frequency task of which results the event depends on. Only Step_Lin should be Step_Lin_ModalHarmonic, Step_Lin_ModalTransient, Step_Lin_ModalRandomResponse, Step_Lin_ResponseSpectrum, or Step_Lin_ComplexFrequency. */
#define ads_Step_Lin_frequencyStep (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 39))

/** Settings for the subspace-based version of the ModalHarmonicEvent. */
#define ads_SubspaceProjectionOptions (ads_CoreFragmentTypeIndex(ads_CoreEventsFragment, 40))

/** 
Enum with record members. */
enum ads_AcousticContributionMembersEnm
{
    ads_AcousticContribution_type
};

enum ads_AcousticContribution_typeEnm
{
    ads_AcousticContribution_type_GRID,
    ads_AcousticContribution_type_LOAD,
    ads_AcousticContribution_type_MODAL_ACOUSTIC,
    ads_AcousticContribution_type_MODAL_LOAD,
    ads_AcousticContribution_type_MODAL_STRUCTURAL,
    ads_AcousticContribution_type_PANEL
};

/** 
Enum with association roles. */
enum ads_AcousticContribution_acousticNodesRolesEnm
{
    ads_AcousticContribution_acousticNodes_referent,
    ads_AcousticContribution_acousticNodes_referrer
};

/** 
Enum with association roles. */
enum ads_AcousticContribution_frequencyRangeRolesEnm
{
    ads_AcousticContribution_frequencyRange_child,
    ads_AcousticContribution_frequencyRange_parent
};

/** 
Enum with association roles. */
enum ads_AcousticContribution_structuralNodesRolesEnm
{
    ads_AcousticContribution_structuralNodes_referent,
    ads_AcousticContribution_structuralNodes_referrer
};

/** 
Enum with record members. */
enum ads_FrequencyRangePointsMembersEnm
{
    ads_FrequencyRangePoints_bias,
    ads_FrequencyRangePoints_frequencyScale,
    ads_FrequencyRangePoints_frequencyScaleFactor,
    ads_FrequencyRangePoints_frequencySpread,
    ads_FrequencyRangePoints_lowerLimit,
    ads_FrequencyRangePoints_numberOfPoints,
    ads_FrequencyRangePoints_upperLimit
};

enum ads_FrequencyRangePoints_frequencyScaleEnm
{
    ads_FrequencyRangePoints_frequencyScale_LINEAR,
    ads_FrequencyRangePoints_frequencyScale_LOGARITHMIC
};

/** 
Enum with association roles. */
enum ads_Model_spectrumRolesEnm
{
    ads_Model_spectrum_child,
    ads_Model_spectrum_parent
};

/** 
Enum with association roles. */
enum ads_Model_spectrumCollectionRolesEnm
{
    ads_Model_spectrumCollection_child,
    ads_Model_spectrumCollection_parent
};

/** 
Enum with record members. */
enum ads_SpectrumUsageMembersEnm
{
    ads_SpectrumUsage_factor,
    ads_SpectrumUsage_frequencyF2,
    ads_SpectrumUsage_time,
    ads_SpectrumUsage_xCosine,
    ads_SpectrumUsage_yCosine,
    ads_SpectrumUsage_zCosine,
    ads_SpectrumUsage_zero
};

/** 
Enum with association roles. */
enum ads_SpectrumUsage_spectrumRolesEnm
{
    ads_SpectrumUsage_spectrum_referent,
    ads_SpectrumUsage_spectrum_referrer
};

/** Enum with association roles. */
enum ads_Spectrum_SasFunction_Acceleration_tableRolesEnm
{
    ads_Spectrum_SasFunction_Acceleration_table_child,
    ads_Spectrum_SasFunction_Acceleration_table_parent
};

/** Enum with association roles. */
enum ads_Spectrum_SasFunction_Displacement_tableRolesEnm
{
    ads_Spectrum_SasFunction_Displacement_table_child,
    ads_Spectrum_SasFunction_Displacement_table_parent
};

/** 
Enum with record members. */
enum ads_Spectrum_SasFunction_GMembersEnm
{
    ads_Spectrum_SasFunction_G_g
};

/** Enum with association roles. */
enum ads_Spectrum_SasFunction_G_tableRolesEnm
{
    ads_Spectrum_SasFunction_G_table_child,
    ads_Spectrum_SasFunction_G_table_parent
};

/** Enum with association roles. */
enum ads_Spectrum_SasFunction_Velocity_tableRolesEnm
{
    ads_Spectrum_SasFunction_Velocity_table_child,
    ads_Spectrum_SasFunction_Velocity_table_parent
};

/** 
Enum with record members. */
enum ads_Step_Lin_DirectHarmonicMembersEnm
{
    ads_Step_Lin_DirectHarmonic_designSensitivity,
    ads_Step_Lin_DirectHarmonic_dsa,
    ads_Step_Lin_DirectHarmonic_amplitude,
    ads_Step_Lin_DirectHarmonic_frictionDamping,
    ads_Step_Lin_DirectHarmonic_interval,
    ads_Step_Lin_DirectHarmonic_matrixSymmetry,
    ads_Step_Lin_DirectHarmonic_realOnly
};

enum ads_Step_Lin_DirectHarmonic_designSensitivityEnm
{
    ads_Step_Lin_DirectHarmonic_designSensitivity_ADJOINT,
    ads_Step_Lin_DirectHarmonic_designSensitivity_NONE
};

enum ads_Step_Lin_DirectHarmonic_amplitudeEnm
{
    ads_Step_Lin_DirectHarmonic_amplitude_RAMP,
    ads_Step_Lin_DirectHarmonic_amplitude_STEP
};

enum ads_Step_Lin_DirectHarmonic_intervalEnm
{
    ads_Step_Lin_DirectHarmonic_interval_EIGENFREQUENCY,
    ads_Step_Lin_DirectHarmonic_interval_RANGE,
    ads_Step_Lin_DirectHarmonic_interval_SPREAD
};

enum ads_Step_Lin_DirectHarmonic_matrixSymmetryEnm
{
    ads_Step_Lin_DirectHarmonic_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_DirectHarmonic_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_DirectHarmonic_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with association roles. */
enum ads_Step_Lin_DirectHarmonic_retainedNodalDofsRolesEnm
{
    ads_Step_Lin_DirectHarmonic_retainedNodalDofs_referent,
    ads_Step_Lin_DirectHarmonic_retainedNodalDofs_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_DirectHarmonic_retainedNodesRolesEnm
{
    ads_Step_Lin_DirectHarmonic_retainedNodes_referent,
    ads_Step_Lin_DirectHarmonic_retainedNodes_referrer
};

/** 
Enum with record members. */
enum ads_Step_Lin_ModalHarmonicMembersEnm
{
    ads_Step_Lin_ModalHarmonic_designSensitivity,
    ads_Step_Lin_ModalHarmonic_dsa,
    ads_Step_Lin_ModalHarmonic_amplitude,
    ads_Step_Lin_ModalHarmonic_frictionDamping,
    ads_Step_Lin_ModalHarmonic_interval,
    ads_Step_Lin_ModalHarmonic_matrixSymmetry,
    ads_Step_Lin_ModalHarmonic_realOnly
};

enum ads_Step_Lin_ModalHarmonic_designSensitivityEnm
{
    ads_Step_Lin_ModalHarmonic_designSensitivity_ADJOINT,
    ads_Step_Lin_ModalHarmonic_designSensitivity_NONE
};

enum ads_Step_Lin_ModalHarmonic_amplitudeEnm
{
    ads_Step_Lin_ModalHarmonic_amplitude_RAMP,
    ads_Step_Lin_ModalHarmonic_amplitude_STEP
};

enum ads_Step_Lin_ModalHarmonic_intervalEnm
{
    ads_Step_Lin_ModalHarmonic_interval_EIGENFREQUENCY,
    ads_Step_Lin_ModalHarmonic_interval_RANGE,
    ads_Step_Lin_ModalHarmonic_interval_SPREAD
};

enum ads_Step_Lin_ModalHarmonic_matrixSymmetryEnm
{
    ads_Step_Lin_ModalHarmonic_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_ModalHarmonic_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_ModalHarmonic_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with association roles. */
enum ads_Step_Lin_ModalHarmonic_acousticContributionsRolesEnm
{
    ads_Step_Lin_ModalHarmonic_acousticContributions_child,
    ads_Step_Lin_ModalHarmonic_acousticContributions_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_ModalHarmonic_subspaceProjectionOptionsRolesEnm
{
    ads_Step_Lin_ModalHarmonic_subspaceProjectionOptions_child,
    ads_Step_Lin_ModalHarmonic_subspaceProjectionOptions_parent
};

/** 
Enum with record members. */
enum ads_Step_Lin_ModalRandomResponseMembersEnm
{
    ads_Step_Lin_ModalRandomResponse_designSensitivity,
    ads_Step_Lin_ModalRandomResponse_dsa,
    ads_Step_Lin_ModalRandomResponse_amplitude,
    ads_Step_Lin_ModalRandomResponse_matrixSymmetry
};

enum ads_Step_Lin_ModalRandomResponse_designSensitivityEnm
{
    ads_Step_Lin_ModalRandomResponse_designSensitivity_ADJOINT,
    ads_Step_Lin_ModalRandomResponse_designSensitivity_NONE
};

enum ads_Step_Lin_ModalRandomResponse_amplitudeEnm
{
    ads_Step_Lin_ModalRandomResponse_amplitude_RAMP,
    ads_Step_Lin_ModalRandomResponse_amplitude_STEP
};

enum ads_Step_Lin_ModalRandomResponse_matrixSymmetryEnm
{
    ads_Step_Lin_ModalRandomResponse_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_ModalRandomResponse_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_ModalRandomResponse_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Lin_ModalTransientMembersEnm
{
    ads_Step_Lin_ModalTransient_designSensitivity,
    ads_Step_Lin_ModalTransient_dsa,
    ads_Step_Lin_ModalTransient_amplitude,
    ads_Step_Lin_ModalTransient_continue,
    ads_Step_Lin_ModalTransient_initialTimeIncrement,
    ads_Step_Lin_ModalTransient_matrixSymmetry,
    ads_Step_Lin_ModalTransient_totalTime
};

enum ads_Step_Lin_ModalTransient_designSensitivityEnm
{
    ads_Step_Lin_ModalTransient_designSensitivity_ADJOINT,
    ads_Step_Lin_ModalTransient_designSensitivity_NONE
};

enum ads_Step_Lin_ModalTransient_amplitudeEnm
{
    ads_Step_Lin_ModalTransient_amplitude_RAMP,
    ads_Step_Lin_ModalTransient_amplitude_STEP
};

enum ads_Step_Lin_ModalTransient_matrixSymmetryEnm
{
    ads_Step_Lin_ModalTransient_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_ModalTransient_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_ModalTransient_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with record members. */
enum ads_Step_Lin_ResponseSpectrumMembersEnm
{
    ads_Step_Lin_ResponseSpectrum_designSensitivity,
    ads_Step_Lin_ResponseSpectrum_dsa,
    ads_Step_Lin_ResponseSpectrum_amplitude,
    ads_Step_Lin_ResponseSpectrum_comp,
    ads_Step_Lin_ResponseSpectrum_matrixSymmetry,
    ads_Step_Lin_ResponseSpectrum_missingMassMethod,
    ads_Step_Lin_ResponseSpectrum_rigidResponse,
    ads_Step_Lin_ResponseSpectrum_sum
};

enum ads_Step_Lin_ResponseSpectrum_designSensitivityEnm
{
    ads_Step_Lin_ResponseSpectrum_designSensitivity_ADJOINT,
    ads_Step_Lin_ResponseSpectrum_designSensitivity_NONE
};

enum ads_Step_Lin_ResponseSpectrum_amplitudeEnm
{
    ads_Step_Lin_ResponseSpectrum_amplitude_RAMP,
    ads_Step_Lin_ResponseSpectrum_amplitude_STEP
};

enum ads_Step_Lin_ResponseSpectrum_compEnm
{
    ads_Step_Lin_ResponseSpectrum_comp_ALGEBRAIC,
    ads_Step_Lin_ResponseSpectrum_comp_R30,
    ads_Step_Lin_ResponseSpectrum_comp_R40,
    ads_Step_Lin_ResponseSpectrum_comp_SRSS
};

enum ads_Step_Lin_ResponseSpectrum_matrixSymmetryEnm
{
    ads_Step_Lin_ResponseSpectrum_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_ResponseSpectrum_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_ResponseSpectrum_matrixSymmetry_UNSYMMETRIC
};

enum ads_Step_Lin_ResponseSpectrum_rigidResponseEnm
{
    ads_Step_Lin_ResponseSpectrum_rigidResponse_GUPTA,
    ads_Step_Lin_ResponseSpectrum_rigidResponse_LINDLEY_YOW,
    ads_Step_Lin_ResponseSpectrum_rigidResponse_NONE
};

enum ads_Step_Lin_ResponseSpectrum_sumEnm
{
    ads_Step_Lin_ResponseSpectrum_sum_ABS,
    ads_Step_Lin_ResponseSpectrum_sum_CQC,
    ads_Step_Lin_ResponseSpectrum_sum_DSC,
    ads_Step_Lin_ResponseSpectrum_sum_GRP,
    ads_Step_Lin_ResponseSpectrum_sum_NRL,
    ads_Step_Lin_ResponseSpectrum_sum_SRSS,
    ads_Step_Lin_ResponseSpectrum_sum_TENP
};

/** 
Enum with association roles. */
enum ads_Step_Lin_ResponseSpectrum_useSpectrumRolesEnm
{
    ads_Step_Lin_ResponseSpectrum_useSpectrum_child,
    ads_Step_Lin_ResponseSpectrum_useSpectrum_parent
};

/** 
Enum with record members. */
enum ads_Step_Lin_StaticMembersEnm
{
    ads_Step_Lin_Static_designSensitivity,
    ads_Step_Lin_Static_dsa,
    ads_Step_Lin_Static_amplitude,
    ads_Step_Lin_Static_matrixSymmetry,
    ads_Step_Lin_Static_residualModes
};

enum ads_Step_Lin_Static_designSensitivityEnm
{
    ads_Step_Lin_Static_designSensitivity_ADJOINT,
    ads_Step_Lin_Static_designSensitivity_NONE
};

enum ads_Step_Lin_Static_amplitudeEnm
{
    ads_Step_Lin_Static_amplitude_RAMP,
    ads_Step_Lin_Static_amplitude_STEP
};

enum ads_Step_Lin_Static_matrixSymmetryEnm
{
    ads_Step_Lin_Static_matrixSymmetry_AUTOMATIC,
    ads_Step_Lin_Static_matrixSymmetry_SYMMETRIC,
    ads_Step_Lin_Static_matrixSymmetry_UNSYMMETRIC
};

/** 
Enum with association roles. */
enum ads_Step_Lin_activeModesRolesEnm
{
    ads_Step_Lin_activeModes_referent,
    ads_Step_Lin_activeModes_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_baseStateStepRolesEnm
{
    ads_Step_Lin_baseStateStep_referent,
    ads_Step_Lin_baseStateStep_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_frequenciesRolesEnm
{
    ads_Step_Lin_frequencies_referent,
    ads_Step_Lin_frequencies_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Lin_frequencyRangePointsRolesEnm
{
    ads_Step_Lin_frequencyRangePoints_child,
    ads_Step_Lin_frequencyRangePoints_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_frequencyStepRolesEnm
{
    ads_Step_Lin_frequencyStep_referent,
    ads_Step_Lin_frequencyStep_referrer
};

/** 
Enum with record members. */
enum ads_SubspaceProjectionOptionsMembersEnm
{
    ads_SubspaceProjectionOptions_dampingChange,
    ads_SubspaceProjectionOptions_projection,
    ads_SubspaceProjectionOptions_stiffnessChange
};

enum ads_SubspaceProjectionOptions_projectionEnm
{
    ads_SubspaceProjectionOptions_projection_ALL_FREQUENCIES,
    ads_SubspaceProjectionOptions_projection_CONSTANT,
    ads_SubspaceProjectionOptions_projection_EIGENFREQUENCY,
    ads_SubspaceProjectionOptions_projection_PROPERTY_CHANGE,
    ads_SubspaceProjectionOptions_projection_RANGE
};

#endif
