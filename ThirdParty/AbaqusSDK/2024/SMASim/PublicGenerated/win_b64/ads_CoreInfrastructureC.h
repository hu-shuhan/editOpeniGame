//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreInfrastructureC_h
#define ads_CoreInfrastructureC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Infrastructure of the latest level of form Core */

/** This data structure stores a map from c-members in one collection to c-members in another collection of the same type. The map contains two c-sets of the same size. The mapping is done by pairing each c-members in one set to another c-members in the same relative poisition in the other set. The same map (the same pair of c-sets) can be used in Image operations in either direction; the "from" and "to" roles do not imply the map is uni-directional. Certain optimizations may decide to store one of the c-set as an ascending c-set, but that is not required by the CMemberMap schema. If one of the c-sets is ascending, then the map is optimized in one direction. It makes sense in that case to put the ascending c-set in the "from" side of the map and the ordered c-set in the "to" side. */
#define ads_CMemberMap (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 0))

/** One of the two c-sets in the map. The "from" word is just used to make it easy to pair each CSet with the appropriate collection. The two CSets, themselves, establish no directionality in the map. I.e., the data represented by the pair of CSets can be used to map indices in either direction. */
#define ads_CMemberMap_fromCSet (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 1))

/** One of the two c-sets in the map. The "to" word is just used to make it easy to pair each CSet with the appropriate collection. The two CSets, themselves, establish no directionality in the map. I.e., the data represented by the pair of CSets can be used to map indices in either direction. */
#define ads_CMemberMap_toCSet (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 2))

/** The GenericCollection is a mechanism to avoid "typing" a dimension of a distribution. Most distributions in SIM are strongly typed: the c-sets (and D-sets) of different distributions come from different collections, captured in the schema. For example, distributions with node- related values use indices coming from the NodeCollection, and distributions with element-related values use indices coming from the ElementCollection. The GenericCollection is used in those rare cases where the elements in the distribution are not captured by the schema and should not be captured by the schema. One example is a 1D distribution with as many entries as Drawers. We think it would be very strange to have a DrawerCollection type in the schema. Note that c-sets in this collection are not necessarily just indexing "the array"; they may have gaps in them (e.g.: c-set of DrawerID minors). */
#define ads_GenericCollection (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 3))

/** An abstract data type for Generic collection */
#define ads_GenericCollectionItem (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 4))

/** Single dimensional grid spanning Generic counting. */
#define ads_GenericGrid (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 5))

/** Generic collection. */
#define ads_GlobalCollections_genericCollection (ads_CoreFragmentTypeIndex(ads_CoreInfrastructureFragment, 6))

/** 
Enum with association roles. */
enum ads_CMemberMap_fromCSetRolesEnm
{
    ads_CMemberMap_fromCSet_referent,
    ads_CMemberMap_fromCSet_referrer
};

/** 
Enum with association roles. */
enum ads_CMemberMap_toCSetRolesEnm
{
    ads_CMemberMap_toCSet_referent,
    ads_CMemberMap_toCSet_referrer
};

/** 
Enum with grid dimensions. */
enum ads_GenericGridDimensionsEnm
{
    ads_GenericGrid_generic
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_genericCollectionRolesEnm
{
    ads_GlobalCollections_genericCollection_child,
    ads_GlobalCollections_genericCollection_parent
};

#endif
