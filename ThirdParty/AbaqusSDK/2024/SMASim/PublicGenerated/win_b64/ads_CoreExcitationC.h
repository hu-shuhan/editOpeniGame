//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreExcitationC_h
#define ads_CoreExcitationC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Excitation of the latest level of form Core */

/** This option specify that the boundary conditions are the 'driven variables' in a submodel analysis.. */
#define ads_BoundarySubmodel (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 0))

#define ads_BoundarySubmodel_stepInc (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 1))

/** The order is important only in case of predefined fields where last one wins rule applies in case of overlaping nodes/elements. */
#define ads_Case_excitations (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 2))

/** This option is used to define the cross-correlation as part of the definition of random loading */
#define ads_Correlation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 3))

#define ads_Correlation_excitation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 4))

#define ads_Correlation_psd (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 5))

/** A class representing an excitation which can be a load, a boundary condition, an initial condition or a predefined field. Each Excitation has an association to the Field which define the excitation values. The excitation also has an association to FieldType. This FieldType need not be same as the field type of the Field the Excitation is associated to (even though it usually is). For example; the Field may be a stress Field but it may be used in Excitation as a traction FieldType. So, it is best to rely on the FieldType the excitation is associated to rather than relying on the FieldType of the Field. The Excitation is also associated to the region using the ExcitationRegion association. The field can be space-only, or space x time. The field may be modulated; a modulated field is represented as a field of type FieldModulator which itself refers to the field to be modulated and a Curve which does the modulating. The field may also be time-shifted through the use of a FieldShifter. For boundary conditions and loads applied to a step (as opposed to a non-step task) the field must be space x time, and time is step time. Excitations apply to Tasks of various kinds. For these purposes there are Tasks which do not use cases which we will call them Case-free tasks. For example; Step, FrequencyTask, and ComplexFrequencyTask. The other Tasks classes use Cases and are called Case-using Tasks. The connection between a Case-using task and an excitation is as follows: the case-using Task is connected to one or more Cases; Case in turn is connected to the Excitation through the HolderExcitation association. The Case-free Tasks on the other hand are associated directly to excitations through the HolderExcitation assocition. */
#define ads_Excitation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 6))

#define ads_ExcitationRelay (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 7))

/** This excitation relay should be used for fixing the inertia relief to be at the end of the previous step. */
#define ads_ExcitationRelay_InertiaRelief_Fixed (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 8))

#define ads_ExcitationRelay_amplitude (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 9))

#define ads_ExcitationRelay_excitation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 10))

/** This is temporary, will be deleted once all clients start using RelayModulator. */
#define ads_ExcitationRelay_intervalStepInc (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 11))

/** Phase angle should be specified in radians. */
#define ads_ExcitationRelay_phaseAngle (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 12))

#define ads_ExcitationRelay_relayModulations (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 13))

#define ads_Excitation_BC (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 14))

#define ads_Excitation_BC_BaseMotion (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 15))

#define ads_Excitation_BC_BaseMotion_globalDOF (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 16))

#define ads_Excitation_BC_BaseMotion_pointOfRotation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 17))

#define ads_Excitation_BC_BaseMotion_secondaryBase (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 18))

#define ads_Excitation_BC_CFD (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 19))

#define ads_Excitation_BC_CFD_Density (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 20))

#define ads_Excitation_BC_CFD_Displacement (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 21))

#define ads_Excitation_BC_CFD_Enthalpy (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 22))

#define ads_Excitation_BC_CFD_Fan (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 23))

#define ads_Excitation_BC_CFD_Freestream (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 24))

#define ads_Excitation_BC_CFD_HeatFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 25))

/** Human comfort */
#define ads_Excitation_BC_CFD_Human (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 26))

#define ads_Excitation_BC_CFD_InternalEnergy (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 27))

/** Mass Flow Inlet */
#define ads_Excitation_BC_CFD_MassFlowInlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 28))

/** Mass Flow Split */
#define ads_Excitation_BC_CFD_MassFlowSplit (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 29))

#define ads_Excitation_BC_CFD_MassFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 30))

#define ads_Excitation_BC_CFD_NonreflectingOutlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 31))

#define ads_Excitation_BC_CFD_NormalDisplacement (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 32))

#define ads_Excitation_BC_CFD_NormalHeatFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 33))

#define ads_Excitation_BC_CFD_NormalMassFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 34))

#define ads_Excitation_BC_CFD_NormalVelocity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 35))

#define ads_Excitation_BC_CFD_NormalWallDistance (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 36))

#define ads_Excitation_BC_CFD_ParticleInjector (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 37))

#define ads_Excitation_BC_CFD_PassiveOutflow (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 38))

#define ads_Excitation_BC_CFD_Pressure (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 39))

#define ads_Excitation_BC_CFD_PressureOutflow (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 40))

/** Pressure outlet. */
#define ads_Excitation_BC_CFD_PressureOutlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 41))

#define ads_Excitation_BC_CFD_PressureVolumeDependent (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 42))

/** Stagnation inlet. */
#define ads_Excitation_BC_CFD_StagnationInlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 43))

/** Symmetric BC condition. */
#define ads_Excitation_BC_CFD_Symmetric (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 44))

#define ads_Excitation_BC_CFD_Temperature (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 45))

#define ads_Excitation_BC_CFD_Traction (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 46))

#define ads_Excitation_BC_CFD_TurbulentDissipation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 47))

#define ads_Excitation_BC_CFD_TurbulentIntensity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 48))

#define ads_Excitation_BC_CFD_TurbulentKineticEnergy (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 49))

#define ads_Excitation_BC_CFD_TurbulentLengthScale (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 50))

#define ads_Excitation_BC_CFD_TurbulentProduction (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 51))

#define ads_Excitation_BC_CFD_TurbulentTimeScale (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 52))

#define ads_Excitation_BC_CFD_TurbulentVelocityRatio (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 53))

#define ads_Excitation_BC_CFD_TurbulentVelocityScale (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 54))

#define ads_Excitation_BC_CFD_TurbulentViscosity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 55))

#define ads_Excitation_BC_CFD_TurbulentViscosityRatio (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 56))

#define ads_Excitation_BC_CFD_Velocity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 57))

/** Velocity inlet. */
#define ads_Excitation_BC_CFD_VelocityInlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 58))

#define ads_Excitation_BC_CFD_Velocity_dofType (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 59))

/** Wall. */
#define ads_Excitation_BC_CFD_Wall (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 60))

/** Rotating Wall BC. */
#define ads_Excitation_BC_CFD_Wall_Rotating (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 61))

#define ads_Excitation_BC_CFD_WaveInlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 62))

#define ads_Excitation_BC_ConnectorMotion (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 63))

#define ads_Excitation_BC_ElectricPotential (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 64))

#define ads_Excitation_BC_FIXED (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 65))

#define ads_Excitation_BC_FIXED_dofTypes (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 66))

#define ads_Excitation_BC_FluidElectricPotential (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 67))

#define ads_Excitation_BC_IonConcentration (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 68))

#define ads_Excitation_BC_Pressure (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 69))

#define ads_Excitation_BC_RestraintSymbolic (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 70))

#define ads_Excitation_BC_Temperature (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 71))

#define ads_Excitation_BC_Temperature_temperaturePoints (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 72))

#define ads_Excitation_BC_TransportVelocity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 73))

#define ads_Excitation_BC_UVA (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 74))

#define ads_Excitation_BC_UVA_dofType (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 75))

#define ads_Excitation_BC_submodel (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 76))

#define ads_Excitation_CFD (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 77))

#define ads_Excitation_CFD_ECooling (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 78))

#define ads_Excitation_CFD_ECooling_Blower (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 79))

#define ads_Excitation_CFD_ECooling_Blower_table (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 80))

#define ads_Excitation_CFD_ECooling_CHeatSink (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 81))

#define ads_Excitation_CFD_ECooling_CHeatSink_Manual (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 82))

#define ads_Excitation_CFD_ECooling_CHeatSink_support (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 83))

#define ads_Excitation_CFD_ECooling_CompactPCB (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 84))

#define ads_Excitation_CFD_ECooling_CompactPCB_support (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 85))

#define ads_Excitation_CFD_ECooling_PerforatedPlate (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 86))

#define ads_Excitation_CFD_ECooling_PerforatedPlate_support (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 87))

#define ads_Excitation_CFD_ECooling_PerforatedPlate_table (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 88))

/** Thermoelectric Cooler */
#define ads_Excitation_CFD_ECooling_ThermoelectricCooler (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 89))

#define ads_Excitation_CFD_ECooling_TwoResistor (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 90))

#define ads_Excitation_CFD_ECooling_TwoResistor_support (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 91))

#define ads_Excitation_CFD_ECooling_section (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 92))

#define ads_Excitation_ExternalField (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 93))

#define ads_Excitation_Load (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 94))

/** Line load for beam elements */
#define ads_Excitation_Load_BeamLine (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 95))

/** Set C_1,C_2,C_3 for force in global directions PX,PY,PZ respectilvely. Set C_1,C_2 for force in local directions P1,P2 respectively. */
#define ads_Excitation_Load_BeamLine_component (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 96))

#define ads_Excitation_Load_BodyFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 97))

#define ads_Excitation_Load_BodyForce (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 98))

#define ads_Excitation_Load_BodyForce_component (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 99))

#define ads_Excitation_Load_ConcentratedElectricCharge (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 100))

#define ads_Excitation_Load_ConcentratedElectricCurrent (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 101))

#define ads_Excitation_Load_ConcentratedFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 102))

#define ads_Excitation_Load_ConcentratedFlux_PhantomEdge (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 103))

#define ads_Excitation_Load_ConcentratedFlux_PhantomEdge_node2 (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 104))

#define ads_Excitation_Load_ConcentratedFlux_PhantomNode (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 105))

#define ads_Excitation_Load_ConcentratedFlux_temperaturePoints (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 106))

#define ads_Excitation_Load_ConcentratedForce (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 107))

#define ads_Excitation_Load_ConcentratedForce_dofType (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 108))

#define ads_Excitation_Load_ConcentratedMoment (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 109))

#define ads_Excitation_Load_ConcentratedMoment_dofType (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 110))

#define ads_Excitation_Load_ConnectorForce (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 111))

#define ads_Excitation_Load_ConnectorMoment (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 112))

/** Cooling Channel Inlet loading for the cooling step. */
#define ads_Excitation_Load_CoolingChannelInlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 113))

/** Cooling Channel Outlet loading for the cooling step. */
#define ads_Excitation_Load_CoolingChannelOutlet (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 114))

#define ads_Excitation_Load_ElectricBodyCharge (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 115))

#define ads_Excitation_Load_ElectricBodyCurrent (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 116))

#define ads_Excitation_Load_ElectricSurfaceCharge (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 117))

#define ads_Excitation_Load_ElectricSurfaceCurrent (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 118))

#define ads_Excitation_Load_ElectricSurfaceCurrent_currOrientation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 119))

#define ads_Excitation_Load_ElectricSurfaceCurrent_direction (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 120))

#define ads_Excitation_Load_ElectrolyteConcentratedElectricCurrent (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 121))

#define ads_Excitation_Load_ElectrolyteElectricSurfaceCurrent (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 122))

/** To define fluid pressure penetration surface pressures */
#define ads_Excitation_Load_FluidPressurePenetration (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 123))

/** Wetting Advance algorithm. */
#define ads_Excitation_Load_FluidPressurePenetration_WettingAdvance (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 124))

/** Node or node set on the surface initially exposed to fluid pressure. */
#define ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_exposedNodes (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 125))

/** Amplitude curve that defines the variation of the critical contact pressure threshold during the step. */
#define ads_Excitation_Load_FluidPressurePenetration_contactPressureCurve (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 126))

#define ads_Excitation_Load_Gravity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 127))

/** Gravity Vector. */
#define ads_Excitation_Load_Gravity_gravityVector (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 128))

#define ads_Excitation_Load_HeatFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 129))

#define ads_Excitation_Load_HeatFlux_faceId (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 130))

#define ads_Excitation_Load_HydroStaticPressure (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 131))

#define ads_Excitation_Load_InertiaRelief (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 132))

#define ads_Excitation_Load_IonBodyFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 133))

#define ads_Excitation_Load_IonConcentratedFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 134))

#define ads_Excitation_Load_IonSurfaceFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 135))

#define ads_Excitation_Load_MovingBodyFlux (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 136))

#define ads_Excitation_Load_MovingBodyFlux_tableContainer (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 137))

#define ads_Excitation_Load_Multiphysics (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 138))

#define ads_Excitation_Load_Multiphysics_ButlerVolmer (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 139))

#define ads_Excitation_Load_Multiphysics_faceId (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 140))

#define ads_Excitation_Load_Multiphysics_tableContainer (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 141))

#define ads_Excitation_Load_OhmicLoss (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 142))

#define ads_Excitation_Load_Porosity (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 143))

#define ads_Excitation_Load_PorousZone (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 144))

#define ads_Excitation_Load_PorousZone_PVCurve (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 145))

#define ads_Excitation_Load_PorousZone_PVCurve_table (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 146))

#define ads_Excitation_Load_Pressure (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 147))

#define ads_Excitation_Load_RRF (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 148))

#define ads_Excitation_Load_RRF_Centrifugal (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 149))

#define ads_Excitation_Load_RRF_CentrifugalRhoOmega2 (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 150))

#define ads_Excitation_Load_RRF_Coriolis (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 151))

#define ads_Excitation_Load_RRF_RotaryAcceleration (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 152))

/** Direction of the axis of rotation. */
#define ads_Excitation_Load_RRF_axisDirection (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 153))

/** Point on the axis of rotation. */
#define ads_Excitation_Load_RRF_axisPoint (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 154))

#define ads_Excitation_Load_ShellEdge (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 155))

#define ads_Excitation_Load_ShellEdge_tractionVectorDirection (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 156))

#define ads_Excitation_Load_Traction (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 157))

/** Traction Vector Direction. */
#define ads_Excitation_Load_Traction_tractionVectorDirection (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 158))

/** Multi-Species Volumetric Source. */
#define ads_Excitation_Load_VolumetricSource (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 159))

#define ads_Excitation_Load_Zone (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 160))

#define ads_Excitation_Load_Zone_MRF (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 161))

#define ads_Excitation_Load_Zone_Sedimentation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 162))

#define ads_Excitation_Load_Zone_Sedimentation_sedimentScaling (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 163))

#define ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 164))

#define ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 165))

#define ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 166))

#define ads_Excitation_Load_scaleFactors (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 167))

#define ads_Excitation_PF (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 168))

/** Datatype to define the motion of a reference frame in steady-state transport analysis, or to define the velocity of the material transported through the mesh during a static analysis. It can also be used to specify the velocity of an element set representing a conductor transported through the mesh in an eddy current analysis. */
#define ads_Excitation_PF_Motion (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 169))

#define ads_Excitation_PF_Motion_dofType (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 170))

/** Ordered CSet of nodes to capture the points of rotation. */
#define ads_Excitation_PF_Motion_rotationAxisNodes (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 171))

#define ads_Excitation_PF_Motion_rotationAxisPointA (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 172))

#define ads_Excitation_PF_Motion_rotationAxisPointB (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 173))

#define ads_Excitation_PF_Temperature (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 174))

#define ads_Excitation_PF_UserField (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 175))

/** First field captures the gradient in the n1 direction for beams or gradient through the thickness for shells. Second field captures the gradient in the n2 direction for beams. */
#define ads_Excitation_PF_UserField_gradientFields (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 176))

#define ads_Excitation_Rve (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 177))

#define ads_Excitation_Rve_FarFieldCondition (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 178))

#define ads_Excitation_Rve_FarFieldLoad (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 179))

#define ads_Excitation_Rve_rve (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 180))

#define ads_Excitation_Set (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 181))

#define ads_Excitation_Set_loadSetExcitations (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 182))

#define ads_Excitation_field (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 183))

#define ads_Excitation_fields (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 184))

#define ads_Excitation_gradient (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 185))

#define ads_Excitation_initWettedRegion (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 186))

#define ads_Excitation_orientation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 187))

#define ads_Excitation_region (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 188))

/** Abstract datatype to capture the fatigue loading block. */
#define ads_FatigueLoadingBlockV1 (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 189))

/** Datatype to capture the fatigue loading through a history file in case of a superimposition load application. */
#define ads_FatigueLoadingBlockV1_HistoryFile (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 190))

/** Datatype to capture the fatigue loading through multiplier in case of a superimposition load application. */
#define ads_FatigueLoadingBlockV1_Multiplier (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 191))

/** Datatype to capture the fatigue loading in case of a sequence load application. */
#define ads_FatigueLoadingBlockV1_Sequence (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 192))

#define ads_Interaction_CFD_section (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 193))

#define ads_Model_excitations (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 194))

/** Composition from Model to the FatigueLoadingBlockV1. In reality there could be several blocks of loading per model. However, for the first version we would like to capture only one block of loading per model and the roleName reflects the fact that this has potential to change in the near future. */
#define ads_Model_fatigueLoadingBlockV1 (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 195))

#define ads_Model_psd (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 196))

/** This option is used to define a frequency function for reference in the *CORRELATION option to define the frequency dependence of the random loading */
#define ads_PSD (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 197))

#define ads_PSD_table (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 198))

#define ads_PSD_unitsRecords (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 199))

/** Data to define pressure velocity table */
#define ads_PerfPlate_ExiPressureVelocityTable (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 200))

/** Data to define pressure velocity table */
#define ads_PorousPressureVelocityTable (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 201))

/** Modulates individual fields in a excitation. */
#define ads_RelayModulator (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 202))

#define ads_RelayModulator_amplitude (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 203))

#define ads_RelayModulator_field (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 204))

/** Link to attach IntervalStepInc to a RelayModulator. This data will supersede that attached directly to the Field_ExternalSourceAbq to which the RelayModulator refers. */
#define ads_RelayModulator_intervalStepInc (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 205))

#define ads_SecondaryBase (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 206))

#define ads_SecondaryBase_excitationBCs (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 207))

/** This option selects the modes to be used in a dynamic analysis based on modes, in a complex eigenvalue extraction analysis, or in a substructure generation analysis. */
#define ads_SelectEigenModes (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 208))

/** Table to define the frequency range */
#define ads_SelectEigenModesTable (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 209))

/** The selected list of modes when the definition is MODE_NUMBERS */
#define ads_SelectEigenModes_modes (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 210))

#define ads_SelectEigenModes_table (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 211))

#define ads_Step_Lin_ModalRandomResponse_correlation (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 212))

/** The order is important only in case of predefined fields where last one wins rule applies in case of overlaping nodes/elements. */
#define ads_Task_excitations (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 213))

/** The task to the selected eigen modes relation */
#define ads_Task_selectEigenModes (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 214))

#define ads_ValveGate (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 215))

/** Data to define fan curve table */
#define ads_fanCurveTable (ads_CoreFragmentTypeIndex(ads_CoreExcitationFragment, 216))

/** Enum with association roles. */
enum ads_BoundarySubmodel_stepIncRolesEnm
{
    ads_BoundarySubmodel_stepInc_child,
    ads_BoundarySubmodel_stepInc_parent
};

/** 
Enum with association roles. */
enum ads_Case_excitationsRolesEnm
{
    ads_Case_excitations_child,
    ads_Case_excitations_parent
};

/** 
Enum with record members. */
enum ads_CorrelationMembersEnm
{
    ads_Correlation_imaginaryScalingFactor,
    ads_Correlation_isComplex,
    ads_Correlation_realScalingFactor,
    ads_Correlation_type
};

enum ads_Correlation_typeEnm
{
    ads_Correlation_type_CORRELATED,
    ads_Correlation_type_MOVING_NOISE,
    ads_Correlation_type_UNCORRELATED
};

/** Enum with association roles. */
enum ads_Correlation_excitationRolesEnm
{
    ads_Correlation_excitation_referent,
    ads_Correlation_excitation_referrer
};

/** Enum with association roles. */
enum ads_Correlation_psdRolesEnm
{
    ads_Correlation_psd_referent,
    ads_Correlation_psd_referrer
};

/** 
Enum with record members. */
enum ads_ExcitationMembersEnm
{
    ads_Excitation_user
};

/** Enum with record members. */
enum ads_ExcitationRelayMembersEnm
{
    ads_ExcitationRelay_autoPropogated,
    ads_ExcitationRelay_scaleFactor
};

/** 
Enum with record members. */
enum ads_ExcitationRelay_InertiaRelief_FixedMembersEnm
{
    ads_ExcitationRelay_InertiaRelief_Fixed_autoPropogated,
    ads_ExcitationRelay_InertiaRelief_Fixed_scaleFactor
};

/** Enum with association roles. */
enum ads_ExcitationRelay_amplitudeRolesEnm
{
    ads_ExcitationRelay_amplitude_referent,
    ads_ExcitationRelay_amplitude_referrer
};

/** Enum with association roles. */
enum ads_ExcitationRelay_excitationRolesEnm
{
    ads_ExcitationRelay_excitation_referent,
    ads_ExcitationRelay_excitation_referrer
};

/** 
Enum with association roles. */
enum ads_ExcitationRelay_intervalStepIncRolesEnm
{
    ads_ExcitationRelay_intervalStepInc_child,
    ads_ExcitationRelay_intervalStepInc_parent
};

/** 
Enum with association roles. */
enum ads_ExcitationRelay_phaseAngleRolesEnm
{
    ads_ExcitationRelay_phaseAngle_child,
    ads_ExcitationRelay_phaseAngle_parent
};

/** Enum with association roles. */
enum ads_ExcitationRelay_relayModulationsRolesEnm
{
    ads_ExcitationRelay_relayModulations_child,
    ads_ExcitationRelay_relayModulations_parent
};

/** Enum with record members. */
enum ads_Excitation_BCMembersEnm
{
    ads_Excitation_BC_user
};

/** Enum with record members. */
enum ads_Excitation_BC_BaseMotionMembersEnm
{
    ads_Excitation_BC_BaseMotion_user
};

/** Enum with association roles. */
enum ads_Excitation_BC_BaseMotion_globalDOFRolesEnm
{
    ads_Excitation_BC_BaseMotion_globalDOF_referent,
    ads_Excitation_BC_BaseMotion_globalDOF_referrer
};

/** Enum with association roles. */
enum ads_Excitation_BC_BaseMotion_pointOfRotationRolesEnm
{
    ads_Excitation_BC_BaseMotion_pointOfRotation_child,
    ads_Excitation_BC_BaseMotion_pointOfRotation_parent
};

/** Enum with association roles. */
enum ads_Excitation_BC_BaseMotion_secondaryBaseRolesEnm
{
    ads_Excitation_BC_BaseMotion_secondaryBase_referent,
    ads_Excitation_BC_BaseMotion_secondaryBase_referrer
};

/** Enum with record members. */
enum ads_Excitation_BC_CFDMembersEnm
{
    ads_Excitation_BC_CFD_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_DensityMembersEnm
{
    ads_Excitation_BC_CFD_Density_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_DisplacementMembersEnm
{
    ads_Excitation_BC_CFD_Displacement_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_EnthalpyMembersEnm
{
    ads_Excitation_BC_CFD_Enthalpy_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_FanMembersEnm
{
    ads_Excitation_BC_CFD_Fan_user,
    ads_Excitation_BC_CFD_Fan_type
};

enum ads_Excitation_BC_CFD_Fan_typeEnm
{
    ads_Excitation_BC_CFD_Fan_type_INLET,
    ads_Excitation_BC_CFD_Fan_type_OUTLET
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_FreestreamMembersEnm
{
    ads_Excitation_BC_CFD_Freestream_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_HeatFluxMembersEnm
{
    ads_Excitation_BC_CFD_HeatFlux_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_HumanMembersEnm
{
    ads_Excitation_BC_CFD_Human_user,
    ads_Excitation_BC_CFD_Human_clothingInsulation,
    ads_Excitation_BC_CFD_Human_mets
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_InternalEnergyMembersEnm
{
    ads_Excitation_BC_CFD_InternalEnergy_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_MassFlowInletMembersEnm
{
    ads_Excitation_BC_CFD_MassFlowInlet_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_MassFlowSplitMembersEnm
{
    ads_Excitation_BC_CFD_MassFlowSplit_user,
    ads_Excitation_BC_CFD_MassFlowSplit_weightFactor
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_MassFluxMembersEnm
{
    ads_Excitation_BC_CFD_MassFlux_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_NonreflectingOutletMembersEnm
{
    ads_Excitation_BC_CFD_NonreflectingOutlet_user,
    ads_Excitation_BC_CFD_NonreflectingOutlet_type
};

enum ads_Excitation_BC_CFD_NonreflectingOutlet_typeEnm
{
    ads_Excitation_BC_CFD_NonreflectingOutlet_type_CHARACTERISTIC,
    ads_Excitation_BC_CFD_NonreflectingOutlet_type_NONE,
    ads_Excitation_BC_CFD_NonreflectingOutlet_type_SPONGE
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_NormalDisplacementMembersEnm
{
    ads_Excitation_BC_CFD_NormalDisplacement_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_NormalHeatFluxMembersEnm
{
    ads_Excitation_BC_CFD_NormalHeatFlux_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_NormalMassFluxMembersEnm
{
    ads_Excitation_BC_CFD_NormalMassFlux_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_NormalVelocityMembersEnm
{
    ads_Excitation_BC_CFD_NormalVelocity_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_NormalWallDistanceMembersEnm
{
    ads_Excitation_BC_CFD_NormalWallDistance_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_ParticleInjectorMembersEnm
{
    ads_Excitation_BC_CFD_ParticleInjector_user,
    ads_Excitation_BC_CFD_ParticleInjector_massType
};

enum ads_Excitation_BC_CFD_ParticleInjector_massTypeEnm
{
    ads_Excitation_BC_CFD_ParticleInjector_massType_MASS,
    ads_Excitation_BC_CFD_ParticleInjector_massType_MASSLESS
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_PassiveOutflowMembersEnm
{
    ads_Excitation_BC_CFD_PassiveOutflow_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_PressureMembersEnm
{
    ads_Excitation_BC_CFD_Pressure_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_PressureOutflowMembersEnm
{
    ads_Excitation_BC_CFD_PressureOutflow_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_PressureOutletMembersEnm
{
    ads_Excitation_BC_CFD_PressureOutlet_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_PressureVolumeDependentMembersEnm
{
    ads_Excitation_BC_CFD_PressureVolumeDependent_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_StagnationInletMembersEnm
{
    ads_Excitation_BC_CFD_StagnationInlet_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_SymmetricMembersEnm
{
    ads_Excitation_BC_CFD_Symmetric_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TemperatureMembersEnm
{
    ads_Excitation_BC_CFD_Temperature_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TractionMembersEnm
{
    ads_Excitation_BC_CFD_Traction_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentDissipationMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentDissipation_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentIntensityMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentIntensity_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentKineticEnergyMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentKineticEnergy_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentLengthScaleMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentLengthScale_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentProductionMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentProduction_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentTimeScaleMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentTimeScale_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentVelocityRatioMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentVelocityRatio_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentVelocityScaleMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentVelocityScale_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentViscosityMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentViscosity_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_TurbulentViscosityRatioMembersEnm
{
    ads_Excitation_BC_CFD_TurbulentViscosityRatio_user
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_VelocityMembersEnm
{
    ads_Excitation_BC_CFD_Velocity_user
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_VelocityInletMembersEnm
{
    ads_Excitation_BC_CFD_VelocityInlet_user
};

/** Enum with association roles. */
enum ads_Excitation_BC_CFD_Velocity_dofTypeRolesEnm
{
    ads_Excitation_BC_CFD_Velocity_dofType_referent,
    ads_Excitation_BC_CFD_Velocity_dofType_referrer
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_WallMembersEnm
{
    ads_Excitation_BC_CFD_Wall_user,
    ads_Excitation_BC_CFD_Wall_autoComputeVelocity,
    ads_Excitation_BC_CFD_Wall_fluidFilmPhaseChange,
    ads_Excitation_BC_CFD_Wall_isMRF,
    ads_Excitation_BC_CFD_Wall_wallType
};

enum ads_Excitation_BC_CFD_Wall_wallTypeEnm
{
    ads_Excitation_BC_CFD_Wall_wallType_MOVING_WALL,
    ads_Excitation_BC_CFD_Wall_wallType_NO_SLIP_WALL,
    ads_Excitation_BC_CFD_Wall_wallType_SLIP_WALL,
    ads_Excitation_BC_CFD_Wall_wallType_TANGENTIAL_WALL
};

/** 
Enum with record members. */
enum ads_Excitation_BC_CFD_Wall_RotatingMembersEnm
{
    ads_Excitation_BC_CFD_Wall_Rotating_user,
    ads_Excitation_BC_CFD_Wall_Rotating_autoComputeVelocity,
    ads_Excitation_BC_CFD_Wall_Rotating_fluidFilmPhaseChange,
    ads_Excitation_BC_CFD_Wall_Rotating_isMRF,
    ads_Excitation_BC_CFD_Wall_Rotating_wallType,
    ads_Excitation_BC_CFD_Wall_Rotating_rotationAxis
};

enum ads_Excitation_BC_CFD_Wall_Rotating_wallTypeEnm
{
    ads_Excitation_BC_CFD_Wall_Rotating_wallType_MOVING_WALL,
    ads_Excitation_BC_CFD_Wall_Rotating_wallType_NO_SLIP_WALL,
    ads_Excitation_BC_CFD_Wall_Rotating_wallType_SLIP_WALL,
    ads_Excitation_BC_CFD_Wall_Rotating_wallType_TANGENTIAL_WALL
};

enum ads_Excitation_BC_CFD_Wall_Rotating_rotationAxisEnm
{
    ads_Excitation_BC_CFD_Wall_Rotating_rotationAxis_LOCAL_1,
    ads_Excitation_BC_CFD_Wall_Rotating_rotationAxis_LOCAL_2,
    ads_Excitation_BC_CFD_Wall_Rotating_rotationAxis_LOCAL_3
};

/** Enum with record members. */
enum ads_Excitation_BC_CFD_WaveInletMembersEnm
{
    ads_Excitation_BC_CFD_WaveInlet_user,
    ads_Excitation_BC_CFD_WaveInlet_waveHeight,
    ads_Excitation_BC_CFD_WaveInlet_waveOrder
};

/** Enum with record members. */
enum ads_Excitation_BC_ConnectorMotionMembersEnm
{
    ads_Excitation_BC_ConnectorMotion_user,
    ads_Excitation_BC_ConnectorMotion_corm
};

enum ads_Excitation_BC_ConnectorMotion_cormEnm
{
    ads_Excitation_BC_ConnectorMotion_corm_CORM_1,
    ads_Excitation_BC_ConnectorMotion_corm_CORM_2,
    ads_Excitation_BC_ConnectorMotion_corm_CORM_3,
    ads_Excitation_BC_ConnectorMotion_corm_CORM_4,
    ads_Excitation_BC_ConnectorMotion_corm_CORM_5,
    ads_Excitation_BC_ConnectorMotion_corm_CORM_6
};

/** Enum with record members. */
enum ads_Excitation_BC_ElectricPotentialMembersEnm
{
    ads_Excitation_BC_ElectricPotential_user
};

/** Enum with record members. */
enum ads_Excitation_BC_FIXEDMembersEnm
{
    ads_Excitation_BC_FIXED_user
};

/** Enum with association roles. */
enum ads_Excitation_BC_FIXED_dofTypesRolesEnm
{
    ads_Excitation_BC_FIXED_dofTypes_referent,
    ads_Excitation_BC_FIXED_dofTypes_referrer
};

/** Enum with record members. */
enum ads_Excitation_BC_FluidElectricPotentialMembersEnm
{
    ads_Excitation_BC_FluidElectricPotential_user
};

/** Enum with record members. */
enum ads_Excitation_BC_IonConcentrationMembersEnm
{
    ads_Excitation_BC_IonConcentration_user
};

/** Enum with record members. */
enum ads_Excitation_BC_PressureMembersEnm
{
    ads_Excitation_BC_Pressure_user
};

/** Enum with record members. */
enum ads_Excitation_BC_RestraintSymbolicMembersEnm
{
    ads_Excitation_BC_RestraintSymbolic_user,
    ads_Excitation_BC_RestraintSymbolic_restraint
};

enum ads_Excitation_BC_RestraintSymbolic_restraintEnm
{
    ads_Excitation_BC_RestraintSymbolic_restraint_ENCASTRE,
    ads_Excitation_BC_RestraintSymbolic_restraint_NODEFORM,
    ads_Excitation_BC_RestraintSymbolic_restraint_NOOVAL,
    ads_Excitation_BC_RestraintSymbolic_restraint_NOWARP,
    ads_Excitation_BC_RestraintSymbolic_restraint_PINNED,
    ads_Excitation_BC_RestraintSymbolic_restraint_XASYMM,
    ads_Excitation_BC_RestraintSymbolic_restraint_XSYMM,
    ads_Excitation_BC_RestraintSymbolic_restraint_YASYMM,
    ads_Excitation_BC_RestraintSymbolic_restraint_YSYMM,
    ads_Excitation_BC_RestraintSymbolic_restraint_ZASYMM,
    ads_Excitation_BC_RestraintSymbolic_restraint_ZSYMM
};

/** Enum with record members. */
enum ads_Excitation_BC_TemperatureMembersEnm
{
    ads_Excitation_BC_Temperature_user
};

/** Enum with association roles. */
enum ads_Excitation_BC_Temperature_temperaturePointsRolesEnm
{
    ads_Excitation_BC_Temperature_temperaturePoints_referent,
    ads_Excitation_BC_Temperature_temperaturePoints_referrer
};

/** Enum with record members. */
enum ads_Excitation_BC_TransportVelocityMembersEnm
{
    ads_Excitation_BC_TransportVelocity_user
};

/** Enum with record members. */
enum ads_Excitation_BC_UVAMembersEnm
{
    ads_Excitation_BC_UVA_user
};

/** Enum with association roles. */
enum ads_Excitation_BC_UVA_dofTypeRolesEnm
{
    ads_Excitation_BC_UVA_dofType_referent,
    ads_Excitation_BC_UVA_dofType_referrer
};

/** Enum with association roles. */
enum ads_Excitation_BC_submodelRolesEnm
{
    ads_Excitation_BC_submodel_child,
    ads_Excitation_BC_submodel_parent
};

/** Enum with record members. */
enum ads_Excitation_CFDMembersEnm
{
    ads_Excitation_CFD_user
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECoolingMembersEnm
{
    ads_Excitation_CFD_ECooling_user
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECooling_BlowerMembersEnm
{
    ads_Excitation_CFD_ECooling_Blower_user,
    ads_Excitation_CFD_ECooling_Blower_operatingRPM
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_Blower_tableRolesEnm
{
    ads_Excitation_CFD_ECooling_Blower_table_child,
    ads_Excitation_CFD_ECooling_Blower_table_parent
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECooling_CHeatSinkMembersEnm
{
    ads_Excitation_CFD_ECooling_CHeatSink_user
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECooling_CHeatSink_ManualMembersEnm
{
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_user,
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_dxx,
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_dyy,
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_dzz,
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_kxx,
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_kyy,
    ads_Excitation_CFD_ECooling_CHeatSink_Manual_kzz
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_CHeatSink_supportRolesEnm
{
    ads_Excitation_CFD_ECooling_CHeatSink_support_referent,
    ads_Excitation_CFD_ECooling_CHeatSink_support_referrer
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECooling_CompactPCBMembersEnm
{
    ads_Excitation_CFD_ECooling_CompactPCB_user,
    ads_Excitation_CFD_ECooling_CompactPCB_power
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_CompactPCB_supportRolesEnm
{
    ads_Excitation_CFD_ECooling_CompactPCB_support_referent,
    ads_Excitation_CFD_ECooling_CompactPCB_support_referrer
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECooling_PerforatedPlateMembersEnm
{
    ads_Excitation_CFD_ECooling_PerforatedPlate_user,
    ads_Excitation_CFD_ECooling_PerforatedPlate_approachVelocity,
    ads_Excitation_CFD_ECooling_PerforatedPlate_modelType,
    ads_Excitation_CFD_ECooling_PerforatedPlate_reynoldNumber,
    ads_Excitation_CFD_ECooling_PerforatedPlate_thickness
};

enum ads_Excitation_CFD_ECooling_PerforatedPlate_modelTypeEnm
{
    ads_Excitation_CFD_ECooling_PerforatedPlate_modelType_BOUNDARY,
    ads_Excitation_CFD_ECooling_PerforatedPlate_modelType_INTERFACE,
    ads_Excitation_CFD_ECooling_PerforatedPlate_modelType_ZONE
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_PerforatedPlate_supportRolesEnm
{
    ads_Excitation_CFD_ECooling_PerforatedPlate_support_referent,
    ads_Excitation_CFD_ECooling_PerforatedPlate_support_referrer
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_PerforatedPlate_tableRolesEnm
{
    ads_Excitation_CFD_ECooling_PerforatedPlate_table_child,
    ads_Excitation_CFD_ECooling_PerforatedPlate_table_parent
};

/** 
Enum with record members. */
enum ads_Excitation_CFD_ECooling_ThermoelectricCoolerMembersEnm
{
    ads_Excitation_CFD_ECooling_ThermoelectricCooler_user,
    ads_Excitation_CFD_ECooling_ThermoelectricCooler_currentOperating,
    ads_Excitation_CFD_ECooling_ThermoelectricCooler_voltageOperating
};

/** Enum with record members. */
enum ads_Excitation_CFD_ECooling_TwoResistorMembersEnm
{
    ads_Excitation_CFD_ECooling_TwoResistor_user
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_TwoResistor_supportRolesEnm
{
    ads_Excitation_CFD_ECooling_TwoResistor_support_referent,
    ads_Excitation_CFD_ECooling_TwoResistor_support_referrer
};

/** Enum with association roles. */
enum ads_Excitation_CFD_ECooling_sectionRolesEnm
{
    ads_Excitation_CFD_ECooling_section_referent,
    ads_Excitation_CFD_ECooling_section_referrer
};

/** Enum with record members. */
enum ads_Excitation_ExternalFieldMembersEnm
{
    ads_Excitation_ExternalField_user
};

/** Enum with record members. */
enum ads_Excitation_LoadMembersEnm
{
    ads_Excitation_Load_user,
    ads_Excitation_Load_usub
};

/** 
Enum with record members. */
enum ads_Excitation_Load_BeamLineMembersEnm
{
    ads_Excitation_Load_BeamLine_user,
    ads_Excitation_Load_BeamLine_usub,
    ads_Excitation_Load_BeamLine_localDirection
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_BeamLine_componentRolesEnm
{
    ads_Excitation_Load_BeamLine_component_referent,
    ads_Excitation_Load_BeamLine_component_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_BodyFluxMembersEnm
{
    ads_Excitation_Load_BodyFlux_user,
    ads_Excitation_Load_BodyFlux_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_BodyForceMembersEnm
{
    ads_Excitation_Load_BodyForce_user,
    ads_Excitation_Load_BodyForce_usub
};

/** Enum with association roles. */
enum ads_Excitation_Load_BodyForce_componentRolesEnm
{
    ads_Excitation_Load_BodyForce_component_referent,
    ads_Excitation_Load_BodyForce_component_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedElectricChargeMembersEnm
{
    ads_Excitation_Load_ConcentratedElectricCharge_user,
    ads_Excitation_Load_ConcentratedElectricCharge_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedElectricCurrentMembersEnm
{
    ads_Excitation_Load_ConcentratedElectricCurrent_user,
    ads_Excitation_Load_ConcentratedElectricCurrent_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedFluxMembersEnm
{
    ads_Excitation_Load_ConcentratedFlux_user,
    ads_Excitation_Load_ConcentratedFlux_usub,
    ads_Excitation_Load_ConcentratedFlux_regionType
};

enum ads_Excitation_Load_ConcentratedFlux_regionTypeEnm
{
    ads_Excitation_Load_ConcentratedFlux_regionType_EULERIAN,
    ads_Excitation_Load_ConcentratedFlux_regionType_LAGRANGIAN,
    ads_Excitation_Load_ConcentratedFlux_regionType_SLIDING
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedFlux_PhantomEdgeMembersEnm
{
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_user,
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_usub,
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_regionType
};

enum ads_Excitation_Load_ConcentratedFlux_PhantomEdge_regionTypeEnm
{
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_regionType_EULERIAN,
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_regionType_LAGRANGIAN,
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_regionType_SLIDING
};

/** Enum with association roles. */
enum ads_Excitation_Load_ConcentratedFlux_PhantomEdge_node2RolesEnm
{
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_node2_referent,
    ads_Excitation_Load_ConcentratedFlux_PhantomEdge_node2_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedFlux_PhantomNodeMembersEnm
{
    ads_Excitation_Load_ConcentratedFlux_PhantomNode_user,
    ads_Excitation_Load_ConcentratedFlux_PhantomNode_usub,
    ads_Excitation_Load_ConcentratedFlux_PhantomNode_regionType
};

enum ads_Excitation_Load_ConcentratedFlux_PhantomNode_regionTypeEnm
{
    ads_Excitation_Load_ConcentratedFlux_PhantomNode_regionType_EULERIAN,
    ads_Excitation_Load_ConcentratedFlux_PhantomNode_regionType_LAGRANGIAN,
    ads_Excitation_Load_ConcentratedFlux_PhantomNode_regionType_SLIDING
};

/** Enum with association roles. */
enum ads_Excitation_Load_ConcentratedFlux_temperaturePointsRolesEnm
{
    ads_Excitation_Load_ConcentratedFlux_temperaturePoints_referent,
    ads_Excitation_Load_ConcentratedFlux_temperaturePoints_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedForceMembersEnm
{
    ads_Excitation_Load_ConcentratedForce_user,
    ads_Excitation_Load_ConcentratedForce_usub,
    ads_Excitation_Load_ConcentratedForce_follower
};

/** Enum with association roles. */
enum ads_Excitation_Load_ConcentratedForce_dofTypeRolesEnm
{
    ads_Excitation_Load_ConcentratedForce_dofType_referent,
    ads_Excitation_Load_ConcentratedForce_dofType_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_ConcentratedMomentMembersEnm
{
    ads_Excitation_Load_ConcentratedMoment_user,
    ads_Excitation_Load_ConcentratedMoment_usub,
    ads_Excitation_Load_ConcentratedMoment_follower
};

/** Enum with association roles. */
enum ads_Excitation_Load_ConcentratedMoment_dofTypeRolesEnm
{
    ads_Excitation_Load_ConcentratedMoment_dofType_referent,
    ads_Excitation_Load_ConcentratedMoment_dofType_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_ConnectorForceMembersEnm
{
    ads_Excitation_Load_ConnectorForce_user,
    ads_Excitation_Load_ConnectorForce_usub,
    ads_Excitation_Load_ConnectorForce_corm
};

enum ads_Excitation_Load_ConnectorForce_cormEnm
{
    ads_Excitation_Load_ConnectorForce_corm_CORM_1,
    ads_Excitation_Load_ConnectorForce_corm_CORM_2,
    ads_Excitation_Load_ConnectorForce_corm_CORM_3
};

/** Enum with record members. */
enum ads_Excitation_Load_ConnectorMomentMembersEnm
{
    ads_Excitation_Load_ConnectorMoment_user,
    ads_Excitation_Load_ConnectorMoment_usub,
    ads_Excitation_Load_ConnectorMoment_corm
};

enum ads_Excitation_Load_ConnectorMoment_cormEnm
{
    ads_Excitation_Load_ConnectorMoment_corm_CORM_4,
    ads_Excitation_Load_ConnectorMoment_corm_CORM_5,
    ads_Excitation_Load_ConnectorMoment_corm_CORM_6
};

/** 
Enum with record members. */
enum ads_Excitation_Load_CoolingChannelInletMembersEnm
{
    ads_Excitation_Load_CoolingChannelInlet_user,
    ads_Excitation_Load_CoolingChannelInlet_usub
};

/** 
Enum with record members. */
enum ads_Excitation_Load_CoolingChannelOutletMembersEnm
{
    ads_Excitation_Load_CoolingChannelOutlet_user,
    ads_Excitation_Load_CoolingChannelOutlet_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ElectricBodyChargeMembersEnm
{
    ads_Excitation_Load_ElectricBodyCharge_user,
    ads_Excitation_Load_ElectricBodyCharge_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ElectricBodyCurrentMembersEnm
{
    ads_Excitation_Load_ElectricBodyCurrent_user,
    ads_Excitation_Load_ElectricBodyCurrent_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ElectricSurfaceChargeMembersEnm
{
    ads_Excitation_Load_ElectricSurfaceCharge_user,
    ads_Excitation_Load_ElectricSurfaceCharge_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ElectricSurfaceCurrentMembersEnm
{
    ads_Excitation_Load_ElectricSurfaceCurrent_user,
    ads_Excitation_Load_ElectricSurfaceCurrent_usub
};

/** Enum with association roles. */
enum ads_Excitation_Load_ElectricSurfaceCurrent_currOrientationRolesEnm
{
    ads_Excitation_Load_ElectricSurfaceCurrent_currOrientation_referent,
    ads_Excitation_Load_ElectricSurfaceCurrent_currOrientation_referrer
};

/** Enum with association roles. */
enum ads_Excitation_Load_ElectricSurfaceCurrent_directionRolesEnm
{
    ads_Excitation_Load_ElectricSurfaceCurrent_direction_child,
    ads_Excitation_Load_ElectricSurfaceCurrent_direction_parent
};

/** Enum with record members. */
enum ads_Excitation_Load_ElectrolyteConcentratedElectricCurrentMembersEnm
{
    ads_Excitation_Load_ElectrolyteConcentratedElectricCurrent_user,
    ads_Excitation_Load_ElectrolyteConcentratedElectricCurrent_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_ElectrolyteElectricSurfaceCurrentMembersEnm
{
    ads_Excitation_Load_ElectrolyteElectricSurfaceCurrent_user,
    ads_Excitation_Load_ElectrolyteElectricSurfaceCurrent_usub
};

/** 
Enum with record members. */
enum ads_Excitation_Load_FluidPressurePenetrationMembersEnm
{
    ads_Excitation_Load_FluidPressurePenetration_user,
    ads_Excitation_Load_FluidPressurePenetration_usub,
    ads_Excitation_Load_FluidPressurePenetration_criticalContactPressure,
    ads_Excitation_Load_FluidPressurePenetration_timePeriod
};

/** 
Enum with record members. */
enum ads_Excitation_Load_FluidPressurePenetration_WettingAdvanceMembersEnm
{
    ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_user,
    ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_usub,
    ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_criticalContactPressure,
    ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_timePeriod
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_exposedNodesRolesEnm
{
    ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_exposedNodes_referent,
    ads_Excitation_Load_FluidPressurePenetration_WettingAdvance_exposedNodes_referrer
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_FluidPressurePenetration_contactPressureCurveRolesEnm
{
    ads_Excitation_Load_FluidPressurePenetration_contactPressureCurve_referent,
    ads_Excitation_Load_FluidPressurePenetration_contactPressureCurve_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_GravityMembersEnm
{
    ads_Excitation_Load_Gravity_user,
    ads_Excitation_Load_Gravity_usub
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_Gravity_gravityVectorRolesEnm
{
    ads_Excitation_Load_Gravity_gravityVector_referent,
    ads_Excitation_Load_Gravity_gravityVector_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_HeatFluxMembersEnm
{
    ads_Excitation_Load_HeatFlux_user,
    ads_Excitation_Load_HeatFlux_usub
};

/** Enum with association roles. */
enum ads_Excitation_Load_HeatFlux_faceIdRolesEnm
{
    ads_Excitation_Load_HeatFlux_faceId_referent,
    ads_Excitation_Load_HeatFlux_faceId_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_HydroStaticPressureMembersEnm
{
    ads_Excitation_Load_HydroStaticPressure_user,
    ads_Excitation_Load_HydroStaticPressure_usub,
    ads_Excitation_Load_HydroStaticPressure_zAtNonzeroPressure,
    ads_Excitation_Load_HydroStaticPressure_zAtZeroPressure
};

/** Enum with record members. */
enum ads_Excitation_Load_InertiaReliefMembersEnm
{
    ads_Excitation_Load_InertiaRelief_user,
    ads_Excitation_Load_InertiaRelief_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_IonBodyFluxMembersEnm
{
    ads_Excitation_Load_IonBodyFlux_user,
    ads_Excitation_Load_IonBodyFlux_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_IonConcentratedFluxMembersEnm
{
    ads_Excitation_Load_IonConcentratedFlux_user,
    ads_Excitation_Load_IonConcentratedFlux_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_IonSurfaceFluxMembersEnm
{
    ads_Excitation_Load_IonSurfaceFlux_user,
    ads_Excitation_Load_IonSurfaceFlux_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_MovingBodyFluxMembersEnm
{
    ads_Excitation_Load_MovingBodyFlux_user,
    ads_Excitation_Load_MovingBodyFlux_usub
};

/** Enum with association roles. */
enum ads_Excitation_Load_MovingBodyFlux_tableContainerRolesEnm
{
    ads_Excitation_Load_MovingBodyFlux_tableContainer_referent,
    ads_Excitation_Load_MovingBodyFlux_tableContainer_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_MultiphysicsMembersEnm
{
    ads_Excitation_Load_Multiphysics_user,
    ads_Excitation_Load_Multiphysics_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_Multiphysics_ButlerVolmerMembersEnm
{
    ads_Excitation_Load_Multiphysics_ButlerVolmer_user,
    ads_Excitation_Load_Multiphysics_ButlerVolmer_usub
};

/** Enum with association roles. */
enum ads_Excitation_Load_Multiphysics_faceIdRolesEnm
{
    ads_Excitation_Load_Multiphysics_faceId_referent,
    ads_Excitation_Load_Multiphysics_faceId_referrer
};

/** Enum with association roles. */
enum ads_Excitation_Load_Multiphysics_tableContainerRolesEnm
{
    ads_Excitation_Load_Multiphysics_tableContainer_referent,
    ads_Excitation_Load_Multiphysics_tableContainer_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_OhmicLossMembersEnm
{
    ads_Excitation_Load_OhmicLoss_user,
    ads_Excitation_Load_OhmicLoss_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_PorosityMembersEnm
{
    ads_Excitation_Load_Porosity_user,
    ads_Excitation_Load_Porosity_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_PorousZoneMembersEnm
{
    ads_Excitation_Load_PorousZone_user,
    ads_Excitation_Load_PorousZone_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_PorousZone_PVCurveMembersEnm
{
    ads_Excitation_Load_PorousZone_PVCurve_user,
    ads_Excitation_Load_PorousZone_PVCurve_usub,
    ads_Excitation_Load_PorousZone_PVCurve_anisotropyRatio,
    ads_Excitation_Load_PorousZone_PVCurve_thickness
};

/** Enum with association roles. */
enum ads_Excitation_Load_PorousZone_PVCurve_tableRolesEnm
{
    ads_Excitation_Load_PorousZone_PVCurve_table_child,
    ads_Excitation_Load_PorousZone_PVCurve_table_parent
};

/** Enum with record members. */
enum ads_Excitation_Load_PressureMembersEnm
{
    ads_Excitation_Load_Pressure_user,
    ads_Excitation_Load_Pressure_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_RRFMembersEnm
{
    ads_Excitation_Load_RRF_user,
    ads_Excitation_Load_RRF_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_RRF_CentrifugalMembersEnm
{
    ads_Excitation_Load_RRF_Centrifugal_user,
    ads_Excitation_Load_RRF_Centrifugal_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_RRF_CentrifugalRhoOmega2MembersEnm
{
    ads_Excitation_Load_RRF_CentrifugalRhoOmega2_user,
    ads_Excitation_Load_RRF_CentrifugalRhoOmega2_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_RRF_CoriolisMembersEnm
{
    ads_Excitation_Load_RRF_Coriolis_user,
    ads_Excitation_Load_RRF_Coriolis_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_RRF_RotaryAccelerationMembersEnm
{
    ads_Excitation_Load_RRF_RotaryAcceleration_user,
    ads_Excitation_Load_RRF_RotaryAcceleration_usub
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_RRF_axisDirectionRolesEnm
{
    ads_Excitation_Load_RRF_axisDirection_referent,
    ads_Excitation_Load_RRF_axisDirection_referrer
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_RRF_axisPointRolesEnm
{
    ads_Excitation_Load_RRF_axisPoint_referent,
    ads_Excitation_Load_RRF_axisPoint_referrer
};

/** Enum with record members. */
enum ads_Excitation_Load_ShellEdgeMembersEnm
{
    ads_Excitation_Load_ShellEdge_user,
    ads_Excitation_Load_ShellEdge_usub,
    ads_Excitation_Load_ShellEdge_constantResultant,
    ads_Excitation_Load_ShellEdge_follower
};

/** Enum with association roles. */
enum ads_Excitation_Load_ShellEdge_tractionVectorDirectionRolesEnm
{
    ads_Excitation_Load_ShellEdge_tractionVectorDirection_child,
    ads_Excitation_Load_ShellEdge_tractionVectorDirection_parent
};

/** Enum with record members. */
enum ads_Excitation_Load_TractionMembersEnm
{
    ads_Excitation_Load_Traction_user,
    ads_Excitation_Load_Traction_usub,
    ads_Excitation_Load_Traction_constantResultant,
    ads_Excitation_Load_Traction_follower
};

/** 
Enum with association roles. */
enum ads_Excitation_Load_Traction_tractionVectorDirectionRolesEnm
{
    ads_Excitation_Load_Traction_tractionVectorDirection_referent,
    ads_Excitation_Load_Traction_tractionVectorDirection_referrer
};

/** 
Enum with record members. */
enum ads_Excitation_Load_VolumetricSourceMembersEnm
{
    ads_Excitation_Load_VolumetricSource_user,
    ads_Excitation_Load_VolumetricSource_usub,
    ads_Excitation_Load_VolumetricSource_nonLinearConstant
};

/** Enum with record members. */
enum ads_Excitation_Load_ZoneMembersEnm
{
    ads_Excitation_Load_Zone_user,
    ads_Excitation_Load_Zone_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_Zone_MRFMembersEnm
{
    ads_Excitation_Load_Zone_MRF_user,
    ads_Excitation_Load_Zone_MRF_usub
};

/** Enum with record members. */
enum ads_Excitation_Load_Zone_SedimentationMembersEnm
{
    ads_Excitation_Load_Zone_Sedimentation_user,
    ads_Excitation_Load_Zone_Sedimentation_usub,
    ads_Excitation_Load_Zone_Sedimentation_backflowTolerance,
    ads_Excitation_Load_Zone_Sedimentation_sedimentMoveAlpha,
    ads_Excitation_Load_Zone_Sedimentation_sedimentMoveBeta,
    ads_Excitation_Load_Zone_Sedimentation_sedimentStart
};

/** Enum with record members. */
enum ads_Excitation_Load_Zone_Sedimentation_sedimentScalingMembersEnm
{
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_user,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_usub,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_backflowTolerance,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_sedimentMoveAlpha,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_sedimentMoveBeta,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_sedimentStart
};

/** Enum with record members. */
enum ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LinearMembersEnm
{
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_user,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_usub,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_backflowTolerance,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_sedimentMoveAlpha,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_sedimentMoveBeta,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_sedimentStart,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_sedimentScalingMax,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_Linear_sedimentScalingMin
};

/** Enum with record members. */
enum ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLawMembersEnm
{
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_user,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_usub,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_backflowTolerance,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_sedimentMoveAlpha,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_sedimentMoveBeta,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_sedimentStart,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_sedimentScalingLogPenalty,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_LogLaw_sedimentScalingMin
};

/** Enum with record members. */
enum ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLawMembersEnm
{
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_user,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_usub,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_backflowTolerance,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_sedimentMoveAlpha,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_sedimentMoveBeta,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_sedimentStart,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_sedimentScalingMax,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_sedimentScalingMin,
    ads_Excitation_Load_Zone_Sedimentation_sedimentScaling_PowerLaw_sedimentScalingPowPenalty
};

/** Enum with association roles. */
enum ads_Excitation_Load_scaleFactorsRolesEnm
{
    ads_Excitation_Load_scaleFactors_referent,
    ads_Excitation_Load_scaleFactors_referrer
};

/** Enum with record members. */
enum ads_Excitation_PFMembersEnm
{
    ads_Excitation_PF_user
};

/** 
Enum with record members. */
enum ads_Excitation_PF_MotionMembersEnm
{
    ads_Excitation_PF_Motion_user,
    ads_Excitation_PF_Motion_definition
};

enum ads_Excitation_PF_Motion_definitionEnm
{
    ads_Excitation_PF_Motion_definition_ROTATION,
    ads_Excitation_PF_Motion_definition_TRANSLATION,
    ads_Excitation_PF_Motion_definition_USER
};

/** Enum with association roles. */
enum ads_Excitation_PF_Motion_dofTypeRolesEnm
{
    ads_Excitation_PF_Motion_dofType_referent,
    ads_Excitation_PF_Motion_dofType_referrer
};

/** 
Enum with association roles. */
enum ads_Excitation_PF_Motion_rotationAxisNodesRolesEnm
{
    ads_Excitation_PF_Motion_rotationAxisNodes_referent,
    ads_Excitation_PF_Motion_rotationAxisNodes_referrer
};

/** Enum with association roles. */
enum ads_Excitation_PF_Motion_rotationAxisPointARolesEnm
{
    ads_Excitation_PF_Motion_rotationAxisPointA_child,
    ads_Excitation_PF_Motion_rotationAxisPointA_parent
};

/** Enum with association roles. */
enum ads_Excitation_PF_Motion_rotationAxisPointBRolesEnm
{
    ads_Excitation_PF_Motion_rotationAxisPointB_child,
    ads_Excitation_PF_Motion_rotationAxisPointB_parent
};

/** Enum with record members. */
enum ads_Excitation_PF_TemperatureMembersEnm
{
    ads_Excitation_PF_Temperature_user
};

/** Enum with record members. */
enum ads_Excitation_PF_UserFieldMembersEnm
{
    ads_Excitation_PF_UserField_user
};

/** 
Enum with association roles. */
enum ads_Excitation_PF_UserField_gradientFieldsRolesEnm
{
    ads_Excitation_PF_UserField_gradientFields_child,
    ads_Excitation_PF_UserField_gradientFields_parent
};

/** Enum with record members. */
enum ads_Excitation_RveMembersEnm
{
    ads_Excitation_Rve_user
};

/** Enum with record members. */
enum ads_Excitation_Rve_FarFieldConditionMembersEnm
{
    ads_Excitation_Rve_FarFieldCondition_user
};

/** Enum with record members. */
enum ads_Excitation_Rve_FarFieldLoadMembersEnm
{
    ads_Excitation_Rve_FarFieldLoad_user
};

/** Enum with association roles. */
enum ads_Excitation_Rve_rveRolesEnm
{
    ads_Excitation_Rve_rve_referent,
    ads_Excitation_Rve_rve_referrer
};

/** Enum with record members. */
enum ads_Excitation_SetMembersEnm
{
    ads_Excitation_Set_user,
    ads_Excitation_Set_scaleFactor
};

/** Enum with association roles. */
enum ads_Excitation_Set_loadSetExcitationsRolesEnm
{
    ads_Excitation_Set_loadSetExcitations_child,
    ads_Excitation_Set_loadSetExcitations_parent
};

/** Enum with association roles. */
enum ads_Excitation_fieldRolesEnm
{
    ads_Excitation_field_referent,
    ads_Excitation_field_referrer
};

/** Enum with association roles. */
enum ads_Excitation_fieldsRolesEnm
{
    ads_Excitation_fields_child,
    ads_Excitation_fields_parent
};

/** Enum with association roles. */
enum ads_Excitation_gradientRolesEnm
{
    ads_Excitation_gradient_referent,
    ads_Excitation_gradient_referrer
};

/** Enum with association roles. */
enum ads_Excitation_initWettedRegionRolesEnm
{
    ads_Excitation_initWettedRegion_referent,
    ads_Excitation_initWettedRegion_referrer
};

/** Enum with association roles. */
enum ads_Excitation_orientationRolesEnm
{
    ads_Excitation_orientation_referent,
    ads_Excitation_orientation_referrer
};

/** Enum with association roles. */
enum ads_Excitation_regionRolesEnm
{
    ads_Excitation_region_referent,
    ads_Excitation_region_referrer
};

/** 
Enum with record members. */
enum ads_FatigueLoadingBlockV1MembersEnm
{
    ads_FatigueLoadingBlockV1_loadEntityID,
    ads_FatigueLoadingBlockV1_repeats,
    ads_FatigueLoadingBlockV1_scaleFactor
};

/** 
Enum with record members. */
enum ads_FatigueLoadingBlockV1_HistoryFileMembersEnm
{
    ads_FatigueLoadingBlockV1_HistoryFile_loadEntityID,
    ads_FatigueLoadingBlockV1_HistoryFile_repeats,
    ads_FatigueLoadingBlockV1_HistoryFile_scaleFactor,
    ads_FatigueLoadingBlockV1_HistoryFile_historyFilePath,
    ads_FatigueLoadingBlockV1_HistoryFile_historyIndex
};

/** 
Enum with record members. */
enum ads_FatigueLoadingBlockV1_MultiplierMembersEnm
{
    ads_FatigueLoadingBlockV1_Multiplier_loadEntityID,
    ads_FatigueLoadingBlockV1_Multiplier_repeats,
    ads_FatigueLoadingBlockV1_Multiplier_scaleFactor,
    ads_FatigueLoadingBlockV1_Multiplier_maxLoadMultiplier,
    ads_FatigueLoadingBlockV1_Multiplier_minLoadMultiplier
};

/** 
Enum with record members. */
enum ads_FatigueLoadingBlockV1_SequenceMembersEnm
{
    ads_FatigueLoadingBlockV1_Sequence_loadEntityID,
    ads_FatigueLoadingBlockV1_Sequence_repeats,
    ads_FatigueLoadingBlockV1_Sequence_scaleFactor
};

/** Enum with association roles. */
enum ads_Interaction_CFD_sectionRolesEnm
{
    ads_Interaction_CFD_section_referent,
    ads_Interaction_CFD_section_referrer
};

/** Enum with association roles. */
enum ads_Model_excitationsRolesEnm
{
    ads_Model_excitations_child,
    ads_Model_excitations_parent
};

/** 
Enum with association roles. */
enum ads_Model_fatigueLoadingBlockV1RolesEnm
{
    ads_Model_fatigueLoadingBlockV1_child,
    ads_Model_fatigueLoadingBlockV1_parent
};

/** Enum with association roles. */
enum ads_Model_psdRolesEnm
{
    ads_Model_psd_child,
    ads_Model_psd_parent
};

/** 
Enum with record members. */
enum ads_PSDMembersEnm
{
    ads_PSD_dbReference,
    ads_PSD_g,
    ads_PSD_type
};

enum ads_PSD_typeEnm
{
    ads_PSD_type_BASE,
    ads_PSD_type_DB,
    ads_PSD_type_FORCE
};

/** Enum with association roles. */
enum ads_PSD_tableRolesEnm
{
    ads_PSD_table_child,
    ads_PSD_table_parent
};

/** Enum with association roles. */
enum ads_PSD_unitsRecordsRolesEnm
{
    ads_PSD_unitsRecords_child,
    ads_PSD_unitsRecords_parent
};

/** 
Enum with record members. */
enum ads_RelayModulatorMembersEnm
{
    ads_RelayModulator_scaleFactor
};

/** Enum with association roles. */
enum ads_RelayModulator_amplitudeRolesEnm
{
    ads_RelayModulator_amplitude_referent,
    ads_RelayModulator_amplitude_referrer
};

/** Enum with association roles. */
enum ads_RelayModulator_fieldRolesEnm
{
    ads_RelayModulator_field_referent,
    ads_RelayModulator_field_referrer
};

/** 
Enum with association roles. */
enum ads_RelayModulator_intervalStepIncRolesEnm
{
    ads_RelayModulator_intervalStepInc_child,
    ads_RelayModulator_intervalStepInc_parent
};

/** Enum with association roles. */
enum ads_SecondaryBase_excitationBCsRolesEnm
{
    ads_SecondaryBase_excitationBCs_referent,
    ads_SecondaryBase_excitationBCs_referrer
};

/** 
Enum with record members. */
enum ads_SelectEigenModesMembersEnm
{
    ads_SelectEigenModes_definition
};

enum ads_SelectEigenModes_definitionEnm
{
    ads_SelectEigenModes_definition_FREQUENCY_RANGE,
    ads_SelectEigenModes_definition_MODE_NUMBERS
};

/** 
Enum with association roles. */
enum ads_SelectEigenModes_modesRolesEnm
{
    ads_SelectEigenModes_modes_referent,
    ads_SelectEigenModes_modes_referrer
};

/** Enum with association roles. */
enum ads_SelectEigenModes_tableRolesEnm
{
    ads_SelectEigenModes_table_child,
    ads_SelectEigenModes_table_parent
};

/** Enum with association roles. */
enum ads_Step_Lin_ModalRandomResponse_correlationRolesEnm
{
    ads_Step_Lin_ModalRandomResponse_correlation_child,
    ads_Step_Lin_ModalRandomResponse_correlation_parent
};

/** 
Enum with association roles. */
enum ads_Task_excitationsRolesEnm
{
    ads_Task_excitations_child,
    ads_Task_excitations_parent
};

/** 
Enum with association roles. */
enum ads_Task_selectEigenModesRolesEnm
{
    ads_Task_selectEigenModes_child,
    ads_Task_selectEigenModes_parent
};

/** Enum with record members. */
enum ads_ValveGateMembersEnm
{
    ads_ValveGate_user,
    ads_ValveGate_controlMethod,
    ads_ValveGate_diameter,
    ads_ValveGate_end,
    ads_ValveGate_normal,
    ads_ValveGate_position,
    ads_ValveGate_start,
    ads_ValveGate_type
};

enum ads_ValveGate_controlMethodEnm
{
    ads_ValveGate_controlMethod_AUTOMATIC,
    ads_ValveGate_controlMethod_TIMEPERCENT,
    ads_ValveGate_controlMethod_VOLUMEPERCENT
};

enum ads_ValveGate_typeEnm
{
    ads_ValveGate_type_POINT,
    ads_ValveGate_type_SURFACE
};

#endif
