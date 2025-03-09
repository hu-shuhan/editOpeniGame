//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreFatigueC_h
#define ads_CoreFatigueC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Fatigue of the latest level of form Core */

/** Base type for FatigueLoadingEvent (FLE) Contributions */
#define ads_FLEContribution (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 0))

/** Contribution from an individual data set. May have signals attached. */
#define ads_FLEContribution_DataSet (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 1))

/** Contribution from a sequence of datasets. */
#define ads_FLEContribution_DataSetSeq (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 2))

/** Embedded signal data. Similar to Curve_Tabular/EquallySpaced, but very often in fatigue a time scale is not used, so the signal is just a sequence of values. A time signal common to load signals can be applied at the FatigueLoadingEvent level. Mutually exclusive with a SignalReference. */
#define ads_FLEContribution_DataSet_embeddedSignal (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 3))

/** Attaches a SignalReference to a DataSet contribution. Mutually exclusive with an embedded signal. */
#define ads_FLEContribution_DataSet_signalReference (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 4))

/** Contribution recipe for a ModalTransDyn event. */
#define ads_FLEContribution_ModalTransDyn (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 5))

#define ads_FLEContribution_ModalTransDyn_frequencyFilters (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 6))

/** Step source for generalized displacement histories */
#define ads_FLEContribution_ModalTransDyn_generalizedDisplacementsSource (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 7))

/** Contribution recipe for a RandomVibration event. */
#define ads_FLEContribution_RandomVibration (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 8))

#define ads_FLEContribution_RandomVibration_channelPairs (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 9))

#define ads_FLEContribution_RandomVibration_channels (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 10))

#define ads_FLEContribution_RandomVibration_frequencyFilters (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 11))

/** Step source for (complex) generalized displacements as functions of frequency and load case */
#define ads_FLEContribution_RandomVibration_generalizedDisplacementsSource (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 12))

/** Step source for stress, strain, and other fatigue driving fields */
#define ads_FLEContribution_fieldSource (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 13))

/** Defines attributes of an algorithm override. */
#define ads_FatigueAlgorithmOverride (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 14))

/** The fatigue algorithm overriding the region. Reusing the material option within a Scenario/Step-level feature. */
#define ads_FatigueAlgorithmOverride_fatigueAlgorithm (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 15))

/** Reference to one or more regions where the material's fatigue algorithm will be overridden. No region references, indicates that the override applies to the whole model. */
#define ads_FatigueAlgorithmOverride_region (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 16))

/** Driving variable for fatigue analysis, e.g. S, E, EE, EP, NFORC, S_USER */
#define ads_FatigueDrivingVariable (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 17))

/** Aggregates fatigue loading events. Note that fatigue loading is not defined in terms of actual loads. It is defined in terms of sequences of stress, strain, and other fields induced by actual loads plus signals that multiply the stresses (very similarly to how Abaqus loads can be varied over time via amplitude curves). */
#define ads_FatigueLoading (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 18))

/** Base type for fatigue loading events. */
#define ads_FatigueLoadingEvent (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 19))

/** To be used as the "frame" dimension in damage per event, for example. */
#define ads_FatigueLoadingEventCollection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 20))

/** Bundles a sequence of primary fatigue loading events. */
#define ads_FatigueLoadingEvent_Bundle (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 21))

/** Capture sequence of primary FatigueLoadingEvents as children of FatigueLoadingEvent_Bundle */
#define ads_FatigueLoadingEvent_Bundle_events (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 22))

/** High-frequency event, superimposed on a primary event. */
#define ads_FatigueLoadingEvent_HighFreq (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 23))

/** Capture the sequence of high-frequency fatigue loading event contributions. Restriction: Only DataSet or DataSetSeq contributions are allowed. */
#define ads_FatigueLoadingEvent_HighFreq_fleContribution (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 24))

/** Primary fatigue loading event. Aggregates fatigue loading contributions and high-frequency events. */
#define ads_FatigueLoadingEvent_Primary (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 25))

/** Capture the set of FEA output variables that drive the fatigue analysis */
#define ads_FatigueLoadingEvent_Primary_drivingVariables (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 26))

/** Embedded signal data for Spectrum event. It attaches to the Spectrum event as a whole, not individual contributions. */
#define ads_FatigueLoadingEvent_Primary_embeddedSignal (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 27))

/** Capture the sequence of primary fatigue loading event contributions. Restrictions: PSD or ModalDyn contribution apply only to corresponding event types, and in such cases only a single contribution is allowed. */
#define ads_FatigueLoadingEvent_Primary_fleContribution (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 28))

/** Capture sequence of high-frequency FatigueLoadingEvents as children of FatigueLoadingEvent_Primary */
#define ads_FatigueLoadingEvent_Primary_highFreqEvents (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 29))

/** Should ONLY be used with primary event type=SPECTRUM */
#define ads_FatigueLoadingEvent_Primary_spectrumTable (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 30))

/** Capture specification of the source of a temperature field for this event, if necessary. temperature policy = MATCHING might cause values in this field to be overridden. */
#define ads_FatigueLoadingEvent_Primary_temperatureField (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 31))

/** Dummy event used to allow OutputRequests to refer to it for per-event variables. It's existence also acts as a flag turn on computing inter-event transition damage, and any other data that goes with this event. */
#define ads_FatigueLoadingEvent_Transitions (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 32))

/** Embedded time signal data for a Primary or HF event that has DataSet or DataSetSeq contributions. Mutually exclusive with timeSignalReference. */
#define ads_FatigueLoadingEvent_embeddedTimeSignal (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 33))

/** Attaches a time SignalReference to a Primary or HF event that has DataSet or DataSetSeq contributions. Mutually exclusive with an embedded time signal. */
#define ads_FatigueLoadingEvent_timeSignalReference (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 34))

/** Event collection for a given fatigue step. */
#define ads_FatigueLoading_eventCollection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 35))

/** Capture sequence of FatigueLoadingEvents as children of FatigueLoading. Restricted to a mix of Bundles or Primary events (i.e. no high-frequency events) */
#define ads_FatigueLoading_events (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 36))

/** Capture specification of the source of residual stress and strain fields */
#define ads_FatigueLoading_residualStressStrain (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 37))

/** Defines attributes of a material override. */
#define ads_FatigueMaterialOverride (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 38))

/** Reference to the material used to override the model assigned material. */
#define ads_FatigueMaterialOverride_material (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 39))

/** Reference to one or more regions where the material will be overridden. No region references, indicates that the override applies to the whole model. */
#define ads_FatigueMaterialOverride_region (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 40))

/** Used to filter modes based on their frequency (the actual modes will not always be available). */
#define ads_FrequencyFilter (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 41))

/** Data type for definining attributes of point fastener. Refer to a region of fastener elements generated by point fastener feature. The type of elements depends on the construct of the point fastener. */
#define ads_FsfPointFastener (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 42))

/** Reference to the material override for the point fastener. If absent, uses the parent spot weld set material assignment. */
#define ads_FsfPointFastener_material (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 43))

/** Reference to a region of elements which represent the spot welds for a point fastener. */
#define ads_FsfPointFastener_weldConnections (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 44))

#define ads_LoadChannel (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 45))

#define ads_LoadChannelPairPSD (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 46))

/** Single occurrence used for auto-correlation (PSD); double occurrence used for cross-correlation (CSD) */
#define ads_LoadChannelPairPSD_channels (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 47))

#define ads_LoadChannelPairPSD_psd (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 48))

/** String key is the PLM ID of the user MSC PLM file. Each table may be referenced by multiple WeldsDefinition_Standard. */
#define ads_Model_userMeanStressCorrectionFTables (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 49))

/** Base type for defining non-weld parameters on a per region basis. */
#define ads_NonWeld_Override (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 50))

/** Subtype for defining parameters for 7th Edition FKM Non-weld assessments on a per region basis. */
#define ads_NonWeld_Override_FKM7 (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 51))

/** Subtype for defining parameters for FKM Non-weld fatigue assessments on a per region basis. */
#define ads_NonWeld_Override_FKM7_Fatigue (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 52))

/** Subtype for defining parameters for FKM Non-weld static assessments on a per region basis. */
#define ads_NonWeld_Override_FKM7_Static (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 53))

/** Reference to one or more regions where the parameters will be overridden. No region references, indicates that the override applies to the whole model. */
#define ads_NonWeld_Override_region (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 54))

/** Links OutputRequest to a FatigueLoadingEvent for per-event field/contour output, e.g. damage per event. The specific subtypes of event that can be linked to are the Primary and Transitions events. This data model implies that a separate field and corresponding OutputRequest must be created for each primary event and transitions event (if present) in the loading. */
#define ads_OutputRequest_perFatigueEventOutput (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 55))

/** Capture a reference to a signal file that is attached as an auxiliary file to the SIM Manifest. Also capture channel name, if necessary. */
#define ads_SignalReference (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 56))

/** Data to define a SpectrumTable. All content is dimensionless. */
#define ads_SpectrumTable (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 57))

/** Base type for definining attributes of a spot weld set. Can refer either to a region of fastener elements, or have FsfPointFastener compositions each of which refer to a region of fastener elements. */
#define ads_SpotWeldSet (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 58))

/** Subtype for definining attributes of standard spot weld set. Presently (Sept-2021) the only spot weld method, but using subtyping as we may add more in the future. */
#define ads_SpotWeldSet_Standard (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 59))

/** Captures the point fastener supports of a spot weld set and their attributes for fatigue analysis. */
#define ads_SpotWeldSet_fsfPointFasteners (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 60))

/** Reference to the material override for the spot weld set. If absent, the point fastener material assignment is used. */
#define ads_SpotWeldSet_material (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 61))

/** Reference to a region of elements which represent the spot welds for a spot weld set. This region is only used when using the manual modelling technique, not when the spot welds are created using point fasteners. */
#define ads_SpotWeldSet_weldConnections (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 62))

/** Capture information needed to find the step within a SIMDoc from which stress, strain, and other fields will be read for driving a fatigue analysis. Note that fatigue can be driven by stresses/strains from multiple FEA SIMDocs. */
#define ads_StepRefInfo (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 63))

/** Links fatigue StepRefInfo to an ExternalSource SIMDoc */
#define ads_StepRefInfo_simDoc (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 64))

/** Fatigue step, needed primarily to capture fatigue results. Fatigue calculations can depend on several FEA jobs and several steps, so fatigue results need to be kept separate from such jobs and steps. It is also used as the anchor for fatigue loading specifications. */
#define ads_Step_Gen_Fatigue (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 65))

/** Fatigue step subtype specifically for FKM guideline's fatigue from local stresses. */
#define ads_Step_Gen_Fatigue_FKM (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 66))

/** Fatigue algorithm overrides associated with the fatigue step. A sequence composition such that the 'last one wins' when there are overlapping regions. */
#define ads_Step_Gen_Fatigue_fatigueAlgorithmOverride (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 67))

/** Single fatigue loading child of Fatigue Step. */
#define ads_Step_Gen_Fatigue_fatigueLoading (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 68))

/** Fatigue material overrides associated with the fatigue step. A sequence composition such that the 'last one wins' when there are overlapping regions. */
#define ads_Step_Gen_Fatigue_fatigueMaterialOverride (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 69))

/** Captures the locations of spot welds and their attributes for fatigue analysis. */
#define ads_Step_Gen_Fatigue_spotWeldSets (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 70))

/** Analysis step for FKM guideline's static strength assessment from local stresses. */
#define ads_Step_Gen_StaticFKM (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 71))

/** Capture a single or linear combination stress fields driving an FKM static strength assessment. */
#define ads_Step_Gen_StaticFKM_fkmStaticLoadFrames (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 72))

/** Links FKM static step to an FKM fatigue step */
#define ads_Step_Gen_StaticFKM_fkmWorstOfFatigueLoading (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 73))

/** Non-weld overrides associated with the step. A sequence composition such that the 'last one wins' when there are overlapping regions. */
#define ads_Step_Gen_nonWeldOverride (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 74))

/** Captures the locations of welds and their attributes for fatigue or static strength assessments. */
#define ads_Step_Gen_weldsDefinition (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 75))

/** Data type to capture the surface finish. Direct specification of Kf. */
#define ads_SurfaceFinish_KfV (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 76))

/** Data type to capture the surface finish. Direct specification of Kt. */
#define ads_SurfaceFinish_KtV (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 77))

/** Data type to capture the surface finish. Kt or Kf from specified Roughness Name. */
#define ads_SurfaceFinish_RN (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 78))

/** Data type to capture the surface finish. Kt or Kf from specified Roughness Value. */
#define ads_SurfaceFinish_RV (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 79))

/** Reference to the surface finish property table which provides Kt or Kf lookup values. Null link to SurfaceFinishProp indicates the table is not required, Kt or Kf is directly specified or R value is used directly (such as in FKM). */
#define ads_SurfaceFinish_surfaceFinishProp (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 80))

/** Datatype for definining attributes of surface stress extrapolation. */
#define ads_SurfaceStressExtrapolation (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 81))

/** Data to define a piecewise linear mean stress correction curve. */
#define ads_UserMeanStressCorrectionFTable (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 82))

/** Datatype for definining attributes of weld end correction. */
#define ads_WeldEndCorrection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 83))

/** Reference to selected weld ends region for weld end correction. Should only contain points. If absent, weld end correction is applied to all ends. */
#define ads_WeldEndCorrection_endCorrectionPoints (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 84))

/** Base type for definining attributes of weld failure mode. */
#define ads_WeldFailureMode (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 85))

/** Sub type for definining attributes of weld failure mode including direction sense of failure. */
#define ads_WeldFailureMode_Sensed (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 86))

/** Reference to a solid region, elements only. Used to directly specify the crack surface used for nodal force summation. */
#define ads_WeldFailureMode_crackDomainSelection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 87))

/** Reference to a surface or solid region, element faces only. Used to directly specify the crack surface used for nodal force summation. */
#define ads_WeldFailureMode_crackSurfaceSelection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 88))

/** Reference to the material override for the current weld failure mode. If absent, uses the section material assignment. */
#define ads_WeldFailureMode_material (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 89))

/** Datatype for definining attributes of weld sub-region of a weld fillet set to allow for different materials to be applied in each sub-region. Sub-regions could be for different section assignments or different line fasteners, for example. */
#define ads_WeldSubRegion (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 90))

/** Reference to the material override for the fusion weld failure mode. Applies to welds derived from the parent WeldSubRegion. */
#define ads_WeldSubRegion_fusionMaterial (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 91))

/** Reference to the material override for the root weld failure mode. Applies to welds derived from the parent WeldSubRegion. */
#define ads_WeldSubRegion_rootMaterial (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 92))

/** Reference to the material override for the throat weld failure mode. Applies to welds derived from the parent WeldSubRegion. */
#define ads_WeldSubRegion_throatMaterial (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 93))

/** Reference to the material override for the toe weld failure mode. Applies to welds derived from the parent WeldSubRegion. */
#define ads_WeldSubRegion_toeMaterial (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 94))

/** Reference to a region of elements which make up the body of the WeldSubRegion. For line fasteners, region contains only the fillet elements. */
#define ads_WeldSubRegion_weldSubBody (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 95))

/** Base type for definining attributes of line welds. Geometry referred to would generally involve many weld fillets or lines. */
#define ads_WeldsDefinition (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 96))

/** Subtype for definining attributes of FKM weld assessments. Applies to 6th Edition only. */
#define ads_WeldsDefinition_FKM (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 97))

/** Subtype for definining attributes of FKM weld assessments. */
#define ads_WeldsDefinition_FKM7 (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 98))

/** Subtype for capturing FKM weld FATIGUE assessment factors. Applies to 7th Edition only. */
#define ads_WeldsDefinition_FKM7_Fatigue (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 99))

/** Subtype for capturing FKM weld STATIC assessment factors. Applies to 7th Edition only. */
#define ads_WeldsDefinition_FKM7_Static (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 100))

/** Subtype for capturing FKM weld FATIGUE assessment factors. Applies to 6th Edition only. */
#define ads_WeldsDefinition_FKM_Fatigue (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 101))

/** Subtype for capturing FKM weld STATIC assessment factors. Applies to 6th Edition only. */
#define ads_WeldsDefinition_FKM_Static (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 102))

/** Subtype for definining attributes of standard weld fillet or line set. */
#define ads_WeldsDefinition_Standard (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 103))

/** Captures the weld fusion failure mode activation and parameters. Weld line sets must have only a single failure mode. */
#define ads_WeldsDefinition_Standard_fusionFailureMode (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 104))

/** Captures the weld root failure mode activation and parameters. Weld line sets must have only a single failure mode. */
#define ads_WeldsDefinition_Standard_rootFailureMode (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 105))

/** Captures the weld throat failure mode activation and parameters. Weld line sets must have only a single failure mode. */
#define ads_WeldsDefinition_Standard_throatFailureMode (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 106))

/** Captures the weld toe failure mode activation and parameters. Weld line sets must have only a single failure mode. */
#define ads_WeldsDefinition_Standard_toeFailureMode (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 107))

/** Reference to the mean stress correction curve data when meanStressCorrectionType is set to USER_FILE. */
#define ads_WeldsDefinition_Standard_userMeanStressCorrectionFTable (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 108))

/** Captures the weld end correction parameters. If absent, weld end correction is not performed. */
#define ads_WeldsDefinition_Standard_weldEndCorrection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 109))

/** Reference to weld line geometry, element edges only. Used (a) to restrict the weld lines analysed to a subset of the possible weld lines on the weldBody (b) as the primary definition of the weld lines. */
#define ads_WeldsDefinition_lineSelection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 110))

/** Reference to a surface, element faces only. Used (a) to disambiguate the surface used for surface stress extrapolation (b) define surface normal used along a weld line to define through-thickness crack direction (b) as the through-thickness crack surface */
#define ads_WeldsDefinition_surfaceSelection (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 111))

/** Captures the surface stress extrapolation parameters. */
#define ads_WeldsDefinition_surfaceStressExtrapolation (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 112))

/** Reference to body/fillet region for welds definitions. Should only contain solid elements or shell elements--no edges or faces. May contain additional elements not considered part of the fillet. Actual fillet elements are obtained by subtracting the weldBodyExclusion elements */
#define ads_WeldsDefinition_weldBody (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 113))

/** Reference to excluded body/fillet region for welds definitions. The elements in this region will be subtracted from those in weldBody. */
#define ads_WeldsDefinition_weldBodyExclusion (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 114))

/** Captures the sub-regions of a weld fillet set and their attributes for fatigue analysis. */
#define ads_WeldsDefinition_weldSubRegions (ads_CoreFragmentTypeIndex(ads_CoreFatigueFragment, 115))

/** 
Enum with record members. */
enum ads_FLEContribution_DataSetMembersEnm
{
    ads_FLEContribution_DataSet_increment,
    ads_FLEContribution_DataSet_loadCase,
    ads_FLEContribution_DataSet_scale,
    ads_FLEContribution_DataSet_signalOffset
};

enum ads_FLEContribution_DataSet_incrementEnm
{
    ads_FLEContribution_DataSet_increment_LAST,
    ads_FLEContribution_DataSet_increment_NONE
};

/** 
Enum with record members. */
enum ads_FLEContribution_DataSetSeqMembersEnm
{
    ads_FLEContribution_DataSetSeq_increments,
    ads_FLEContribution_DataSetSeq_scale
};

enum ads_FLEContribution_DataSetSeq_incrementsEnm
{
    ads_FLEContribution_DataSetSeq_increments_ALL,
    ads_FLEContribution_DataSetSeq_increments_ALL_EXCEPT_FIRST
};

/** 
Enum with association roles. */
enum ads_FLEContribution_DataSet_embeddedSignalRolesEnm
{
    ads_FLEContribution_DataSet_embeddedSignal_child,
    ads_FLEContribution_DataSet_embeddedSignal_parent
};

/** 
Enum with association roles. */
enum ads_FLEContribution_DataSet_signalReferenceRolesEnm
{
    ads_FLEContribution_DataSet_signalReference_child,
    ads_FLEContribution_DataSet_signalReference_parent
};

/** Enum with association roles. */
enum ads_FLEContribution_ModalTransDyn_frequencyFiltersRolesEnm
{
    ads_FLEContribution_ModalTransDyn_frequencyFilters_child,
    ads_FLEContribution_ModalTransDyn_frequencyFilters_parent
};

/** 
Enum with association roles. */
enum ads_FLEContribution_ModalTransDyn_generalizedDisplacementsSourceRolesEnm
{
    ads_FLEContribution_ModalTransDyn_generalizedDisplacementsSource_child,
    ads_FLEContribution_ModalTransDyn_generalizedDisplacementsSource_parent
};

/** 
Enum with record members. */
enum ads_FLEContribution_RandomVibrationMembersEnm
{
    ads_FLEContribution_RandomVibration_kurtosis,
    ads_FLEContribution_RandomVibration_probabilityDensityFunction
};

enum ads_FLEContribution_RandomVibration_probabilityDensityFunctionEnm
{
    ads_FLEContribution_RandomVibration_probabilityDensityFunction_BENDAT_NB,
    ads_FLEContribution_RandomVibration_probabilityDensityFunction_DIRLIK,
    ads_FLEContribution_RandomVibration_probabilityDensityFunction_STIENBURG_NB,
    ads_FLEContribution_RandomVibration_probabilityDensityFunction_TOVO_BENASCIUTTI_FIXED_MEAN,
    ads_FLEContribution_RandomVibration_probabilityDensityFunction_TOVO_BENASCIUTTI_RANDOM_MEAN,
    ads_FLEContribution_RandomVibration_probabilityDensityFunction_WIRSCHIN_LIGHT_NB
};

/** Enum with association roles. */
enum ads_FLEContribution_RandomVibration_channelPairsRolesEnm
{
    ads_FLEContribution_RandomVibration_channelPairs_child,
    ads_FLEContribution_RandomVibration_channelPairs_parent
};

/** Enum with association roles. */
enum ads_FLEContribution_RandomVibration_channelsRolesEnm
{
    ads_FLEContribution_RandomVibration_channels_child,
    ads_FLEContribution_RandomVibration_channels_parent
};

/** Enum with association roles. */
enum ads_FLEContribution_RandomVibration_frequencyFiltersRolesEnm
{
    ads_FLEContribution_RandomVibration_frequencyFilters_child,
    ads_FLEContribution_RandomVibration_frequencyFilters_parent
};

/** 
Enum with association roles. */
enum ads_FLEContribution_RandomVibration_generalizedDisplacementsSourceRolesEnm
{
    ads_FLEContribution_RandomVibration_generalizedDisplacementsSource_child,
    ads_FLEContribution_RandomVibration_generalizedDisplacementsSource_parent
};

/** 
Enum with association roles. */
enum ads_FLEContribution_fieldSourceRolesEnm
{
    ads_FLEContribution_fieldSource_child,
    ads_FLEContribution_fieldSource_parent
};

/** 
Enum with record members. */
enum ads_FatigueAlgorithmOverrideMembersEnm
{
    ads_FatigueAlgorithmOverride_frfFatigueLimit
};

/** 
Enum with association roles. */
enum ads_FatigueAlgorithmOverride_fatigueAlgorithmRolesEnm
{
    ads_FatigueAlgorithmOverride_fatigueAlgorithm_child,
    ads_FatigueAlgorithmOverride_fatigueAlgorithm_parent
};

/** 
Enum with association roles. */
enum ads_FatigueAlgorithmOverride_regionRolesEnm
{
    ads_FatigueAlgorithmOverride_region_referent,
    ads_FatigueAlgorithmOverride_region_referrer
};

/** 
Enum with record members. */
enum ads_FatigueDrivingVariableMembersEnm
{
    ads_FatigueDrivingVariable_variable
};

/** 
Enum with record members. */
enum ads_FatigueLoadingEventMembersEnm
{
    ads_FatigueLoadingEvent_label
};

/** 
Enum with record members. */
enum ads_FatigueLoadingEvent_BundleMembersEnm
{
    ads_FatigueLoadingEvent_Bundle_label,
    ads_FatigueLoadingEvent_Bundle_repeats
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Bundle_eventsRolesEnm
{
    ads_FatigueLoadingEvent_Bundle_events_child,
    ads_FatigueLoadingEvent_Bundle_events_parent
};

/** 
Enum with record members. */
enum ads_FatigueLoadingEvent_HighFreqMembersEnm
{
    ads_FatigueLoadingEvent_HighFreq_label,
    ads_FatigueLoadingEvent_HighFreq_duration
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_HighFreq_fleContributionRolesEnm
{
    ads_FatigueLoadingEvent_HighFreq_fleContribution_child,
    ads_FatigueLoadingEvent_HighFreq_fleContribution_parent
};

/** 
Enum with record members. */
enum ads_FatigueLoadingEvent_PrimaryMembersEnm
{
    ads_FatigueLoadingEvent_Primary_label,
    ads_FatigueLoadingEvent_Primary_duration,
    ads_FatigueLoadingEvent_Primary_matchTemperatures,
    ads_FatigueLoadingEvent_Primary_repeats,
    ads_FatigueLoadingEvent_Primary_scale,
    ads_FatigueLoadingEvent_Primary_temperature,
    ads_FatigueLoadingEvent_Primary_temperaturePolicy,
    ads_FatigueLoadingEvent_Primary_type
};

enum ads_FatigueLoadingEvent_Primary_temperaturePolicyEnm
{
    ads_FatigueLoadingEvent_Primary_temperaturePolicy_AVERAGE,
    ads_FatigueLoadingEvent_Primary_temperaturePolicy_MAXIMUM,
    ads_FatigueLoadingEvent_Primary_temperaturePolicy_MINIMUM
};

enum ads_FatigueLoadingEvent_Primary_typeEnm
{
    ads_FatigueLoadingEvent_Primary_type_MODAL_TRANSIENT,
    ads_FatigueLoadingEvent_Primary_type_PSD_DIRECT,
    ads_FatigueLoadingEvent_Primary_type_PSD_MODAL,
    ads_FatigueLoadingEvent_Primary_type_SCALE_AND_COMBINE,
    ads_FatigueLoadingEvent_Primary_type_SEQUENCE,
    ads_FatigueLoadingEvent_Primary_type_SPECTRUM
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Primary_drivingVariablesRolesEnm
{
    ads_FatigueLoadingEvent_Primary_drivingVariables_child,
    ads_FatigueLoadingEvent_Primary_drivingVariables_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Primary_embeddedSignalRolesEnm
{
    ads_FatigueLoadingEvent_Primary_embeddedSignal_child,
    ads_FatigueLoadingEvent_Primary_embeddedSignal_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Primary_fleContributionRolesEnm
{
    ads_FatigueLoadingEvent_Primary_fleContribution_child,
    ads_FatigueLoadingEvent_Primary_fleContribution_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Primary_highFreqEventsRolesEnm
{
    ads_FatigueLoadingEvent_Primary_highFreqEvents_child,
    ads_FatigueLoadingEvent_Primary_highFreqEvents_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Primary_spectrumTableRolesEnm
{
    ads_FatigueLoadingEvent_Primary_spectrumTable_child,
    ads_FatigueLoadingEvent_Primary_spectrumTable_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_Primary_temperatureFieldRolesEnm
{
    ads_FatigueLoadingEvent_Primary_temperatureField_child,
    ads_FatigueLoadingEvent_Primary_temperatureField_parent
};

/** 
Enum with record members. */
enum ads_FatigueLoadingEvent_TransitionsMembersEnm
{
    ads_FatigueLoadingEvent_Transitions_label
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_embeddedTimeSignalRolesEnm
{
    ads_FatigueLoadingEvent_embeddedTimeSignal_child,
    ads_FatigueLoadingEvent_embeddedTimeSignal_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoadingEvent_timeSignalReferenceRolesEnm
{
    ads_FatigueLoadingEvent_timeSignalReference_child,
    ads_FatigueLoadingEvent_timeSignalReference_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoading_eventCollectionRolesEnm
{
    ads_FatigueLoading_eventCollection_child,
    ads_FatigueLoading_eventCollection_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoading_eventsRolesEnm
{
    ads_FatigueLoading_events_child,
    ads_FatigueLoading_events_parent
};

/** 
Enum with association roles. */
enum ads_FatigueLoading_residualStressStrainRolesEnm
{
    ads_FatigueLoading_residualStressStrain_child,
    ads_FatigueLoading_residualStressStrain_parent
};

/** 
Enum with association roles. */
enum ads_FatigueMaterialOverride_materialRolesEnm
{
    ads_FatigueMaterialOverride_material_referent,
    ads_FatigueMaterialOverride_material_referrer
};

/** 
Enum with association roles. */
enum ads_FatigueMaterialOverride_regionRolesEnm
{
    ads_FatigueMaterialOverride_region_referent,
    ads_FatigueMaterialOverride_region_referrer
};

/** 
Enum with record members. */
enum ads_FsfPointFastenerMembersEnm
{
    ads_FsfPointFastener_fastenerDiameter
};

/** 
Enum with association roles. */
enum ads_FsfPointFastener_materialRolesEnm
{
    ads_FsfPointFastener_material_referent,
    ads_FsfPointFastener_material_referrer
};

/** 
Enum with association roles. */
enum ads_FsfPointFastener_weldConnectionsRolesEnm
{
    ads_FsfPointFastener_weldConnections_referent,
    ads_FsfPointFastener_weldConnections_referrer
};

/** 
Enum with association roles. */
enum ads_LoadChannelPairPSD_channelsRolesEnm
{
    ads_LoadChannelPairPSD_channels_referent,
    ads_LoadChannelPairPSD_channels_referrer
};

/** Enum with association roles. */
enum ads_LoadChannelPairPSD_psdRolesEnm
{
    ads_LoadChannelPairPSD_psd_referent,
    ads_LoadChannelPairPSD_psd_referrer
};

/** 
Enum with association roles. */
enum ads_Model_userMeanStressCorrectionFTablesRolesEnm
{
    ads_Model_userMeanStressCorrectionFTables_child,
    ads_Model_userMeanStressCorrectionFTables_parent
};

/** 
Enum with record members. */
enum ads_NonWeld_Override_FKM7MembersEnm
{
    ads_NonWeld_Override_FKM7_anisotropyFactor,
    ads_NonWeld_Override_FKM7_applyGJLFactor,
    ads_NonWeld_Override_FKM7_castingSafetyFactor,
    ads_NonWeld_Override_FKM7_castingType,
    ads_NonWeld_Override_FKM7_consequencesOfFailure,
    ads_NonWeld_Override_FKM7_effectiveDiameter,
    ads_NonWeld_Override_FKM7_loadFactor,
    ads_NonWeld_Override_FKM7_plasticNotchFactor,
    ads_NonWeld_Override_FKM7_sectionFactor,
    ads_NonWeld_Override_FKM7_sectionFactorCriterion,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod
};

enum ads_NonWeld_Override_FKM7_applyGJLFactorEnm
{
    ads_NonWeld_Override_FKM7_applyGJLFactor_NO,
    ads_NonWeld_Override_FKM7_applyGJLFactor_YES
};

enum ads_NonWeld_Override_FKM7_castingTypeEnm
{
    ads_NonWeld_Override_FKM7_castingType_NDT,
    ads_NonWeld_Override_FKM7_castingType_NO_NDT,
    ads_NonWeld_Override_FKM7_castingType_PREMIUM
};

enum ads_NonWeld_Override_FKM7_consequencesOfFailureEnm
{
    ads_NonWeld_Override_FKM7_consequencesOfFailure_HIGH,
    ads_NonWeld_Override_FKM7_consequencesOfFailure_MEAN,
    ads_NonWeld_Override_FKM7_consequencesOfFailure_MODERATE
};

enum ads_NonWeld_Override_FKM7_sectionFactorCriterionEnm
{
    ads_NonWeld_Override_FKM7_sectionFactorCriterion_ALL_MATERIALS,
    ads_NonWeld_Override_FKM7_sectionFactorCriterion_NONE,
    ads_NonWeld_Override_FKM7_sectionFactorCriterion_PLASTIC_SPOT
};

enum ads_NonWeld_Override_FKM7_surfaceTreatmentMethodEnm
{
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_CASE_HARDENING,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_COLD_ROLLING,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_NITRIDING,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_NONE,
    ads_NonWeld_Override_FKM7_surfaceTreatmentMethod_SHOT_PEENING
};

/** 
Enum with record members. */
enum ads_NonWeld_Override_FKM7_FatigueMembersEnm
{
    ads_NonWeld_Override_FKM7_Fatigue_anisotropyFactor,
    ads_NonWeld_Override_FKM7_Fatigue_applyGJLFactor,
    ads_NonWeld_Override_FKM7_Fatigue_castingSafetyFactor,
    ads_NonWeld_Override_FKM7_Fatigue_castingType,
    ads_NonWeld_Override_FKM7_Fatigue_consequencesOfFailure,
    ads_NonWeld_Override_FKM7_Fatigue_effectiveDiameter,
    ads_NonWeld_Override_FKM7_Fatigue_loadFactor,
    ads_NonWeld_Override_FKM7_Fatigue_plasticNotchFactor,
    ads_NonWeld_Override_FKM7_Fatigue_sectionFactor,
    ads_NonWeld_Override_FKM7_Fatigue_sectionFactorCriterion,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod,
    ads_NonWeld_Override_FKM7_Fatigue_coatingFactor,
    ads_NonWeld_Override_FKM7_Fatigue_coatingThickness,
    ads_NonWeld_Override_FKM7_Fatigue_estimatedFatigueNotchFactor,
    ads_NonWeld_Override_FKM7_Fatigue_ktkfRatio,
    ads_NonWeld_Override_FKM7_Fatigue_ktkfRatioTorsional,
    ads_NonWeld_Override_FKM7_Fatigue_materialSafetyFactor,
    ads_NonWeld_Override_FKM7_Fatigue_meanStressSensitivity,
    ads_NonWeld_Override_FKM7_Fatigue_meanTorsionalStressSensitivity,
    ads_NonWeld_Override_FKM7_Fatigue_regularInspections,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentFactor
};

enum ads_NonWeld_Override_FKM7_Fatigue_applyGJLFactorEnm
{
    ads_NonWeld_Override_FKM7_Fatigue_applyGJLFactor_NO,
    ads_NonWeld_Override_FKM7_Fatigue_applyGJLFactor_YES
};

enum ads_NonWeld_Override_FKM7_Fatigue_castingTypeEnm
{
    ads_NonWeld_Override_FKM7_Fatigue_castingType_NDT,
    ads_NonWeld_Override_FKM7_Fatigue_castingType_NO_NDT,
    ads_NonWeld_Override_FKM7_Fatigue_castingType_PREMIUM
};

enum ads_NonWeld_Override_FKM7_Fatigue_consequencesOfFailureEnm
{
    ads_NonWeld_Override_FKM7_Fatigue_consequencesOfFailure_HIGH,
    ads_NonWeld_Override_FKM7_Fatigue_consequencesOfFailure_MEAN,
    ads_NonWeld_Override_FKM7_Fatigue_consequencesOfFailure_MODERATE
};

enum ads_NonWeld_Override_FKM7_Fatigue_sectionFactorCriterionEnm
{
    ads_NonWeld_Override_FKM7_Fatigue_sectionFactorCriterion_ALL_MATERIALS,
    ads_NonWeld_Override_FKM7_Fatigue_sectionFactorCriterion_NONE,
    ads_NonWeld_Override_FKM7_Fatigue_sectionFactorCriterion_PLASTIC_SPOT
};

enum ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethodEnm
{
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_CASE_HARDENING,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_COLD_ROLLING,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_NITRIDING,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_NONE,
    ads_NonWeld_Override_FKM7_Fatigue_surfaceTreatmentMethod_SHOT_PEENING
};

/** 
Enum with record members. */
enum ads_NonWeld_Override_FKM7_StaticMembersEnm
{
    ads_NonWeld_Override_FKM7_Static_anisotropyFactor,
    ads_NonWeld_Override_FKM7_Static_applyGJLFactor,
    ads_NonWeld_Override_FKM7_Static_castingSafetyFactor,
    ads_NonWeld_Override_FKM7_Static_castingType,
    ads_NonWeld_Override_FKM7_Static_consequencesOfFailure,
    ads_NonWeld_Override_FKM7_Static_effectiveDiameter,
    ads_NonWeld_Override_FKM7_Static_loadFactor,
    ads_NonWeld_Override_FKM7_Static_plasticNotchFactor,
    ads_NonWeld_Override_FKM7_Static_sectionFactor,
    ads_NonWeld_Override_FKM7_Static_sectionFactorCriterion,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod,
    ads_NonWeld_Override_FKM7_Static_assessmentOfFlowFactor,
    ads_NonWeld_Override_FKM7_Static_assessmentOfFractureFactor,
    ads_NonWeld_Override_FKM7_Static_elevatedFlowFactor,
    ads_NonWeld_Override_FKM7_Static_elevatedFracturesFactor,
    ads_NonWeld_Override_FKM7_Static_loadOccurrence
};

enum ads_NonWeld_Override_FKM7_Static_applyGJLFactorEnm
{
    ads_NonWeld_Override_FKM7_Static_applyGJLFactor_NO,
    ads_NonWeld_Override_FKM7_Static_applyGJLFactor_YES
};

enum ads_NonWeld_Override_FKM7_Static_castingTypeEnm
{
    ads_NonWeld_Override_FKM7_Static_castingType_NDT,
    ads_NonWeld_Override_FKM7_Static_castingType_NO_NDT,
    ads_NonWeld_Override_FKM7_Static_castingType_PREMIUM
};

enum ads_NonWeld_Override_FKM7_Static_consequencesOfFailureEnm
{
    ads_NonWeld_Override_FKM7_Static_consequencesOfFailure_HIGH,
    ads_NonWeld_Override_FKM7_Static_consequencesOfFailure_MEAN,
    ads_NonWeld_Override_FKM7_Static_consequencesOfFailure_MODERATE
};

enum ads_NonWeld_Override_FKM7_Static_sectionFactorCriterionEnm
{
    ads_NonWeld_Override_FKM7_Static_sectionFactorCriterion_ALL_MATERIALS,
    ads_NonWeld_Override_FKM7_Static_sectionFactorCriterion_NONE,
    ads_NonWeld_Override_FKM7_Static_sectionFactorCriterion_PLASTIC_SPOT
};

enum ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethodEnm
{
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_CASE_HARDENING,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_COLD_ROLLING,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_NITRIDING,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_NONE,
    ads_NonWeld_Override_FKM7_Static_surfaceTreatmentMethod_SHOT_PEENING
};

enum ads_NonWeld_Override_FKM7_Static_loadOccurrenceEnm
{
    ads_NonWeld_Override_FKM7_Static_loadOccurrence_HIGH,
    ads_NonWeld_Override_FKM7_Static_loadOccurrence_LOW
};

/** 
Enum with association roles. */
enum ads_NonWeld_Override_regionRolesEnm
{
    ads_NonWeld_Override_region_referent,
    ads_NonWeld_Override_region_referrer
};

/** 
Enum with association roles. */
enum ads_OutputRequest_perFatigueEventOutputRolesEnm
{
    ads_OutputRequest_perFatigueEventOutput_referent,
    ads_OutputRequest_perFatigueEventOutput_referrer
};

/** 
Enum with record members. */
enum ads_SignalReferenceMembersEnm
{
    ads_SignalReference_channel,
    ads_SignalReference_signalAuxId
};

/** 
Enum with record members. */
enum ads_SpotWeldSetMembersEnm
{
    ads_SpotWeldSet_nuggetDiameter,
    ads_SpotWeldSet_nuggetDiameterSource
};

enum ads_SpotWeldSet_nuggetDiameterSourceEnm
{
    ads_SpotWeldSet_nuggetDiameterSource_FASTENER,
    ads_SpotWeldSet_nuggetDiameterSource_GEOMETRY,
    ads_SpotWeldSet_nuggetDiameterSource_SPECIFY
};

/** 
Enum with record members. */
enum ads_SpotWeldSet_StandardMembersEnm
{
    ads_SpotWeldSet_Standard_nuggetDiameter,
    ads_SpotWeldSet_Standard_nuggetDiameterSource,
    ads_SpotWeldSet_Standard_IR_FunctionType
};

enum ads_SpotWeldSet_Standard_nuggetDiameterSourceEnm
{
    ads_SpotWeldSet_Standard_nuggetDiameterSource_FASTENER,
    ads_SpotWeldSet_Standard_nuggetDiameterSource_GEOMETRY,
    ads_SpotWeldSet_Standard_nuggetDiameterSource_SPECIFY
};

enum ads_SpotWeldSet_Standard_IR_FunctionTypeEnm
{
    ads_SpotWeldSet_Standard_IR_FunctionType_DISPLACEMENT_CONTROL,
    ads_SpotWeldSet_Standard_IR_FunctionType_LOAD_CONTROL
};

/** 
Enum with association roles. */
enum ads_SpotWeldSet_fsfPointFastenersRolesEnm
{
    ads_SpotWeldSet_fsfPointFasteners_child,
    ads_SpotWeldSet_fsfPointFasteners_parent
};

/** 
Enum with association roles. */
enum ads_SpotWeldSet_materialRolesEnm
{
    ads_SpotWeldSet_material_referent,
    ads_SpotWeldSet_material_referrer
};

/** 
Enum with association roles. */
enum ads_SpotWeldSet_weldConnectionsRolesEnm
{
    ads_SpotWeldSet_weldConnections_referent,
    ads_SpotWeldSet_weldConnections_referrer
};

/** 
Enum with record members. */
enum ads_StepRefInfoMembersEnm
{
    ads_StepRefInfo_step
};

/** 
Enum with association roles. */
enum ads_StepRefInfo_simDocRolesEnm
{
    ads_StepRefInfo_simDoc_referent,
    ads_StepRefInfo_simDoc_referrer
};

/** 
Enum with record members. */
enum ads_Step_Gen_FatigueMembersEnm
{
    ads_Step_Gen_Fatigue_designSensitivity,
    ads_Step_Gen_Fatigue_dsa,
    ads_Step_Gen_Fatigue_beginningTime
};

enum ads_Step_Gen_Fatigue_designSensitivityEnm
{
    ads_Step_Gen_Fatigue_designSensitivity_ADJOINT,
    ads_Step_Gen_Fatigue_designSensitivity_NONE
};

/** 
Enum with record members. */
enum ads_Step_Gen_Fatigue_FKMMembersEnm
{
    ads_Step_Gen_Fatigue_FKM_designSensitivity,
    ads_Step_Gen_Fatigue_FKM_dsa,
    ads_Step_Gen_Fatigue_FKM_beginningTime,
    ads_Step_Gen_Fatigue_FKM_damageAccumulationType,
    ads_Step_Gen_Fatigue_FKM_guidelineEdition,
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection
};

enum ads_Step_Gen_Fatigue_FKM_designSensitivityEnm
{
    ads_Step_Gen_Fatigue_FKM_designSensitivity_ADJOINT,
    ads_Step_Gen_Fatigue_FKM_designSensitivity_NONE
};

enum ads_Step_Gen_Fatigue_FKM_damageAccumulationTypeEnm
{
    ads_Step_Gen_Fatigue_FKM_damageAccumulationType_CONSISTENT_MINERS,
    ads_Step_Gen_Fatigue_FKM_damageAccumulationType_ELEMENTARY_MINERS
};

enum ads_Step_Gen_Fatigue_FKM_meanStressCorrectionEnm
{
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection_F1_CONSTANT_MEAN_STRESS,
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection_F2_CONSTANT_STRESS_RATIO,
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection_F3_CONSTANT_MINIMUM_STRESS,
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection_F4_CONSTANT_MAXIMUM_STRESS,
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection_NO_OVER_LOADING,
    ads_Step_Gen_Fatigue_FKM_meanStressCorrection_USE_MATERIAL_DEFAULT
};

/** 
Enum with association roles. */
enum ads_Step_Gen_Fatigue_fatigueAlgorithmOverrideRolesEnm
{
    ads_Step_Gen_Fatigue_fatigueAlgorithmOverride_child,
    ads_Step_Gen_Fatigue_fatigueAlgorithmOverride_parent
};

/** 
Enum with association roles. */
enum ads_Step_Gen_Fatigue_fatigueLoadingRolesEnm
{
    ads_Step_Gen_Fatigue_fatigueLoading_child,
    ads_Step_Gen_Fatigue_fatigueLoading_parent
};

/** 
Enum with association roles. */
enum ads_Step_Gen_Fatigue_fatigueMaterialOverrideRolesEnm
{
    ads_Step_Gen_Fatigue_fatigueMaterialOverride_child,
    ads_Step_Gen_Fatigue_fatigueMaterialOverride_parent
};

/** 
Enum with association roles. */
enum ads_Step_Gen_Fatigue_spotWeldSetsRolesEnm
{
    ads_Step_Gen_Fatigue_spotWeldSets_child,
    ads_Step_Gen_Fatigue_spotWeldSets_parent
};

/** 
Enum with record members. */
enum ads_Step_Gen_StaticFKMMembersEnm
{
    ads_Step_Gen_StaticFKM_designSensitivity,
    ads_Step_Gen_StaticFKM_dsa,
    ads_Step_Gen_StaticFKM_beginningTime,
    ads_Step_Gen_StaticFKM_globalTemperature,
    ads_Step_Gen_StaticFKM_guidelineEdition,
    ads_Step_Gen_StaticFKM_operatingTimeAtTemperature,
    ads_Step_Gen_StaticFKM_staticLoadType,
    ads_Step_Gen_StaticFKM_useFieldTemperatures
};

enum ads_Step_Gen_StaticFKM_designSensitivityEnm
{
    ads_Step_Gen_StaticFKM_designSensitivity_ADJOINT,
    ads_Step_Gen_StaticFKM_designSensitivity_NONE
};

enum ads_Step_Gen_StaticFKM_staticLoadTypeEnm
{
    ads_Step_Gen_StaticFKM_staticLoadType_MULTI_FRAME,
    ads_Step_Gen_StaticFKM_staticLoadType_SINGLE_FRAME,
    ads_Step_Gen_StaticFKM_staticLoadType_WORST_FATIGUE_LOAD
};

/** 
Enum with association roles. */
enum ads_Step_Gen_StaticFKM_fkmStaticLoadFramesRolesEnm
{
    ads_Step_Gen_StaticFKM_fkmStaticLoadFrames_child,
    ads_Step_Gen_StaticFKM_fkmStaticLoadFrames_parent
};

/** 
Enum with association roles. */
enum ads_Step_Gen_StaticFKM_fkmWorstOfFatigueLoadingRolesEnm
{
    ads_Step_Gen_StaticFKM_fkmWorstOfFatigueLoading_referent,
    ads_Step_Gen_StaticFKM_fkmWorstOfFatigueLoading_referrer
};

/** 
Enum with association roles. */
enum ads_Step_Gen_nonWeldOverrideRolesEnm
{
    ads_Step_Gen_nonWeldOverride_child,
    ads_Step_Gen_nonWeldOverride_parent
};

/** 
Enum with association roles. */
enum ads_Step_Gen_weldsDefinitionRolesEnm
{
    ads_Step_Gen_weldsDefinition_child,
    ads_Step_Gen_weldsDefinition_parent
};

/** 
Enum with record members. */
enum ads_SurfaceFinish_KfVMembersEnm
{
    ads_SurfaceFinish_KfV_kfValue
};

/** 
Enum with record members. */
enum ads_SurfaceFinish_KtVMembersEnm
{
    ads_SurfaceFinish_KtV_ktValue
};

/** 
Enum with record members. */
enum ads_SurfaceFinish_RNMembersEnm
{
    ads_SurfaceFinish_RN_roughnessName
};

/** 
Enum with record members. */
enum ads_SurfaceFinish_RVMembersEnm
{
    ads_SurfaceFinish_RV_roughnessType,
    ads_SurfaceFinish_RV_roughnessValue
};

enum ads_SurfaceFinish_RV_roughnessTypeEnm
{
    ads_SurfaceFinish_RV_roughnessType_Ra,
    ads_SurfaceFinish_RV_roughnessType_Rz
};

/** 
Enum with association roles. */
enum ads_SurfaceFinish_surfaceFinishPropRolesEnm
{
    ads_SurfaceFinish_surfaceFinishProp_referent,
    ads_SurfaceFinish_surfaceFinishProp_referrer
};

/** 
Enum with record members. */
enum ads_SurfaceStressExtrapolationMembersEnm
{
    ads_SurfaceStressExtrapolation_meshGranularity,
    ads_SurfaceStressExtrapolation_seRuleTypeA,
    ads_SurfaceStressExtrapolation_seRuleTypeB,
    ads_SurfaceStressExtrapolation_seTypeAThickness,
    ads_SurfaceStressExtrapolation_seTypeAThicknessType,
    ads_SurfaceStressExtrapolation_stressExtrapolationThreshold,
    ads_SurfaceStressExtrapolation_stressExtrapolationType,
    ads_SurfaceStressExtrapolation_typeACutOffThickness
};

enum ads_SurfaceStressExtrapolation_meshGranularityEnm
{
    ads_SurfaceStressExtrapolation_meshGranularity_AUTO_DETECT,
    ads_SurfaceStressExtrapolation_meshGranularity_COARSE,
    ads_SurfaceStressExtrapolation_meshGranularity_FINE
};

enum ads_SurfaceStressExtrapolation_seRuleTypeAEnm
{
    ads_SurfaceStressExtrapolation_seRuleTypeA_AUTO_SELECT,
    ads_SurfaceStressExtrapolation_seRuleTypeA_RULE_4_10T,
    ads_SurfaceStressExtrapolation_seRuleTypeA_RULE_4_9_14T,
    ads_SurfaceStressExtrapolation_seRuleTypeA_RULE_5_15T
};

enum ads_SurfaceStressExtrapolation_seRuleTypeBEnm
{
    ads_SurfaceStressExtrapolation_seRuleTypeB_AUTO_SELECT,
    ads_SurfaceStressExtrapolation_seRuleTypeB_RULE_4_8_12MM,
    ads_SurfaceStressExtrapolation_seRuleTypeB_RULE_5_15MM
};

enum ads_SurfaceStressExtrapolation_seTypeAThicknessTypeEnm
{
    ads_SurfaceStressExtrapolation_seTypeAThicknessType_AUTO_DETECT,
    ads_SurfaceStressExtrapolation_seTypeAThicknessType_SPECIFY
};

enum ads_SurfaceStressExtrapolation_stressExtrapolationTypeEnm
{
    ads_SurfaceStressExtrapolation_stressExtrapolationType_AUTO_DETECT,
    ads_SurfaceStressExtrapolation_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_SurfaceStressExtrapolation_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_SurfaceStressExtrapolation_stressExtrapolationType_TYPEA,
    ads_SurfaceStressExtrapolation_stressExtrapolationType_TYPEB
};

/** 
Enum with record members. */
enum ads_WeldEndCorrectionMembersEnm
{
    ads_WeldEndCorrection_endCorrectionMethod,
    ads_WeldEndCorrection_endCorrectionWrapAngle
};

enum ads_WeldEndCorrection_endCorrectionMethodEnm
{
    ads_WeldEndCorrection_endCorrectionMethod_VIRTUAL_NODE,
    ads_WeldEndCorrection_endCorrectionMethod_VIRTUAL_NODE_RESCALED
};

/** 
Enum with association roles. */
enum ads_WeldEndCorrection_endCorrectionPointsRolesEnm
{
    ads_WeldEndCorrection_endCorrectionPoints_referent,
    ads_WeldEndCorrection_endCorrectionPoints_referrer
};

/** 
Enum with record members. */
enum ads_WeldFailureMode_SensedMembersEnm
{
    ads_WeldFailureMode_Sensed_failureSense
};

enum ads_WeldFailureMode_Sensed_failureSenseEnm
{
    ads_WeldFailureMode_Sensed_failureSense_FROM_ROOT,
    ads_WeldFailureMode_Sensed_failureSense_FROM_SURFACE,
    ads_WeldFailureMode_Sensed_failureSense_SURFACE_AND_ROOT
};

/** 
Enum with association roles. */
enum ads_WeldFailureMode_crackDomainSelectionRolesEnm
{
    ads_WeldFailureMode_crackDomainSelection_referent,
    ads_WeldFailureMode_crackDomainSelection_referrer
};

/** 
Enum with association roles. */
enum ads_WeldFailureMode_crackSurfaceSelectionRolesEnm
{
    ads_WeldFailureMode_crackSurfaceSelection_referent,
    ads_WeldFailureMode_crackSurfaceSelection_referrer
};

/** 
Enum with association roles. */
enum ads_WeldFailureMode_materialRolesEnm
{
    ads_WeldFailureMode_material_referent,
    ads_WeldFailureMode_material_referrer
};

/** 
Enum with association roles. */
enum ads_WeldSubRegion_fusionMaterialRolesEnm
{
    ads_WeldSubRegion_fusionMaterial_referent,
    ads_WeldSubRegion_fusionMaterial_referrer
};

/** 
Enum with association roles. */
enum ads_WeldSubRegion_rootMaterialRolesEnm
{
    ads_WeldSubRegion_rootMaterial_referent,
    ads_WeldSubRegion_rootMaterial_referrer
};

/** 
Enum with association roles. */
enum ads_WeldSubRegion_throatMaterialRolesEnm
{
    ads_WeldSubRegion_throatMaterial_referent,
    ads_WeldSubRegion_throatMaterial_referrer
};

/** 
Enum with association roles. */
enum ads_WeldSubRegion_toeMaterialRolesEnm
{
    ads_WeldSubRegion_toeMaterial_referent,
    ads_WeldSubRegion_toeMaterial_referrer
};

/** 
Enum with association roles. */
enum ads_WeldSubRegion_weldSubBodyRolesEnm
{
    ads_WeldSubRegion_weldSubBody_referent,
    ads_WeldSubRegion_weldSubBody_referrer
};

/** 
Enum with record members. */
enum ads_WeldsDefinitionMembersEnm
{
    ads_WeldsDefinition_meshGranularity,
    ads_WeldsDefinition_seRuleTypeA,
    ads_WeldsDefinition_seRuleTypeB,
    ads_WeldsDefinition_seTypeAThickness,
    ads_WeldsDefinition_seTypeAThicknessType,
    ads_WeldsDefinition_stressExtrapolationThreshold,
    ads_WeldsDefinition_stressExtrapolationType,
    ads_WeldsDefinition_structuralStressSourceType,
    ads_WeldsDefinition_typeACutOffThickness,
    ads_WeldsDefinition_weldLineSplitAngle,
    ads_WeldsDefinition_weldLineSplitTolerance
};

enum ads_WeldsDefinition_meshGranularityEnm
{
    ads_WeldsDefinition_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_meshGranularity_COARSE,
    ads_WeldsDefinition_meshGranularity_FINE
};

enum ads_WeldsDefinition_seRuleTypeAEnm
{
    ads_WeldsDefinition_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_seRuleTypeBEnm
{
    ads_WeldsDefinition_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_structuralStressSourceType_NONE,
    ads_WeldsDefinition_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_structuralStressSourceType_VOLVO_NODAL_FORCE
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_FKMMembersEnm
{
    ads_WeldsDefinition_FKM_meshGranularity,
    ads_WeldsDefinition_FKM_seRuleTypeA,
    ads_WeldsDefinition_FKM_seRuleTypeB,
    ads_WeldsDefinition_FKM_seTypeAThickness,
    ads_WeldsDefinition_FKM_seTypeAThicknessType,
    ads_WeldsDefinition_FKM_stressExtrapolationThreshold,
    ads_WeldsDefinition_FKM_stressExtrapolationType,
    ads_WeldsDefinition_FKM_structuralStressSourceType,
    ads_WeldsDefinition_FKM_typeACutOffThickness,
    ads_WeldsDefinition_FKM_weldLineSplitAngle,
    ads_WeldsDefinition_FKM_weldLineSplitTolerance,
    ads_WeldsDefinition_FKM_consequencesOfFailure,
    ads_WeldsDefinition_FKM_loadFactor,
    ads_WeldsDefinition_FKM_plasticNotchFactor,
    ads_WeldsDefinition_FKM_sectionFactor,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod,
    ads_WeldsDefinition_FKM_weldFactor,
    ads_WeldsDefinition_FKM_weldQuality,
    ads_WeldsDefinition_FKM_weldType
};

enum ads_WeldsDefinition_FKM_meshGranularityEnm
{
    ads_WeldsDefinition_FKM_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_FKM_meshGranularity_COARSE,
    ads_WeldsDefinition_FKM_meshGranularity_FINE
};

enum ads_WeldsDefinition_FKM_seRuleTypeAEnm
{
    ads_WeldsDefinition_FKM_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_FKM_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_FKM_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_FKM_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_FKM_seRuleTypeBEnm
{
    ads_WeldsDefinition_FKM_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_FKM_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_FKM_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_FKM_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_FKM_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_FKM_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_FKM_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_FKM_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_FKM_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_FKM_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_FKM_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_FKM_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_FKM_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_FKM_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_FKM_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_FKM_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_FKM_structuralStressSourceType_NONE,
    ads_WeldsDefinition_FKM_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_FKM_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_FKM_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_FKM_consequencesOfFailureEnm
{
    ads_WeldsDefinition_FKM_consequencesOfFailure_HIGH,
    ads_WeldsDefinition_FKM_consequencesOfFailure_MEAN,
    ads_WeldsDefinition_FKM_consequencesOfFailure_MODERATE
};

enum ads_WeldsDefinition_FKM_surfaceTreatmentMethodEnm
{
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_CASE_HARDENING,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_COLD_ROLLING,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_NITRIDING,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_NONE,
    ads_WeldsDefinition_FKM_surfaceTreatmentMethod_SHOT_PEENING
};

enum ads_WeldsDefinition_FKM_weldQualityEnm
{
    ads_WeldsDefinition_FKM_weldQuality_NOT_VERIFIED,
    ads_WeldsDefinition_FKM_weldQuality_VERIFIED
};

enum ads_WeldsDefinition_FKM_weldTypeEnm
{
    ads_WeldsDefinition_FKM_weldType_FILLET_WELD,
    ads_WeldsDefinition_FKM_weldType_FULL_PENETRATION,
    ads_WeldsDefinition_FKM_weldType_PARTIAL_PENETRATION,
    ads_WeldsDefinition_FKM_weldType_WELD_WITH_BACK_WELD
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_FKM7MembersEnm
{
    ads_WeldsDefinition_FKM7_meshGranularity,
    ads_WeldsDefinition_FKM7_seRuleTypeA,
    ads_WeldsDefinition_FKM7_seRuleTypeB,
    ads_WeldsDefinition_FKM7_seTypeAThickness,
    ads_WeldsDefinition_FKM7_seTypeAThicknessType,
    ads_WeldsDefinition_FKM7_stressExtrapolationThreshold,
    ads_WeldsDefinition_FKM7_stressExtrapolationType,
    ads_WeldsDefinition_FKM7_structuralStressSourceType,
    ads_WeldsDefinition_FKM7_typeACutOffThickness,
    ads_WeldsDefinition_FKM7_weldLineSplitAngle,
    ads_WeldsDefinition_FKM7_weldLineSplitTolerance,
    ads_WeldsDefinition_FKM7_applyGJLFactor,
    ads_WeldsDefinition_FKM7_consequencesOfFailure,
    ads_WeldsDefinition_FKM7_loadFactor,
    ads_WeldsDefinition_FKM7_plasticNotchFactor,
    ads_WeldsDefinition_FKM7_sectionFactor,
    ads_WeldsDefinition_FKM7_sectionFactorCriterion
};

enum ads_WeldsDefinition_FKM7_meshGranularityEnm
{
    ads_WeldsDefinition_FKM7_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_meshGranularity_COARSE,
    ads_WeldsDefinition_FKM7_meshGranularity_FINE
};

enum ads_WeldsDefinition_FKM7_seRuleTypeAEnm
{
    ads_WeldsDefinition_FKM7_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_FKM7_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_FKM7_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_FKM7_seRuleTypeBEnm
{
    ads_WeldsDefinition_FKM7_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_FKM7_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_FKM7_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_FKM7_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_FKM7_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_FKM7_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_FKM7_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_FKM7_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_FKM7_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_FKM7_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_FKM7_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_FKM7_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_FKM7_structuralStressSourceType_NONE,
    ads_WeldsDefinition_FKM7_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_FKM7_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_FKM7_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_FKM7_applyGJLFactorEnm
{
    ads_WeldsDefinition_FKM7_applyGJLFactor_NO,
    ads_WeldsDefinition_FKM7_applyGJLFactor_YES
};

enum ads_WeldsDefinition_FKM7_consequencesOfFailureEnm
{
    ads_WeldsDefinition_FKM7_consequencesOfFailure_HIGH,
    ads_WeldsDefinition_FKM7_consequencesOfFailure_MEAN,
    ads_WeldsDefinition_FKM7_consequencesOfFailure_MODERATE
};

enum ads_WeldsDefinition_FKM7_sectionFactorCriterionEnm
{
    ads_WeldsDefinition_FKM7_sectionFactorCriterion_ALL_MATERIALS,
    ads_WeldsDefinition_FKM7_sectionFactorCriterion_NONE,
    ads_WeldsDefinition_FKM7_sectionFactorCriterion_PLASTIC_SPOT
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_FKM7_FatigueMembersEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_meshGranularity,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeA,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeB,
    ads_WeldsDefinition_FKM7_Fatigue_seTypeAThickness,
    ads_WeldsDefinition_FKM7_Fatigue_seTypeAThicknessType,
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationThreshold,
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationType,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType,
    ads_WeldsDefinition_FKM7_Fatigue_typeACutOffThickness,
    ads_WeldsDefinition_FKM7_Fatigue_weldLineSplitAngle,
    ads_WeldsDefinition_FKM7_Fatigue_weldLineSplitTolerance,
    ads_WeldsDefinition_FKM7_Fatigue_applyGJLFactor,
    ads_WeldsDefinition_FKM7_Fatigue_consequencesOfFailure,
    ads_WeldsDefinition_FKM7_Fatigue_loadFactor,
    ads_WeldsDefinition_FKM7_Fatigue_plasticNotchFactor,
    ads_WeldsDefinition_FKM7_Fatigue_sectionFactor,
    ads_WeldsDefinition_FKM7_Fatigue_sectionFactorCriterion,
    ads_WeldsDefinition_FKM7_Fatigue_jointCategory,
    ads_WeldsDefinition_FKM7_Fatigue_materialSafetyFactor,
    ads_WeldsDefinition_FKM7_Fatigue_meanStressSensitivity,
    ads_WeldsDefinition_FKM7_Fatigue_meanTorsionalStressSensitivity,
    ads_WeldsDefinition_FKM7_Fatigue_parallelFATClass,
    ads_WeldsDefinition_FKM7_Fatigue_perpendicularFATClass,
    ads_WeldsDefinition_FKM7_Fatigue_regularInspections,
    ads_WeldsDefinition_FKM7_Fatigue_residualStress,
    ads_WeldsDefinition_FKM7_Fatigue_residualStressFactor,
    ads_WeldsDefinition_FKM7_Fatigue_residualTorsionalStressFactor,
    ads_WeldsDefinition_FKM7_Fatigue_surfaceTreatmentFactor,
    ads_WeldsDefinition_FKM7_Fatigue_torsionalFATClass,
    ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethod
};

enum ads_WeldsDefinition_FKM7_Fatigue_meshGranularityEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_Fatigue_meshGranularity_COARSE,
    ads_WeldsDefinition_FKM7_Fatigue_meshGranularity_FINE
};

enum ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeAEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeBEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_FKM7_Fatigue_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_FKM7_Fatigue_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_Fatigue_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_FKM7_Fatigue_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_NONE,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_FKM7_Fatigue_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_FKM7_Fatigue_applyGJLFactorEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_applyGJLFactor_NO,
    ads_WeldsDefinition_FKM7_Fatigue_applyGJLFactor_YES
};

enum ads_WeldsDefinition_FKM7_Fatigue_consequencesOfFailureEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_consequencesOfFailure_HIGH,
    ads_WeldsDefinition_FKM7_Fatigue_consequencesOfFailure_MEAN,
    ads_WeldsDefinition_FKM7_Fatigue_consequencesOfFailure_MODERATE
};

enum ads_WeldsDefinition_FKM7_Fatigue_sectionFactorCriterionEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_sectionFactorCriterion_ALL_MATERIALS,
    ads_WeldsDefinition_FKM7_Fatigue_sectionFactorCriterion_NONE,
    ads_WeldsDefinition_FKM7_Fatigue_sectionFactorCriterion_PLASTIC_SPOT
};

enum ads_WeldsDefinition_FKM7_Fatigue_jointCategoryEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_jointCategory_BUTT_JOINT_AS_WELDED,
    ads_WeldsDefinition_FKM7_Fatigue_jointCategory_BUTT_JOINT_GROUND,
    ads_WeldsDefinition_FKM7_Fatigue_jointCategory_T_JOINT_AS_WELDED,
    ads_WeldsDefinition_FKM7_Fatigue_jointCategory_T_JOINT_GROUND
};

enum ads_WeldsDefinition_FKM7_Fatigue_residualStressEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_residualStress_HIGH,
    ads_WeldsDefinition_FKM7_Fatigue_residualStress_LOW,
    ads_WeldsDefinition_FKM7_Fatigue_residualStress_MODERATE
};

enum ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethodEnm
{
    ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethod_GRINDING,
    ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethod_HAMMER_NEEDLE_PEENING,
    ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethod_NONE,
    ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethod_SHOT_PEENING,
    ads_WeldsDefinition_FKM7_Fatigue_weldSurfaceTreatmentMethod_TIG_DRESSING
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_FKM7_StaticMembersEnm
{
    ads_WeldsDefinition_FKM7_Static_meshGranularity,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeA,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeB,
    ads_WeldsDefinition_FKM7_Static_seTypeAThickness,
    ads_WeldsDefinition_FKM7_Static_seTypeAThicknessType,
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationThreshold,
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationType,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType,
    ads_WeldsDefinition_FKM7_Static_typeACutOffThickness,
    ads_WeldsDefinition_FKM7_Static_weldLineSplitAngle,
    ads_WeldsDefinition_FKM7_Static_weldLineSplitTolerance,
    ads_WeldsDefinition_FKM7_Static_applyGJLFactor,
    ads_WeldsDefinition_FKM7_Static_consequencesOfFailure,
    ads_WeldsDefinition_FKM7_Static_loadFactor,
    ads_WeldsDefinition_FKM7_Static_plasticNotchFactor,
    ads_WeldsDefinition_FKM7_Static_sectionFactor,
    ads_WeldsDefinition_FKM7_Static_sectionFactorCriterion,
    ads_WeldsDefinition_FKM7_Static_assessmentOfFlowFactor,
    ads_WeldsDefinition_FKM7_Static_assessmentOfFractureFactor,
    ads_WeldsDefinition_FKM7_Static_castingSafetyFactor,
    ads_WeldsDefinition_FKM7_Static_castingType,
    ads_WeldsDefinition_FKM7_Static_elevatedFlowFactor,
    ads_WeldsDefinition_FKM7_Static_elevatedFracturesFactor,
    ads_WeldsDefinition_FKM7_Static_loadOccurrence,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod
};

enum ads_WeldsDefinition_FKM7_Static_meshGranularityEnm
{
    ads_WeldsDefinition_FKM7_Static_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_Static_meshGranularity_COARSE,
    ads_WeldsDefinition_FKM7_Static_meshGranularity_FINE
};

enum ads_WeldsDefinition_FKM7_Static_seRuleTypeAEnm
{
    ads_WeldsDefinition_FKM7_Static_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_FKM7_Static_seRuleTypeBEnm
{
    ads_WeldsDefinition_FKM7_Static_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_FKM7_Static_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_FKM7_Static_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_FKM7_Static_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_Static_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_FKM7_Static_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_FKM7_Static_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_FKM7_Static_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_NONE,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_FKM7_Static_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_FKM7_Static_applyGJLFactorEnm
{
    ads_WeldsDefinition_FKM7_Static_applyGJLFactor_NO,
    ads_WeldsDefinition_FKM7_Static_applyGJLFactor_YES
};

enum ads_WeldsDefinition_FKM7_Static_consequencesOfFailureEnm
{
    ads_WeldsDefinition_FKM7_Static_consequencesOfFailure_HIGH,
    ads_WeldsDefinition_FKM7_Static_consequencesOfFailure_MEAN,
    ads_WeldsDefinition_FKM7_Static_consequencesOfFailure_MODERATE
};

enum ads_WeldsDefinition_FKM7_Static_sectionFactorCriterionEnm
{
    ads_WeldsDefinition_FKM7_Static_sectionFactorCriterion_ALL_MATERIALS,
    ads_WeldsDefinition_FKM7_Static_sectionFactorCriterion_NONE,
    ads_WeldsDefinition_FKM7_Static_sectionFactorCriterion_PLASTIC_SPOT
};

enum ads_WeldsDefinition_FKM7_Static_castingTypeEnm
{
    ads_WeldsDefinition_FKM7_Static_castingType_NDT,
    ads_WeldsDefinition_FKM7_Static_castingType_NO_NDT,
    ads_WeldsDefinition_FKM7_Static_castingType_PREMIUM
};

enum ads_WeldsDefinition_FKM7_Static_loadOccurrenceEnm
{
    ads_WeldsDefinition_FKM7_Static_loadOccurrence_HIGH,
    ads_WeldsDefinition_FKM7_Static_loadOccurrence_LOW
};

enum ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethodEnm
{
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_CASE_HARDENING,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_COLD_ROLLING,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_NITRIDING,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_NONE,
    ads_WeldsDefinition_FKM7_Static_surfaceTreatmentMethod_SHOT_PEENING
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_FKM_FatigueMembersEnm
{
    ads_WeldsDefinition_FKM_Fatigue_meshGranularity,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeA,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeB,
    ads_WeldsDefinition_FKM_Fatigue_seTypeAThickness,
    ads_WeldsDefinition_FKM_Fatigue_seTypeAThicknessType,
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationThreshold,
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationType,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType,
    ads_WeldsDefinition_FKM_Fatigue_typeACutOffThickness,
    ads_WeldsDefinition_FKM_Fatigue_weldLineSplitAngle,
    ads_WeldsDefinition_FKM_Fatigue_weldLineSplitTolerance,
    ads_WeldsDefinition_FKM_Fatigue_consequencesOfFailure,
    ads_WeldsDefinition_FKM_Fatigue_loadFactor,
    ads_WeldsDefinition_FKM_Fatigue_plasticNotchFactor,
    ads_WeldsDefinition_FKM_Fatigue_sectionFactor,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod,
    ads_WeldsDefinition_FKM_Fatigue_weldFactor,
    ads_WeldsDefinition_FKM_Fatigue_weldQuality,
    ads_WeldsDefinition_FKM_Fatigue_weldType,
    ads_WeldsDefinition_FKM_Fatigue_materialSafetyFactor,
    ads_WeldsDefinition_FKM_Fatigue_meanStressSensitivity,
    ads_WeldsDefinition_FKM_Fatigue_meanTorsionalStressSensitivity,
    ads_WeldsDefinition_FKM_Fatigue_parallelFATClass,
    ads_WeldsDefinition_FKM_Fatigue_perpendicularFATClass,
    ads_WeldsDefinition_FKM_Fatigue_regularInspections,
    ads_WeldsDefinition_FKM_Fatigue_residualStress,
    ads_WeldsDefinition_FKM_Fatigue_residualStressFactor,
    ads_WeldsDefinition_FKM_Fatigue_residualTorsionalStressFactor,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentFactor,
    ads_WeldsDefinition_FKM_Fatigue_temperatureFactor,
    ads_WeldsDefinition_FKM_Fatigue_thicknessExponent,
    ads_WeldsDefinition_FKM_Fatigue_thicknessFactorCase,
    ads_WeldsDefinition_FKM_Fatigue_torsionalFATClass
};

enum ads_WeldsDefinition_FKM_Fatigue_meshGranularityEnm
{
    ads_WeldsDefinition_FKM_Fatigue_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_FKM_Fatigue_meshGranularity_COARSE,
    ads_WeldsDefinition_FKM_Fatigue_meshGranularity_FINE
};

enum ads_WeldsDefinition_FKM_Fatigue_seRuleTypeAEnm
{
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_FKM_Fatigue_seRuleTypeBEnm
{
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_FKM_Fatigue_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_FKM_Fatigue_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_FKM_Fatigue_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_FKM_Fatigue_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_FKM_Fatigue_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_NONE,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_FKM_Fatigue_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_FKM_Fatigue_consequencesOfFailureEnm
{
    ads_WeldsDefinition_FKM_Fatigue_consequencesOfFailure_HIGH,
    ads_WeldsDefinition_FKM_Fatigue_consequencesOfFailure_MEAN,
    ads_WeldsDefinition_FKM_Fatigue_consequencesOfFailure_MODERATE
};

enum ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethodEnm
{
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_CASE_HARDENING,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_COLD_ROLLING,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_NITRIDING,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_NONE,
    ads_WeldsDefinition_FKM_Fatigue_surfaceTreatmentMethod_SHOT_PEENING
};

enum ads_WeldsDefinition_FKM_Fatigue_weldQualityEnm
{
    ads_WeldsDefinition_FKM_Fatigue_weldQuality_NOT_VERIFIED,
    ads_WeldsDefinition_FKM_Fatigue_weldQuality_VERIFIED
};

enum ads_WeldsDefinition_FKM_Fatigue_weldTypeEnm
{
    ads_WeldsDefinition_FKM_Fatigue_weldType_FILLET_WELD,
    ads_WeldsDefinition_FKM_Fatigue_weldType_FULL_PENETRATION,
    ads_WeldsDefinition_FKM_Fatigue_weldType_PARTIAL_PENETRATION,
    ads_WeldsDefinition_FKM_Fatigue_weldType_WELD_WITH_BACK_WELD
};

enum ads_WeldsDefinition_FKM_Fatigue_residualStressEnm
{
    ads_WeldsDefinition_FKM_Fatigue_residualStress_HIGH,
    ads_WeldsDefinition_FKM_Fatigue_residualStress_LOW,
    ads_WeldsDefinition_FKM_Fatigue_residualStress_MODERATE
};

enum ads_WeldsDefinition_FKM_Fatigue_thicknessFactorCaseEnm
{
    ads_WeldsDefinition_FKM_Fatigue_thicknessFactorCase_CASE_A,
    ads_WeldsDefinition_FKM_Fatigue_thicknessFactorCase_CASE_B
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_FKM_StaticMembersEnm
{
    ads_WeldsDefinition_FKM_Static_meshGranularity,
    ads_WeldsDefinition_FKM_Static_seRuleTypeA,
    ads_WeldsDefinition_FKM_Static_seRuleTypeB,
    ads_WeldsDefinition_FKM_Static_seTypeAThickness,
    ads_WeldsDefinition_FKM_Static_seTypeAThicknessType,
    ads_WeldsDefinition_FKM_Static_stressExtrapolationThreshold,
    ads_WeldsDefinition_FKM_Static_stressExtrapolationType,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType,
    ads_WeldsDefinition_FKM_Static_typeACutOffThickness,
    ads_WeldsDefinition_FKM_Static_weldLineSplitAngle,
    ads_WeldsDefinition_FKM_Static_weldLineSplitTolerance,
    ads_WeldsDefinition_FKM_Static_consequencesOfFailure,
    ads_WeldsDefinition_FKM_Static_loadFactor,
    ads_WeldsDefinition_FKM_Static_plasticNotchFactor,
    ads_WeldsDefinition_FKM_Static_sectionFactor,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod,
    ads_WeldsDefinition_FKM_Static_weldFactor,
    ads_WeldsDefinition_FKM_Static_weldQuality,
    ads_WeldsDefinition_FKM_Static_weldType,
    ads_WeldsDefinition_FKM_Static_additionalPartialSafetyFactor,
    ads_WeldsDefinition_FKM_Static_assessmentOfFlowFactor,
    ads_WeldsDefinition_FKM_Static_assessmentOfFlowTemperatureFactor,
    ads_WeldsDefinition_FKM_Static_assessmentOfFractureFactor,
    ads_WeldsDefinition_FKM_Static_assessmentOfFractureTemperatureFactor,
    ads_WeldsDefinition_FKM_Static_elevatedFlowFactor,
    ads_WeldsDefinition_FKM_Static_elevatedFlowTemperatureFactor,
    ads_WeldsDefinition_FKM_Static_elevatedFracturesFactor,
    ads_WeldsDefinition_FKM_Static_elevatedFracturesTemperatureFactor,
    ads_WeldsDefinition_FKM_Static_loadOccurrence,
    ads_WeldsDefinition_FKM_Static_weldPartialSafetyFactor
};

enum ads_WeldsDefinition_FKM_Static_meshGranularityEnm
{
    ads_WeldsDefinition_FKM_Static_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_FKM_Static_meshGranularity_COARSE,
    ads_WeldsDefinition_FKM_Static_meshGranularity_FINE
};

enum ads_WeldsDefinition_FKM_Static_seRuleTypeAEnm
{
    ads_WeldsDefinition_FKM_Static_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_FKM_Static_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_FKM_Static_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_FKM_Static_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_FKM_Static_seRuleTypeBEnm
{
    ads_WeldsDefinition_FKM_Static_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_FKM_Static_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_FKM_Static_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_FKM_Static_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_FKM_Static_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_FKM_Static_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_FKM_Static_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_FKM_Static_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_FKM_Static_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_FKM_Static_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_FKM_Static_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_FKM_Static_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_FKM_Static_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_NONE,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_FKM_Static_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_FKM_Static_consequencesOfFailureEnm
{
    ads_WeldsDefinition_FKM_Static_consequencesOfFailure_HIGH,
    ads_WeldsDefinition_FKM_Static_consequencesOfFailure_MEAN,
    ads_WeldsDefinition_FKM_Static_consequencesOfFailure_MODERATE
};

enum ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethodEnm
{
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_CARBO_NITRIDING,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_CASE_HARDENING,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_COLD_ROLLING,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_INDUCTIVE_HARDENING,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_NITRIDING,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_NONE,
    ads_WeldsDefinition_FKM_Static_surfaceTreatmentMethod_SHOT_PEENING
};

enum ads_WeldsDefinition_FKM_Static_weldQualityEnm
{
    ads_WeldsDefinition_FKM_Static_weldQuality_NOT_VERIFIED,
    ads_WeldsDefinition_FKM_Static_weldQuality_VERIFIED
};

enum ads_WeldsDefinition_FKM_Static_weldTypeEnm
{
    ads_WeldsDefinition_FKM_Static_weldType_FILLET_WELD,
    ads_WeldsDefinition_FKM_Static_weldType_FULL_PENETRATION,
    ads_WeldsDefinition_FKM_Static_weldType_PARTIAL_PENETRATION,
    ads_WeldsDefinition_FKM_Static_weldType_WELD_WITH_BACK_WELD
};

enum ads_WeldsDefinition_FKM_Static_loadOccurrenceEnm
{
    ads_WeldsDefinition_FKM_Static_loadOccurrence_HIGH,
    ads_WeldsDefinition_FKM_Static_loadOccurrence_LOW
};

/** 
Enum with record members. */
enum ads_WeldsDefinition_StandardMembersEnm
{
    ads_WeldsDefinition_Standard_meshGranularity,
    ads_WeldsDefinition_Standard_seRuleTypeA,
    ads_WeldsDefinition_Standard_seRuleTypeB,
    ads_WeldsDefinition_Standard_seTypeAThickness,
    ads_WeldsDefinition_Standard_seTypeAThicknessType,
    ads_WeldsDefinition_Standard_stressExtrapolationThreshold,
    ads_WeldsDefinition_Standard_stressExtrapolationType,
    ads_WeldsDefinition_Standard_structuralStressSourceType,
    ads_WeldsDefinition_Standard_typeACutOffThickness,
    ads_WeldsDefinition_Standard_weldLineSplitAngle,
    ads_WeldsDefinition_Standard_weldLineSplitTolerance,
    ads_WeldsDefinition_Standard_IR_FunctionType,
    ads_WeldsDefinition_Standard_bluntRootThreshold,
    ads_WeldsDefinition_Standard_failurePlaneTolerance,
    ads_WeldsDefinition_Standard_meanStressCorrectionType,
    ads_WeldsDefinition_Standard_multiaxialModWangBrown,
    ads_WeldsDefinition_Standard_multiaxialNonLinearMapping,
    ads_WeldsDefinition_Standard_multiaxialRatioThreshold,
    ads_WeldsDefinition_Standard_plateThickness,
    ads_WeldsDefinition_Standard_plateThicknessType,
    ads_WeldsDefinition_Standard_smoothingIterations,
    ads_WeldsDefinition_Standard_structuralStressDamageParameter
};

enum ads_WeldsDefinition_Standard_meshGranularityEnm
{
    ads_WeldsDefinition_Standard_meshGranularity_AUTO_DETECT,
    ads_WeldsDefinition_Standard_meshGranularity_COARSE,
    ads_WeldsDefinition_Standard_meshGranularity_FINE
};

enum ads_WeldsDefinition_Standard_seRuleTypeAEnm
{
    ads_WeldsDefinition_Standard_seRuleTypeA_AUTO_SELECT,
    ads_WeldsDefinition_Standard_seRuleTypeA_RULE_4_10T,
    ads_WeldsDefinition_Standard_seRuleTypeA_RULE_4_9_14T,
    ads_WeldsDefinition_Standard_seRuleTypeA_RULE_5_15T
};

enum ads_WeldsDefinition_Standard_seRuleTypeBEnm
{
    ads_WeldsDefinition_Standard_seRuleTypeB_AUTO_SELECT,
    ads_WeldsDefinition_Standard_seRuleTypeB_RULE_4_8_12MM,
    ads_WeldsDefinition_Standard_seRuleTypeB_RULE_5_15MM
};

enum ads_WeldsDefinition_Standard_seTypeAThicknessTypeEnm
{
    ads_WeldsDefinition_Standard_seTypeAThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_Standard_seTypeAThicknessType_SPECIFY
};

enum ads_WeldsDefinition_Standard_stressExtrapolationTypeEnm
{
    ads_WeldsDefinition_Standard_stressExtrapolationType_AUTO_DETECT,
    ads_WeldsDefinition_Standard_stressExtrapolationType_SPECIFY_RULE_TYPEA,
    ads_WeldsDefinition_Standard_stressExtrapolationType_SPECIFY_RULE_TYPEB,
    ads_WeldsDefinition_Standard_stressExtrapolationType_TYPEA,
    ads_WeldsDefinition_Standard_stressExtrapolationType_TYPEB
};

enum ads_WeldsDefinition_Standard_structuralStressSourceTypeEnm
{
    ads_WeldsDefinition_Standard_structuralStressSourceType_AUTO_SELECT,
    ads_WeldsDefinition_Standard_structuralStressSourceType_BATTELLE_BASIC_NODAL_FORCE,
    ads_WeldsDefinition_Standard_structuralStressSourceType_ENHANCED_NODAL_FORCE,
    ads_WeldsDefinition_Standard_structuralStressSourceType_NONE,
    ads_WeldsDefinition_Standard_structuralStressSourceType_STRESS_EXTRAPOLATION,
    ads_WeldsDefinition_Standard_structuralStressSourceType_STRESS_LINEARIZATION,
    ads_WeldsDefinition_Standard_structuralStressSourceType_VOLVO_NODAL_FORCE
};

enum ads_WeldsDefinition_Standard_IR_FunctionTypeEnm
{
    ads_WeldsDefinition_Standard_IR_FunctionType_DISPLACEMENT_CONTROL,
    ads_WeldsDefinition_Standard_IR_FunctionType_LOAD_CONTROL
};

enum ads_WeldsDefinition_Standard_meanStressCorrectionTypeEnm
{
    ads_WeldsDefinition_Standard_meanStressCorrectionType_ASME_TENSILE,
    ads_WeldsDefinition_Standard_meanStressCorrectionType_BATTELLE_COMPRESSIVE,
    ads_WeldsDefinition_Standard_meanStressCorrectionType_BATTELLE_LOW,
    ads_WeldsDefinition_Standard_meanStressCorrectionType_MODIFIED_GOODMAN,
    ads_WeldsDefinition_Standard_meanStressCorrectionType_NONE,
    ads_WeldsDefinition_Standard_meanStressCorrectionType_USER_FILE
};

enum ads_WeldsDefinition_Standard_plateThicknessTypeEnm
{
    ads_WeldsDefinition_Standard_plateThicknessType_AUTO_DETECT,
    ads_WeldsDefinition_Standard_plateThicknessType_SPECIFY_FIXED,
    ads_WeldsDefinition_Standard_plateThicknessType_SPECIFY_MAXIMUM
};

enum ads_WeldsDefinition_Standard_structuralStressDamageParameterEnm
{
    ads_WeldsDefinition_Standard_structuralStressDamageParameter_MULTIAXIAL_BLENDED_NORMAL_SHEAR,
    ads_WeldsDefinition_Standard_structuralStressDamageParameter_NORMAL,
    ads_WeldsDefinition_Standard_structuralStressDamageParameter_SHEAR,
    ads_WeldsDefinition_Standard_structuralStressDamageParameter_WORST_NORMAL_SHEAR
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_Standard_fusionFailureModeRolesEnm
{
    ads_WeldsDefinition_Standard_fusionFailureMode_child,
    ads_WeldsDefinition_Standard_fusionFailureMode_parent
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_Standard_rootFailureModeRolesEnm
{
    ads_WeldsDefinition_Standard_rootFailureMode_child,
    ads_WeldsDefinition_Standard_rootFailureMode_parent
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_Standard_throatFailureModeRolesEnm
{
    ads_WeldsDefinition_Standard_throatFailureMode_child,
    ads_WeldsDefinition_Standard_throatFailureMode_parent
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_Standard_toeFailureModeRolesEnm
{
    ads_WeldsDefinition_Standard_toeFailureMode_child,
    ads_WeldsDefinition_Standard_toeFailureMode_parent
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_Standard_userMeanStressCorrectionFTableRolesEnm
{
    ads_WeldsDefinition_Standard_userMeanStressCorrectionFTable_referent,
    ads_WeldsDefinition_Standard_userMeanStressCorrectionFTable_referrer
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_Standard_weldEndCorrectionRolesEnm
{
    ads_WeldsDefinition_Standard_weldEndCorrection_child,
    ads_WeldsDefinition_Standard_weldEndCorrection_parent
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_lineSelectionRolesEnm
{
    ads_WeldsDefinition_lineSelection_referent,
    ads_WeldsDefinition_lineSelection_referrer
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_surfaceSelectionRolesEnm
{
    ads_WeldsDefinition_surfaceSelection_referent,
    ads_WeldsDefinition_surfaceSelection_referrer
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_surfaceStressExtrapolationRolesEnm
{
    ads_WeldsDefinition_surfaceStressExtrapolation_child,
    ads_WeldsDefinition_surfaceStressExtrapolation_parent
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_weldBodyRolesEnm
{
    ads_WeldsDefinition_weldBody_referent,
    ads_WeldsDefinition_weldBody_referrer
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_weldBodyExclusionRolesEnm
{
    ads_WeldsDefinition_weldBodyExclusion_referent,
    ads_WeldsDefinition_weldBodyExclusion_referrer
};

/** 
Enum with association roles. */
enum ads_WeldsDefinition_weldSubRegionsRolesEnm
{
    ads_WeldsDefinition_weldSubRegions_child,
    ads_WeldsDefinition_weldSubRegions_parent
};

#endif
