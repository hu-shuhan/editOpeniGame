#ifndef CATSysObjCClass_H
#define CATSysObjCClass_H

/* COPYRIGHT DASSAULT SYSTEMES 2000 */
/** @CAA2Required */
/************************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS   */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME   */
/************************************************************************/

/**
 * This header lets use forward declaration of ObjC object in both ObjC files and C++ files
 */
#ifdef __OBJC__
#define OBJC_CLASS(name) @class name
#elif defined (__cplusplus)
#define OBJC_CLASS(name) class name
#else // C
#define OBJC_CLASS(name) typedef struct objc_object name
#endif

#endif
