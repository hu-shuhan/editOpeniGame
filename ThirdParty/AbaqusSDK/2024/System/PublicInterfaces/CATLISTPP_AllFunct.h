
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/** @nodoc 
 * Switches used by the macros to declare classes which implement 
 * lists of pointers to a user-defined type X with a pre-allocated
 * static buffer.
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
#define CATLISTPP_CtorFromArrayPtrs

/** @nodoc */
#define CATLISTPP_Append

/** @nodoc */
#define CATLISTPP_AppendList

/** @nodoc */
#define CATLISTPP_InsertAt

/** @nodoc */
#define CATLISTPP_ReSize

/** @nodoc */
#define CATLISTPP_Storage

/** @nodoc */
#define CATLISTPP_Locate

/** @nodoc */
#define CATLISTPP_RemoveValue

/** @nodoc */
#define CATLISTPP_RemoveList

/** @nodoc */
#define CATLISTPP_RemovePosition

/** @nodoc */
#define CATLISTPP_RemoveAll

/** @nodoc */
#define CATLISTPP_RemoveNulls

/** @nodoc */
#define CATLISTPP_RemoveDuplicates

/** @nodoc */
#define CATLISTPP_Compare

/** @nodoc */
#define CATLISTPP_Swap

/** @nodoc */
#define CATLISTPP_QuickSort

/** @nodoc */
#define CATLISTPP_FillArrayPtrs

/** @nodoc */
#define CATLISTPP_NbOccur

/** @nodoc */
#define CATLISTPP_Intersection
