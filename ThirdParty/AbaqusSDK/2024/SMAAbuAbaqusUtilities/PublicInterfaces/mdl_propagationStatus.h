//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef mdl_propagationStatus_h
#define mdl_propagationStatus_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: mdl_propagationStatus.h
//
// Description:  Propagation status enumerated values.  Not all enumerated
//               values are valid for all propagating keywords types. The
//               allowable range of status values for particular keyword type
//               are enforced using symbolic constants.  The various symbolic
//               constants are defined in the appropriate propagating keyword
//               base class header file, e.g. kloC_Load.h.
//

// Includes

// Begin local includes

#include <cow_List.h>
#include <omu_PrimEnum.h>
// Forward declarations

// Propagation status enum

enum mdl_PropagationStatusEnm
{
    mdl_NOT_YET_ACTIVE,
    mdl_CREATED,
    mdl_PROPAGATED,
    mdl_MODIFIED,
    mdl_DEACTIVATED,
    mdl_NO_LONGER_ACTIVE,
    mdl_TYPE_NOT_APPLICABLE,
    mdl_INSTANCE_NOT_APPLICABLE,
    mdl_BUILT_INTO_BASE_STATE,
    mdl_PROPAGATED_FROM_BASE_STATE,
    mdl_MODIFIED_FROM_BASE_STATE,
    mdl_DEACTIVATED_FROM_BASE_STATE,
    mdl_BUILT_INTO_MODES,
    mdl_COMPUTED,
    mdl_PROPAGATED_FROM_COMPUTED,
    mdl_RESET_TO_INITIAL,
    mdl_PROPAGATED_FROM_FREQUENCY
};

COW_LIST_ITER_BLT_DECL(mdl_PropagationStatusEnm, mdl_ListPropagationStatusEnm);

#ifdef SMA_NO_TGEN 
extern template class omu_PrimEnum<mdl_PropagationStatusEnm,1>;
#endif
typedef omu_PrimEnum<mdl_PropagationStatusEnm,1> mdl_PropagationStatusConst;


// Returns true for those statuses that are considered deactivated statuses
bool mdl_deactivatedPropagationStatus(mdl_PropagationStatusEnm status);

// Returns true for those statuses that are considered active statuses
bool mdl_activePropagationStatus(mdl_PropagationStatusEnm status);

// Returns true for statuses thatr belong to General Analysis Steps
bool mdl_generalAnalysisStatus(mdl_PropagationStatusEnm status);

#endif /* mdl_propagationStatus_h */
