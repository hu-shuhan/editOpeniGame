//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreContourC_h
#define ads_CoreContourC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Contour of the latest level of form Core */

/** A contour */
#define ads_Contour (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 0))

/** A collection of contours */
#define ads_ContourCollection (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 1))

/** The base crack object */
#define ads_Crack (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 2))

/** A collection of cracks */
#define ads_CrackCollection (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 3))

/** A crack defined with crack normals and no crack tip nodes */
#define ads_NormalCrack (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 4))

/** A crack with a normal and crack tip nodes defined */
#define ads_NormalCrackTipCrack (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 5))

/** A crack type with a virtual crack extension direction and no crack tip nodes */
#define ads_VirtualCrackExtensionCrack (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 6))

/** Defines a crack with crack tip nodes and virtual crack directions */
#define ads_VirtualCrackTipCrack (ads_CoreFragmentTypeIndex(ads_CoreContourFragment, 7))

/** 
Enum with record members. */
enum ads_CrackMembersEnm
{
    ads_Crack_symmetric
};

/** 
Enum with record members. */
enum ads_NormalCrackMembersEnm
{
    ads_NormalCrack_symmetric
};

/** 
Enum with record members. */
enum ads_NormalCrackTipCrackMembersEnm
{
    ads_NormalCrackTipCrack_symmetric
};

/** 
Enum with record members. */
enum ads_VirtualCrackExtensionCrackMembersEnm
{
    ads_VirtualCrackExtensionCrack_symmetric
};

/** 
Enum with record members. */
enum ads_VirtualCrackTipCrackMembersEnm
{
    ads_VirtualCrackTipCrack_symmetric
};

#endif
