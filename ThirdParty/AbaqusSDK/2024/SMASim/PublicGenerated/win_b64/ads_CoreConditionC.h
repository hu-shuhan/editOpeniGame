//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreConditionC_h
#define ads_CoreConditionC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Condition of the latest level of form Core */

/** An "escape" mechanism to be able to define structures similar to record types without using SSDL. The first application of this type is for diagnostics. In that case there are thousands of different diagnostics types, and we prefer to not represent each one a different record type. In that case the "metadata" about the condition types is in the dia tables. */
#define ads_Condition (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 0))

/** This is the key piece of the non-SSDL Condition type. A Condition contains a list of arguments, each argument being a specialization of this record. */
#define ads_ConditionArgument (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 1))

/** Used when part of the data in a Condition is actually in a c-set. For example: Element set. */
#define ads_ConditionArgument_CSet (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 2))

/** Condition argument referencing a c-set. */
#define ads_ConditionArgument_CSet_cset (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 3))

/** Persistence mechanism for the double values of a Condition. */
#define ads_ConditionArgument_Double (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 4))

/** Persistence mechanism for the integer values of a Condition. */
#define ads_ConditionArgument_Int (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 5))

/** Used when part of the data in a Condition is actually in a different record. For example: Field. */
#define ads_ConditionArgument_Record (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 6))

/** If the data of a Condition includes the reference to more than one record, create multiple ConditionArgument_Record instances. Implementation note: the anyChildType="true" specification may be replaced by "any record type", if that becomes available. */
#define ads_ConditionArgument_Record_record (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 7))

/** Persistence mechanism for the string values of a Condition. */
#define ads_ConditionArgument_String (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 8))

/** The data in a Condition. For more info see the Condition documentation. */
#define ads_Condition_arguments (ads_CoreFragmentTypeIndex(ads_CoreConditionFragment, 9))

/** 
Enum with record members. */
enum ads_ConditionMembersEnm
{
    ads_Condition_category,
    ads_Condition_severity,
    ads_Condition_type
};

enum ads_Condition_categoryEnm
{
    ads_Condition_category_CONTACT_DIAGNOSTIC,
    ads_Condition_category_ELEMENT_DIAGNOSTIC,
    ads_Condition_category_GENERIC,
    ads_Condition_category_WARNING_OR_ERROR
};

enum ads_Condition_severityEnm
{
    ads_Condition_severity_ERROR,
    ads_Condition_severity_FATAL_ERROR,
    ads_Condition_severity_NOTE,
    ads_Condition_severity_WARNING
};

/** 
Enum with association roles. */
enum ads_ConditionArgument_CSet_csetRolesEnm
{
    ads_ConditionArgument_CSet_cset_referent,
    ads_ConditionArgument_CSet_cset_referrer
};

/** 
Enum with record members. */
enum ads_ConditionArgument_DoubleMembersEnm
{
    ads_ConditionArgument_Double_value
};

/** 
Enum with record members. */
enum ads_ConditionArgument_IntMembersEnm
{
    ads_ConditionArgument_Int_value
};

/** 
Enum with association roles. */
enum ads_ConditionArgument_Record_recordRolesEnm
{
    ads_ConditionArgument_Record_record_referent,
    ads_ConditionArgument_Record_record_referrer
};

/** 
Enum with record members. */
enum ads_ConditionArgument_StringMembersEnm
{
    ads_ConditionArgument_String_value
};

/** 
Enum with association roles. */
enum ads_Condition_argumentsRolesEnm
{
    ads_Condition_arguments_child,
    ads_Condition_arguments_parent
};

#endif
