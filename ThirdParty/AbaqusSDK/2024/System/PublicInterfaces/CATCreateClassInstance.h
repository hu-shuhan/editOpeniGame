#include "CATCreateClassInstance_required.h"
#ifndef __CATCreateClassInstance
#define __CATCreateClassInstance

// COPYRIGHT DASSAULT SYSTEMES 2012

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
 
#include "CATFillDictionary.h"
#include "CATOMInitializer.h"   // CATOMInitializerProlog / CATOMInitializerEpilog

/**
 * Set of macro used to register a factory function that will be used to create implementation instances.
 *
 * These macros associate a factory function to a string identifier and register this couple in the in-session functions table.
 * The factory function is either implemented by the macro itself either given to the macro as an argument. 
 * Choose the right macro for your need, depending if you want
 *      - customizing the creation of the instance or not.
 *      - giving a specific identifier to this function or let the default one (the implementation name).
 * It's also mandatory to create a new entry in the .func dictionary of your framework in order ObjectModeler could load the 
 * implementation library if necessary. This entry will be as this : "IdentifierName OMFactoryFunctionType libMyImplLibrary"
 * If your framework doesn't already have a .func file, create it in the dictionaries directory (same as for .dic files).
 *
 * The two instantiation by name methods, CATInstantiateComponent and CATCreateClassInstance, will query first to the in-session functions 
 * table if such factory function exists before trying to use the CATICreateInstance mechanism. This means that implementations of CATICreateInstance
 * can be replaced by these macros with a big gain in code size. 
 */
 
/* Use CATCreateAndAddOMFactoryFunction macro if the factory function is the usual one (creation of a new instance of an OM component).
 * The identifier of this factory function will be the OM component name ("class")
 * @param class is the OM component to be instantiated, it must derivate from CATBaseUnknown.
 */
#define CATCreateAndAddOMFactoryFunction(class) \
static CATBaseUnknown* OMFactoryFunct##class() {\
     CATBaseUnknown * rc = new class();\
     return rc;\
}\
CATOMInitializerProlog(LocalFillDictionary__##class)\
  [[maybe_unused]] char LocalFillDictionary__##class = CATFillDictionary::RegisterFunctionCreation(#class,OMFactoryFunct##class);\
CATOMInitializerEpilog(LocalFillDictionary__##class);

/* Use CATCreateAndAddOMFactoryFunctionWithSpecificKeyName macro if the factory function is the usual one (creation of a new instance of an OM component)
 * and if the identifier of this factory function is different from the OM component name.
 * @param class is the OM component to be instantiated, it must derivate from CATBaseUnknown.
 * @param keyname is the identifier string of this factory function.
 */
#define CATCreateAndAddOMFactoryFunctionWithSpecificKeyName(keyname,class) \
static CATBaseUnknown* OMFactoryFunct##class() {\
     CATBaseUnknown * rc = new class();\
     return rc;\
}\
CATOMInitializerProlog(LocalFillDictionary__##keyname)\
  [[maybe_unused]] char LocalFillDictionary__##keyname = CATFillDictionary::RegisterFunctionCreation(#keyname,OMFactoryFunct##class);\
CATOMInitializerEpilog(LocalFillDictionary__##keyname);

/* Use CATAddOMFactoryFunction macro if the factory function is customized.
 * @param keyname is the identifier string of this factory function.
 * @param MyFunction is the customized factory function, it must be compliant with the typedef OMFactoryFunctionType.
 */
#define CATAddOMFactoryFunction(keyname,MyFunction) \
CATOMInitializerProlog(LocalFillDictionary__##keyname)\
  [[maybe_unused]] char LocalFillDictionary__##keyname = CATFillDictionary::RegisterFunctionCreation(#keyname,MyFunction);\
CATOMInitializerEpilog(LocalFillDictionary__##keyname);


#endif // __CATCreateClassInstance
