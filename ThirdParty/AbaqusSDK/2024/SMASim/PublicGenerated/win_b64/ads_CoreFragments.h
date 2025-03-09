//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreFragments_h
#define ads_CoreFragments_h

// Enumeration with all fragments of this form
enum
{
    ads_CoreFragment,
    ads_CoreCfdStepFragment,
    ads_CoreCfdStepOldFragment,
    ads_CoreConditionFragment,
    ads_CoreConnectorDofFragment,
    ads_CoreConnectorLibraryFragment,
    ads_CoreConnectorTableFragment,
    ads_CoreConstraintsFragment,
    ads_CoreContourFragment,
    ads_CoreCosimulationFragment,
    ads_CoreCurveFragment,
    ads_CoreDiagnosticsFragment,
    ads_CoreEventsFragment,
    ads_CoreExcitationFragment,
    ads_CoreFESystemFragment,
    ads_CoreFatigueFragment,
    ads_CoreFieldFragment,
    ads_CoreFiltersFragment,
    ads_CoreFocusFragment,
    ads_CoreInfrastructureFragment,
    ads_CoreInitialConditionsFragment,
    ads_CoreInteractionsFragment,
    ads_CoreInteractionsContactFragment,
    ads_CoreInternalDataFragment,
    ads_CoreLayerFragment,
    ads_CoreMaterialLibraryFragment,
    ads_CoreMeshFragment,
    ads_CoreMicrotopologyFragment,
    ads_CoreModelFragment,
    ads_CoreModelOldFragment,
    ads_CoreOrientationsFragment,
    ads_CoreOutputRequestsFragment,
    ads_CoreParameterFragment,
    ads_CorePropertyConcreteFragment,
    ads_CorePropertyCreepFragment,
    ads_CorePropertyDamageFragment,
    ads_CorePropertyElasticHyperFragment,
    ads_CorePropertyElasticLinearFragment,
    ads_CorePropertyElasticOtherFragment,
    ads_CorePropertyElasticStructFragment,
    ads_CorePropertyFatigueFragment,
    ads_CorePropertyFluidFragment,
    ads_CorePropertyGeneralFragment,
    ads_CorePropertyHighLevelFragment,
    ads_CorePropertyIMFragment,
    ads_CorePropertyMultiPhysicsFragment,
    ads_CorePropertyOtherFragment,
    ads_CorePropertyPlasticFragment,
    ads_CorePropertyTableFragment,
    ads_CorePropertyTestDataFragment,
    ads_CorePropertyThermalFragment,
    ads_CorePropertyUniaxialFragment,
    ads_CorePropertyViscoFragment,
    ads_CoreRegionFragment,
    ads_CoreResultsFragment,
    ads_CoreSectionFragment,
    ads_CoreSectionPropFragment,
    ads_CoreStepEigenSolverFragment,
    ads_CoreStepIMFragment,
    ads_CoreStepTimeIncrementFragment,
    ads_CoreStepsFragment,
    ads_CoreSurfaceFragment,
    ads_CoreTasksFragment,
    ads_CoreUnitFragment
};

int ads_CoreFragmentTypeIndex(int fragment, int fragmentTypeIndex);

#endif
