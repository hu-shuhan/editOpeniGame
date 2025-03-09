//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CorePropertyTableC_h
#define ads_CorePropertyTableC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment PropertyTable of the latest level of form Core */

/** A collection of field variables for property tables. */
#define ads_GlobalCollections_propertyFieldVariableTypeCollection (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 0))

/** A counting PropertyTable collection. */
#define ads_GlobalCollections_propertyTableCollection (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 1))

/** A collection of property table rows. */
#define ads_GlobalCollections_propertyTableRowCollection (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 2))

/** Identifies a field variable dependency for a property. */
#define ads_PropertyFieldVariableType (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 3))

/** Collection used to identify field variable types. */
#define ads_PropertyFieldVariableTypeCollection (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 4))

/** A collection of property tables. */
#define ads_PropertyTableCollection (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 5))

/** Elements of this grid are assignments of property tables to elements. */
#define ads_PropertyTableElementGrid (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 6))

/** A row in a property table. */
#define ads_PropertyTableRow (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 7))

/** Collection used to count property table rows */
#define ads_PropertyTableRowCollection (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 8))

/** Two dimensional grid spanning property rows and tensor components. */
#define ads_PropertyTableRowComponentGrid (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 9))

/** Single dimensional grid spanning property rows. */
#define ads_PropertyTableRowGrid (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 10))

/** Two dimensional grid spanning property rows and field variables. */
#define ads_PropertyTableRowPropertyFieldVariableTypeGrid (ads_CoreFragmentTypeIndex(ads_CorePropertyTableFragment, 11))

/** 
Enum with association roles. */
enum ads_GlobalCollections_propertyFieldVariableTypeCollectionRolesEnm
{
    ads_GlobalCollections_propertyFieldVariableTypeCollection_child,
    ads_GlobalCollections_propertyFieldVariableTypeCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_propertyTableCollectionRolesEnm
{
    ads_GlobalCollections_propertyTableCollection_child,
    ads_GlobalCollections_propertyTableCollection_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_propertyTableRowCollectionRolesEnm
{
    ads_GlobalCollections_propertyTableRowCollection_child,
    ads_GlobalCollections_propertyTableRowCollection_parent
};

/** 
Enum with grid dimensions. */
enum ads_PropertyTableElementGridDimensionsEnm
{
    ads_PropertyTableElementGrid_element,
    ads_PropertyTableElementGrid_propertyTable
};

/** 
Enum with grid dimensions. */
enum ads_PropertyTableRowComponentGridDimensionsEnm
{
    ads_PropertyTableRowComponentGrid_components,
    ads_PropertyTableRowComponentGrid_row
};

/** 
Enum with grid dimensions. */
enum ads_PropertyTableRowGridDimensionsEnm
{
    ads_PropertyTableRowGrid_cell
};

/** 
Enum with grid dimensions. */
enum ads_PropertyTableRowPropertyFieldVariableTypeGridDimensionsEnm
{
    ads_PropertyTableRowPropertyFieldVariableTypeGrid_fields,
    ads_PropertyTableRowPropertyFieldVariableTypeGrid_row
};

#endif
