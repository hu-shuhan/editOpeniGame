
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * These #undef statements clean-up the preprocessor so that
 * the hash table declaration macros can be reused at some other
 * point in the source code to declare a hash table on some other
 * type of objects.
 */
#undef	CATHTAB_Retrieve
#undef	CATHTAB_KeyLocate
#undef	CATHTAB_KeyLocatePosition
#undef	CATHTAB_Locate
#undef	CATHTAB_LocatePosition
#undef	CATHTAB_Next
#undef	CATHTAB_NextPosition
#undef	CATHTAB_Remove
#undef	CATHTAB_RemovePosition
#undef	CATHTAB_RemoveAll
#undef	CATHTAB_Dimension
#undef	CATHTAB_Collisions
#undef	CATHTAB_Unused
#undef	CATHTAB_PrintStats
#undef	CATHTAB_PrintAddr
#undef	CATHTAB_ApplyMemberFunct
#undef	CATHTAB_ApplyMemberFunctConst
#undef	CATHTAB_ApplyGlobalFunct
#undef	CATHTAB_ApplyDelete
#undef	CATHTAB_ostreamOP
#undef	CATHTAB_Rebuild

#undef	CATCOLLEC_ExportedBy
#define	CATCOLLEC_ExportedBy
