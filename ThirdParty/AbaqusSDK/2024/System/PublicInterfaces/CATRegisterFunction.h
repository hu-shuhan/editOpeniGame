#include "CATRegisterFunction_required.h"
#ifndef __CATRegisterFunction
#define __CATRegisterFunction

#include "CATOMInitializer.h"   // CATOMInitializerProlog / CATOMInitializerEpilog

// COPYRIGHT DASSAULT SYSTEMES 2012

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/* 
 * @nodoc
 * Use CATRegisterFunction to register a function of a defined type associated to a identifier name
 * Should be associated with a following line in .func dictionary of the framework : KeyName Type_Function libName
 * @param KeyName        : the identifier of the registered function.
 * @param Ptr_Function   : the pointer on the function, must be compliant with the type Type_Function.
 * @param Type_Function  : the type of the registered function.
 */
#define CATRegisterFunction(KeyName,Ptr_Function,Type_Function) \
Type_Function ForTestingIfFunctionIsRightType##KeyName##Type_Function = Ptr_Function; \
CATOMInitializerProlog(LocalFillDictionary__##KeyName##Type_Function)\
  [[maybe_unused]] char LocalFillDictionary__##KeyName##Type_Function = CATFillDictionary::RegisterFunction(#KeyName,Ptr_Function,#Type_Function);\
CATOMInitializerEpilog(LocalFillDictionary__##KeyName##Type_Function);

#endif // __CATRegisterFunction
