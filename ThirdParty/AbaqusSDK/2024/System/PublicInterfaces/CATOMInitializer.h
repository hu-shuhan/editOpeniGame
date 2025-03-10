// COPYRIGHT DASSAULT SYSTEMES 2021
#if !defined(CATOMInitializer_H)
#define CATOMInitializer_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// Copyright DASSAULT SYSTEMS 2021
//-----------------------------------------------------------------------------
// Abstract: expose macros CATOMInitializerProlog / CATOMInitializerEpilog
// that pack an initializer in a custom binary section on supported platforms
//-----------------------------------------------------------------------------

// IMPORTANT: platform-dependent code is placed in a platform-generated header 
// in order to avoid unnecessary coupling between platforms, 
// so that platform-specific modifications don't require a useless rebuild for all other platforms
#include "CATOMInitializerPlatformSpecific.h"   // CATOM_SECTION_DECL + CATOM_REGISTER_INITIALIZER

#define CATOM_PASTER(x,y) x ## y
#define CATOM_INIT_VAR_NAME(id)     CATOM_PASTER(CATOMInit_,id)


#if !defined(CATOM_REGISTER_INITIALIZER)

// Fallback implementation: execute the initializer right away
#define CATOM_REGISTER_INITIALIZER(id, fn) static const CATOMModule* CATOM_INIT_VAR_NAME(id) = fn()

#define CATOMInitializerProlog(id)     static
#define CATOMInitializerEpilog(id)     /* Expects a semicolon */

#define CATOMInitializer_Fallback

#elif !defined(CATOMInitializerProlog)  // IMPORTANT: allows to override the following common bricks 
// in the platform-specific header: CATOMInitializerPlatformSpecific.h


#include "DSYExport.h"  // DSYExport / DSYImport / DSYLocal
#include <cstdint>      // uintptr_t


#define CATOM_STR(s) #s
#define CATOM_XSTR(s) CATOM_STR(s)
#define CATOM_EVALUATOR(x,y)  CATOM_PASTER(x,y)

// Section names must be 8 characters or less (Visual C++)
#define CATOM_BINARY_SECTION_NAME   dsyom

// Follow GCC name convention, compiler which generates __start_/__stop_ variables automatically when CATOM_REGISTER_INITIALIZER is used
// (Provided that the section name only contains characters valid for variable names - dot not supported)
#define CATOM_SECTION_START     CATOM_EVALUATOR(__start_, CATOM_BINARY_SECTION_NAME)
#define CATOM_SECTION_END       CATOM_EVALUATOR(__stop_ , CATOM_BINARY_SECTION_NAME)



struct CATOMModuleInfo {
    uintptr_t sec_start;
    uintptr_t sec_end;
};

class CATOMModule 
{
public:
    ExportedByJS0CORBA CATOMModule(CATOMModuleInfo const & info);
    
private:
    static void atexit_handler();    // May be used as an alternative to a destructor
};


using PCATOMFuncInit_t = CATOMModule* (*)(void);




CATOM_SECTION_DECL  // Depends on PCATOMFuncInit_t / CATOM_SECTION_START macro etc and defines variables CATOM_SECTION_START / CATOM_SECTION_END




/**
 * _CATOMModule.
 * The 'DSYLocal' attribute is crucial for initialization to trigger as expected.
 * The key is to make sure that the symbol referring to the singleton is private to the module.
 */
template<typename _Typ>
class DSYLocal _CATOMModule : public CATOMModule
{
public:
    _CATOMModule() : 
        // The following constructor is hosted in System code
        CATOMModule({ /*.sec_start =*/ (uintptr_t)&CATOM_SECTION_START, /*.sec_end =*/ (uintptr_t)&CATOM_SECTION_END }) 
    {
    }
    
    static _CATOMModule<_Typ> s_Inst;
};

template<typename _Typ>
_CATOMModule<_Typ> _CATOMModule<_Typ>::s_Inst;





#define CATOMInitializerProlog(id)                      \
    static CATOMModule* CATOMInitFunc_##id() {
        
#define CATOMInitializerEpilog(id)                      \
        /* Force implicit instanciation of the '_CATOMModule<void>::s_Inst' singleton ONLY in modules that call this macro at least once. */\
        /* The objective is not to pollute modules that do not use ObjectModeler, that is to say by generating neither the singleton, nor the custom binary section in such cases. */\
        /* IMPORTANT: we need to make sure that the compiler cannot assume that it can elide the singleton! */\
        /* We do that by returning the signleton's address */\
        return &(_CATOMModule<void>::s_Inst);           \
    }                                                   \
    CATOM_REGISTER_INITIALIZER(id, CATOMInitFunc_##id)   /* Expects a semicolon */


#endif  // CATOM_REGISTER_INITIALIZER

#endif  // CATOMInitializer_H
