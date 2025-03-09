//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreParameterC_h
#define ads_CoreParameterC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Parameter of the latest level of form Core */

/** A collection of design parameters. */
#define ads_GlobalCollections_parameterCollection (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 0))

/** A Link to design parameters. */
#define ads_Model_designParameters (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 1))

/** Parameters. A parameter may or may not be a design parameter, and it may or may not be a shape paramater. */
#define ads_Model_parameters (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 2))

/** Represents a parameter. A parameter may or may not be a design parameter, and it may or may not be a shape paramater. */
#define ads_Parameter (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 3))

/** The collection of design parameters */
#define ads_ParameterCollection (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 4))

/** A field which represents a shape vector. The shape vector is the deriviative of co-ordinate with respect to the parameter. */
#define ads_Parameter_shapeVariation (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 5))

/** The value of the parameter. */
#define ads_Parameter_value (ads_CoreFragmentTypeIndex(ads_CoreParameterFragment, 6))

/** 
Enum with association roles. */
enum ads_GlobalCollections_parameterCollectionRolesEnm
{
    ads_GlobalCollections_parameterCollection_child,
    ads_GlobalCollections_parameterCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_designParametersRolesEnm
{
    ads_Model_designParameters_referent,
    ads_Model_designParameters_referrer
};

/** 
Enum with association roles. */
enum ads_Model_parametersRolesEnm
{
    ads_Model_parameters_child,
    ads_Model_parameters_parent
};

/** 
Enum with association roles. */
enum ads_Parameter_shapeVariationRolesEnm
{
    ads_Parameter_shapeVariation_referent,
    ads_Parameter_shapeVariation_referrer
};

/** 
Enum with association roles. */
enum ads_Parameter_valueRolesEnm
{
    ads_Parameter_value_child,
    ads_Parameter_value_parent
};

#endif
