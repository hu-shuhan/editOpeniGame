//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreCosimulationC_h
#define ads_CoreCosimulationC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Cosimulation of the latest level of form Core */

/** The Co-Simulation Interface record is used to indicate which regions in the model are involved a co-simulation, and what field quantities are to be imported and exported. */
#define ads_CosimulationInterface (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 0))

/** Link between a co-simulation interface and the exported field type */
#define ads_CosimulationInterface_exportedFields (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 1))

/** Link between a co-simulation interface and the exported sensors */
#define ads_CosimulationInterface_exportedSensors (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 2))

/** Link between a co-simulation interface and the imported actuators */
#define ads_CosimulationInterface_importedActuators (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 3))

/** Link between a co-simulation interface and the imported field type */
#define ads_CosimulationInterface_importedFields (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 4))

/** Link between a co-simulation interface and the region */
#define ads_CosimulationInterface_region (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 5))

/** Link between a Task and co-simulation interface. */
#define ads_Task_cosimulationInterfaces (ads_CoreFragmentTypeIndex(ads_CoreCosimulationFragment, 6))

/** 
Enum with association roles. */
enum ads_CosimulationInterface_exportedFieldsRolesEnm
{
    ads_CosimulationInterface_exportedFields_referent,
    ads_CosimulationInterface_exportedFields_referrer
};

/** 
Enum with association roles. */
enum ads_CosimulationInterface_exportedSensorsRolesEnm
{
    ads_CosimulationInterface_exportedSensors_referent,
    ads_CosimulationInterface_exportedSensors_referrer
};

/** 
Enum with association roles. */
enum ads_CosimulationInterface_importedActuatorsRolesEnm
{
    ads_CosimulationInterface_importedActuators_referent,
    ads_CosimulationInterface_importedActuators_referrer
};

/** 
Enum with association roles. */
enum ads_CosimulationInterface_importedFieldsRolesEnm
{
    ads_CosimulationInterface_importedFields_referent,
    ads_CosimulationInterface_importedFields_referrer
};

/** 
Enum with association roles. */
enum ads_CosimulationInterface_regionRolesEnm
{
    ads_CosimulationInterface_region_referent,
    ads_CosimulationInterface_region_referrer
};

/** 
Enum with association roles. */
enum ads_Task_cosimulationInterfacesRolesEnm
{
    ads_Task_cosimulationInterfaces_child,
    ads_Task_cosimulationInterfaces_parent
};

#endif
