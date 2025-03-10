//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreConnectorTableC_h
#define ads_CoreConnectorTableC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment ConnectorTable of the latest level of form Core */

#define ads_CMecFailureTable (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 0))

#define ads_CMecLockTable (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 1))

#define ads_CMecStopTable (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 2))

/** Abstraction of tabular properties for connectors. */
#define ads_ConnectorPropertyTable (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 3))

/** Specify control options to refine tabular data associated with connector behaviors. */
#define ads_ConnectorPropertyTableOptions (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 4))

/** Two dimensional grid spanning property rows and tensor components. */
#define ads_DofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 5))

/** Anchor for tabular properties in nonlinear connector elasticity definition. */
#define ads_NonLinearConnectorElasticityTable (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 6))

#define ads_Prop_CMec_Failure_table (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 7))

#define ads_Prop_CMec_Lock_table (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 8))

#define ads_Prop_CMec_Lock_tables (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 9))

#define ads_Prop_CMec_Stop_table (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 10))

/** Specify control options to refine tabular data associated with connector behaviors. */
#define ads_Prop_CMec_TableOptions (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 11))

#define ads_Prop_CMec_tableOptions (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 12))

/** Two dimensional grid spanning property rows and tensor components. */
#define ads_PropertyTableRowDofTypeGrid (ads_CoreFragmentTypeIndex(ads_CoreConnectorTableFragment, 13))

/** 
Enum with record members. */
enum ads_ConnectorPropertyTableMembersEnm
{
    ads_ConnectorPropertyTable_extrapolation,
    ads_ConnectorPropertyTable_name,
    ads_ConnectorPropertyTable_regularize,
    ads_ConnectorPropertyTable_rtol
};

enum ads_ConnectorPropertyTable_extrapolationEnm
{
    ads_ConnectorPropertyTable_extrapolation_CONSTANT,
    ads_ConnectorPropertyTable_extrapolation_LINEAR
};

enum ads_ConnectorPropertyTable_regularizeEnm
{
    ads_ConnectorPropertyTable_regularize_OFF,
    ads_ConnectorPropertyTable_regularize_ON,
    ads_ConnectorPropertyTable_regularize_ORIGIN
};

/** 
Enum with record members. */
enum ads_ConnectorPropertyTableOptionsMembersEnm
{
    ads_ConnectorPropertyTableOptions_extrapolation,
    ads_ConnectorPropertyTableOptions_rTol,
    ads_ConnectorPropertyTableOptions_regularize
};

enum ads_ConnectorPropertyTableOptions_extrapolationEnm
{
    ads_ConnectorPropertyTableOptions_extrapolation_CONSTANT,
    ads_ConnectorPropertyTableOptions_extrapolation_LINEAR
};

/** 
Enum with grid dimensions. */
enum ads_DofTypeGridDimensionsEnm
{
    ads_DofTypeGrid_components
};

/** 
Enum with record members. */
enum ads_NonLinearConnectorElasticityTableMembersEnm
{
    ads_NonLinearConnectorElasticityTable_extrapolation,
    ads_NonLinearConnectorElasticityTable_name,
    ads_NonLinearConnectorElasticityTable_regularize,
    ads_NonLinearConnectorElasticityTable_rtol
};

enum ads_NonLinearConnectorElasticityTable_extrapolationEnm
{
    ads_NonLinearConnectorElasticityTable_extrapolation_CONSTANT,
    ads_NonLinearConnectorElasticityTable_extrapolation_LINEAR
};

enum ads_NonLinearConnectorElasticityTable_regularizeEnm
{
    ads_NonLinearConnectorElasticityTable_regularize_OFF,
    ads_NonLinearConnectorElasticityTable_regularize_ON,
    ads_NonLinearConnectorElasticityTable_regularize_ORIGIN
};

/** Enum with association roles. */
enum ads_Prop_CMec_Failure_tableRolesEnm
{
    ads_Prop_CMec_Failure_table_child,
    ads_Prop_CMec_Failure_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Lock_tableRolesEnm
{
    ads_Prop_CMec_Lock_table_child,
    ads_Prop_CMec_Lock_table_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Lock_tablesRolesEnm
{
    ads_Prop_CMec_Lock_tables_child,
    ads_Prop_CMec_Lock_tables_parent
};

/** Enum with association roles. */
enum ads_Prop_CMec_Stop_tableRolesEnm
{
    ads_Prop_CMec_Stop_table_child,
    ads_Prop_CMec_Stop_table_parent
};

/** 
Enum with record members. */
enum ads_Prop_CMec_TableOptionsMembersEnm
{
    ads_Prop_CMec_TableOptions_extrapolation,
    ads_Prop_CMec_TableOptions_rTol,
    ads_Prop_CMec_TableOptions_regularize
};

enum ads_Prop_CMec_TableOptions_extrapolationEnm
{
    ads_Prop_CMec_TableOptions_extrapolation_CONSTANT,
    ads_Prop_CMec_TableOptions_extrapolation_LINEAR
};

/** Enum with association roles. */
enum ads_Prop_CMec_tableOptionsRolesEnm
{
    ads_Prop_CMec_tableOptions_child,
    ads_Prop_CMec_tableOptions_parent
};

/** 
Enum with grid dimensions. */
enum ads_PropertyTableRowDofTypeGridDimensionsEnm
{
    ads_PropertyTableRowDofTypeGrid_components,
    ads_PropertyTableRowDofTypeGrid_row
};

#endif
