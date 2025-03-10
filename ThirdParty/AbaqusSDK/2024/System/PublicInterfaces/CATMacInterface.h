

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// DO NOT USE THESE MACs, USE CATMacForIUnknown.h INSTEAD


#ifndef __CATMacInterface
#define __CATMacInterface

#include "JS0CORBA.h"
#include "CATMetaObject.h"
#include "CATMacForIUnknown.h"


/* Macro used inside the include file defining a class, ie ".h" */
#define CATDeclareKindOf	CATDeclareClass

/* Macro used inside the code file defining a class, ie ".cpp" */
#define CATBeginImplementKindOf(Class,Typeofclass,Basemeta,Impmeta)	\
CATBeginImplementClass(Class,Typeofclass,Basemeta,Impmeta)

/* Macro used to end the declaration of multiple extensions */		\
#define CATEndImplementKindOf(Class)					\
CATEndImplementClass(Class)

/* Macro used to declare a new implementation linked to an extension */	\
#define CATAddExtendedImplementation(implementation)			\
CATAddClassExtension(implementation)

/* Macro used inside the code file defining a class, ie ".cpp" */
#define CATImplementKindOf(Class,Typeofclass,Basemeta,Impmeta)		\
CATBeginImplementKindOf(Class,Typeofclass,Basemeta,Impmeta);		\
CATEndImplementKindOf(Class)

#endif
