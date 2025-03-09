//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef mdl_propAttributeState_h
#define mdl_propAttributeState_h


// Description: 
// The enum isused to provide information about the 
// value of a propagating object's attribute
// The value describes the state of the attribute 
// on a given Step and compares it with states
// on previous Steps.
// The symbolic constant is used by the ACL as
// an alternative value to attributes. For
// instance: cf1=UNCHANGED instead of cf1=123
// Both the enum and the const are supposed to be 
// generic enough to be used by all objects in all modules
// that may need it
// Main use: the mdl_PropAttr constructor



// Includes

// Begin local includes

#include <omu_PrimEnum.h>

enum mdl_PropAttributeStateEnm 
{
    mdl_UNSET_ATT, 
    mdl_MODIFIED_ATT, 
    mdl_PROPAGATED_ATT,
    mdl_MODIF_CONDITION_ATT, //toggle off a togglable att, ex: bc's dofs
    mdl_NOT_APPLICABLE_ATT,
    mdl_INVALID_ATT,
    mdl_ABSOLUTE_DEFAULT_ATT,
    mdl_COMPUTED_DEFAULT_ATT
};

#ifdef SMA_NO_TGEN 
extern template class omu_PrimEnum<mdl_PropAttributeStateEnm,1>;
#endif
typedef omu_PrimEnum<mdl_PropAttributeStateEnm,1> mdl_PropAttributeStateConst;



#endif
