#include "CATCreateExternalObject_required.h"
#ifndef __CATCreateExternalObject
#define __CATCreateExternalObject

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "JS0CORBA.h"
#include "CATFillDictionary.h"
#include "CATOMInitializer.h"   // CATOMInitializerProlog / CATOMInitializerEpilog

#ifndef Exported
/** @nodoc */
#define Exported DSYExport
#include "DSYExport.h"
#endif


/**
 * Defines the command creation function.
 * @param command Command name used as input of the @href CATCreateExternalObject function.
 * @param class The object to create.
 */
#define MacCreateCommand(command,class)                     \
                                                            \
extern "C" Exported void *fctCreate##command(void *_pt_Arg) \
{                                                           \
   return (new class());                                    \
}                                                           \
                                                            \
CATOMInitializerProlog(static_##command_##class)            \
  CATFillDictionary static_##command_##class(#command,fctCreate##command);\
CATOMInitializerEpilog(static_##command_##class);


/**
 * Defines the command creation function.
 * @param Class Class of the command to create and used as input
 * of the @href CATCreateExternalObject function. 
 */
#define CATCreateClass(Class)                               \
                                                            \
extern "C" Exported void *fctCreate##Class(void *_pt_Arg)   \
{                                                           \
   return (new Class());                                    \
}                                                           \
                                                            \
CATOMInitializerProlog(static_Create##Class)                \
  CATFillDictionary static_Create##Class(#Class,fctCreate##Class);\
CATOMInitializerEpilog(static_Create##Class)

/**
 * Defines the command creation function with an argument.
 * @param Class Class of the command to create and used as input 
 * of the @href CATCreateExternalObject function. 
 * @param TypArg Argument type expected by the function
 */
#define CATCreateClassArg(Class,TypArg)                     \
                                                            \
extern "C" Exported void *fctCreate##Class(TypArg *_pt_Arg) \
{                                                           \
   return (new Class(_pt_Arg));                             \
}                                                           \
                                                            \
CATOMInitializerProlog(static_CreateArg##Class)             \
  CATFillDictionary static_CreateArg##Class(#Class,fctCreate##Class);\
CATOMInitializerEpilog(static_CreateArg##Class)


/**
 * Creates a command by instantiating the class that represents it.
 * @param iClassName
 *   Name of the class to instantiate to create the command
 * @param iLibPath   
 *   Pathname of the directory which contains the command executable code
 * @param iLibSpec
 *   Name of the library used to retrieve its executable code file.
 *   CATCreateExternalObject possibly adds a prefix and a suffix to this name to
 *   match the operating system naming convention. For example, the command
 *   CD0PRINT is named libCD0PRINT.a with AIX, libCD0PRINT.sl with HP, libCD0PRINT.so
 *   with IRIX and SOLARIS, and CD0PRINT.dll with Windows/NT.
 * @param iArg
 *   Argument value passed to the class constructor. 
 * @return
 *   A pointer to the created object or 0 if an error occured.
 */
ExportedByJS0CORBA extern void *CATCreateExternalObject(CATClassId iClassName,
                                                        const char* iLibPath = nullptr,
                                                        const char* iLibSpec = nullptr,
                                                        void *iArg = nullptr);
#endif
