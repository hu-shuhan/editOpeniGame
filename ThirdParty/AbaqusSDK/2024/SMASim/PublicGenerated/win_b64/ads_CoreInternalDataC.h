//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreInternalDataC_h
#define ads_CoreInternalDataC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment InternalData of the latest level of form Core */

/** This is the Analysis Mesh. The analysis mesh is internal to the analysis code. Compared to the Results Mesh, the analysis mesh has node and element indexing spaces that are compacted and permuted for analysis code optimization. Until we replace the ADB with SIM, the representation of the analysis mesh and its fields in SIM will be very incomplete. */
#define ads_Model_analysisMesh (ads_CoreFragmentTypeIndex(ads_CoreInternalDataFragment, 0))

/** 
Enum with association roles. */
enum ads_Model_analysisMeshRolesEnm
{
    ads_Model_analysisMesh_child,
    ads_Model_analysisMesh_parent
};

#endif
