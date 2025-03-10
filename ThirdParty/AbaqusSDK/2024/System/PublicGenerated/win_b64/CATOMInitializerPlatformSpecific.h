// COPYRIGHT DASSAULT SYSTEMES 2021
#if !defined(CATOMInitializerPlatformSpecific_H)
#define CATOMInitializerPlatformSpecific_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

#if !defined(CATOMInitializer_CompileTimeRegisterDisable)
#if defined(_MSC_VER)
// Visual C++

#define CATOM_BINARY_SECTION_NAME_MK(suffix)   CATOM_XSTR(CATOM_EVALUATOR(CATOM_BINARY_SECTION_NAME, suffix))

// Prevent removal of symbol 'sym_name' by the linker in Optimized/Release build
// cf. options /OPT:REF, which eliminates functions and data that are never referenced (COMDATs)
// Other solution: pass these options on LINK's command line /OPT:NOREF [/LTCG]
// cf. https://docs.microsoft.com/en-us/cpp/build/reference/opt-optimizations
// cf. https://docs.microsoft.com/en-US/cpp/build/reference/include-force-symbol-references
// NOTE: the symbols are not exported from the generated module
#if defined(_WIN64)
#define CATOM_ATTR_USED(sym_name)   __pragma(comment(linker, "/INCLUDE:" sym_name))
#else   // 32 bit: symbols decorated with a leading underscore
// cf. https://docs.microsoft.com/en-us/cpp/build/reference/decorated-names?view=msvc-160&viewFallbackFrom=vs-2019#FormatC
#define CATOM_ATTR_USED(sym_name)   __pragma(comment(linker, "/INCLUDE:_" sym_name))
#endif  // _WIN64

#define CATOM_SECTION_DECL  \
    __pragma(section(CATOM_BINARY_SECTION_NAME_MK($a),read))    \
    __pragma(section(CATOM_BINARY_SECTION_NAME_MK($m),read))    \
    __pragma(section(CATOM_BINARY_SECTION_NAME_MK($z),read))    \
                            \
    __declspec(allocate(CATOM_BINARY_SECTION_NAME_MK($a))) __declspec(selectany) extern   \
    const PCATOMFuncInit_t CATOM_SECTION_START = nullptr;       \
    __declspec(allocate(CATOM_BINARY_SECTION_NAME_MK($z))) __declspec(selectany) extern   \
    const PCATOMFuncInit_t CATOM_SECTION_END   = nullptr;


#define CATOM_REGISTER_INITIALIZER(id, fn) \
    CATOM_ATTR_USED(CATOM_XSTR(CATOM_INIT_VAR_NAME(id))) /* Incompatible with 'static' qualification? */ \
    extern "C" __declspec(allocate(CATOM_BINARY_SECTION_NAME_MK($m))) \
    const PCATOMFuncInit_t CATOM_INIT_VAR_NAME(id) = fn


#else   // _MSC_VER check
#error "Unknown platform, not currently supported" 
#endif
#endif  // NOT CATOMInitializer_CompileTimeRegisterDisable


#endif  // CATOMInitializerPlatformSpecific_H
