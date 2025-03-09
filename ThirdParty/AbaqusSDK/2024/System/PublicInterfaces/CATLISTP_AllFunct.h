
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/** @nodoc 
 * Switches used by the macros to declare classes which implement 
 * lists of pointers to a user-defined type X.
 * <br><b>Role</b>
 * Each #define statement acts as a switch which triggers
 * the declaration and the definition of a method within a
 * list class. This lets you create list classes
 * with just the functionalities which are appropriate to
 * your problem, thereby sparing memory. 
 * If you want the full set of available
 * functionalities, you should include this file in your list class 
 * declaration file. This will cause all list methods to be defined for
 * your list class.
 */
#define	CATLISTP_CtorFromArrayPtrs

/** @nodoc */
#define	CATLISTP_Append

/** @nodoc */
#define	CATLISTP_AppendList

/** @nodoc */
#define	CATLISTP_InsertAt

/** @nodoc */
#define	CATLISTP_ReSize

/** @nodoc */
#define	CATLISTP_Locate

/** @nodoc */
#define	CATLISTP_RemoveValue

/** @nodoc */
#define	CATLISTP_RemoveList

/** @nodoc */
#define	CATLISTP_RemovePosition

/** @nodoc */
#define	CATLISTP_RemoveAll

/** @nodoc */
#define	CATLISTP_RemoveNulls

/** @nodoc */
#define	CATLISTP_RemoveDuplicates

/** @nodoc */
#define	CATLISTP_Compare

/** @nodoc */
#define	CATLISTP_Swap

/** @nodoc */
#define	CATLISTP_QuickSort

/** @nodoc */
#define	CATLISTP_FillArrayPtrs

/** @nodoc */
#define	CATLISTP_NbOccur

/** @nodoc */
#define	CATLISTP_Intersection
