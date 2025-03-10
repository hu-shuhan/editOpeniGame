
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/** @nodoc 
 * Switches used by the macros to declare classes which implement 
 * lists of values of user-defined type X.
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
#define	CATLISTV_CtorFromArrayVals

/** @nodoc */
#define	CATLISTV_CtorFromArrayPtrs

/** @nodoc */
#define	CATLISTV_AppendList

/** @nodoc */
#define	CATLISTV_InsertAfter

/** @nodoc */
#define	CATLISTV_InsertBefore

/** @nodoc */
#define	CATLISTV_InsertAt

/** @nodoc */
#define	CATLISTV_ReSize

/** @nodoc */
#define	CATLISTV_Locate

/** @nodoc */
#define	CATLISTV_RemoveValue

/** @nodoc */
#define	CATLISTV_RemoveList

/** @nodoc */
#define	CATLISTV_RemovePosition

/** @nodoc */
#define	CATLISTV_RemoveDuplicatesExtract

/** @nodoc */
#define	CATLISTV_RemoveDuplicatesCount

/** @nodoc */
#define	CATLISTV_geOP

/** @nodoc */
#define	CATLISTV_leOP

/** @nodoc */
#define	CATLISTV_gtOP

/** @nodoc */
#define	CATLISTV_ltOP

/** @nodoc */
#define	CATLISTV_Compare

/** @nodoc */
#define	CATLISTV_Replace

/** @nodoc */
#define	CATLISTV_Swap

/** @nodoc */
#define	CATLISTV_QuickSort

/** @nodoc */
#define	CATLISTV_ArrayVals

/** @nodoc */
#define	CATLISTV_ArrayPtrs

/** @nodoc */
#define	CATLISTV_NbOccur

/** @nodoc */
#define	CATLISTV_Intersection

/** @nodoc */
#define	CATLISTV_Apply
