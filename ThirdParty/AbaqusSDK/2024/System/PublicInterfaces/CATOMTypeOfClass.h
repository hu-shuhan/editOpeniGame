#ifndef CATOMTypeOfClass_H
#define CATOMTypeOfClass_H

// COPYRIGHT DASSAULT SYSTEMES 2023

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/**
 * @nodoc
 * <b>Role</b>: Type of class.
 * Used by private methods only, except for the following values used by the CATImplementClass macro:
 * <dl>
 * <dt>Implementation <dd>for a class that implements interfaces
 * <dt>DataExtension  <dd>for a class that implements interfaces as an extension
 *                        of another class and whose data members contain
 *                        persistent data
 * <dt>CodeExtension  <dd>for a class that implements interfaces as an extension
 *                        of another class and which doesn't contain any data
 *                        member, but only methods
 * <dt>CacheExtension <dd>for a class that implements interfaces as an extension
 *                        of another class and which doesn't contain any data
 *                        member, but only methods
 * <dt>TransientExtension <dd>for a class that implements interfaces as an extension
 *                        of another class and whose data members contain temporary data.
 * </dl>
 */
typedef enum ENUMTypeOfClass : unsigned char {
    NothingType,
    Implementation,
    DataExtension,
    CodeExtension,
    CacheExtension,
    TransientExtension,
    TIE,
    TIEchain,
    Interfaces
} TypeOfClass;

#endif  // CATOMTypeOfClass_H
