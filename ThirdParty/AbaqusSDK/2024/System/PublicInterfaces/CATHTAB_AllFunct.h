
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/** 
 * Switches used by the macros to declare classes which implement 
 * hash table of pointers to a user-defined type X.
 * <br><b>Role</b>
 * Each #define statement acts as a switch which triggers
 * the declaration and the definition of a method within a
 * hash table class. This lets you create hash table classes
 * with just the functionalities which are appropriate to
 * your problem, thereby sparing memory. 
 * If you want the full set of available
 * functionalities, you should include this file in your hash table class 
 * declaration file. This will cause all hash table methods to be defined for
 * your hash table class.
 */

/**
 * @nodoc
 */
#define	CATHTAB_Retrieve
/**
 * @nodoc
 */
#define	CATHTAB_KeyLocate
/**
 * @nodoc
 */
#define	CATHTAB_KeyLocatePosition
/**
 * @nodoc
 */
#define	CATHTAB_Locate
/**
 * @nodoc
 */
#define	CATHTAB_LocatePosition
/**
 * @nodoc
 */
#define	CATHTAB_Next
/**
 * @nodoc
 */
#define	CATHTAB_NextPosition
/**
 * @nodoc
 */
#define	CATHTAB_Remove
/**
 * @nodoc
 */
#define	CATHTAB_RemovePosition
/**
 * @nodoc
 */
#define	CATHTAB_RemoveAll
/**
 * @nodoc
 */
#define	CATHTAB_Dimension
/**
 * @nodoc
 */
#define	CATHTAB_Collisions
/**
 * @nodoc
 */
#define	CATHTAB_Unused
/**
 * @nodoc
 */
#define	CATHTAB_PrintStats
/**
 * @nodoc
 */
#define	CATHTAB_PrintAddr
/**
 * @nodoc
 */
#define	CATHTAB_ApplyMemberFunct
/**
 * @nodoc
 */
#define	CATHTAB_ApplyMemberFunctConst
/**
 * @nodoc
 */
#define	CATHTAB_ApplyGlobalFunct
/**
 * @nodoc
 */
#define	CATHTAB_ApplyDelete
/**
 * @nodoc
 */
#define	CATHTAB_ostreamOP
/**
 * @nodoc
 */
