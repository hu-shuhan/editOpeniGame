
#ifndef DSYSysMacForBuildtimeWarning_H
#define DSYSysMacForBuildtimeWarning_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/* COPYRIGHT DASSAULT SYSTEMES 2022 */
#include <type_traits>  // std::integral_constant




/** 
 * @nodoc DSY_SYS_STATIC_WARNING(cond, msg).
 * compile-time warning if "cond" evaluates to false 
 */
/** @nodoc */
#define DSY_SYS_STATIC_WARNING_CAT(x,y) DSY_SYS_STATIC_WARNING_CAT1(x,y)
/** @nodoc */
#define DSY_SYS_STATIC_WARNING_CAT1(x,y) x##y
/** @nodoc */
#define DSY_SYS_STATIC_WARNING(cond, msg) \
    struct DSY_SYS_STATIC_WARNING_CAT(static_warning,__LINE__) { \
        /* Define a function named "_", with 2 overloads, one of which is marked deprecated with our custom message */\
        [[maybe_unused]] constexpr void _(std::integral_constant<bool,true> const& ) {} \
        [[maybe_unused]] [[deprecated(msg)]] constexpr void _(std::integral_constant<bool,false> const& ) {} \
        /* Constructor */\
        [[maybe_unused]] constexpr DSY_SYS_STATIC_WARNING_CAT(static_warning,__LINE__)() { _( std::integral_constant<bool, (cond)>() ); } \
    }




/** 
 * @nodoc DSY_SYS_DEPRECATED_MACRO(macro_name, msg).
 * Issue a buildtime deprecation warning for macro named "macro_name", printing message "msg" (if supported)
 * This macro MUST be defined AFTER "macro_name"'s definition. Otherwise, there will be portability issues (buildtime error with the clang compiler for e.g.) 
 * Usage example:
 *  #define SomeMacroNowDeprecated   ...
 *  DSY_SYS_DEPRECATED_MACRO(SomeMacroNowDeprecated, "<message>")
 */

#if defined(__clang__)  // DSY_SYS_DEPRECATED_MACRO
/** @nodoc */
#define DSY_SYS_DEPRECATED_MACRO_STR(x) #x

// Support macro deprecation: #pragma clang deprecated
// cf. https://reviews.llvm.org/rG26c695b7893071d5e69afbaa70c4850ab2e468be 
// Trick to avoid potential expansion of argument "macro_name": concat with empty token
/** @nodoc */
#define DSY_SYS_DEPRECATED_MACRO(macro_name, message, ...) \
    _Pragma("clang diagnostic push")\
    _Pragma("clang diagnostic ignored  \"-Wunknown-pragmas\"")\
    _Pragma(DSY_SYS_DEPRECATED_MACRO_STR(clang deprecated(macro_name##__VA_ARGS__, message)))\
    _Pragma("clang diagnostic pop")

#elif defined(_MSC_VER)

// cf. https://docs.microsoft.com/en-us/cpp/preprocessor/deprecated-c-cpp?view=msvc-170 
// The Windows version does not support a custom message
/** @nodoc */
#define DSY_SYS_DEPRECATED_MACRO(macro_name, message) \
    __pragma(deprecated(#macro_name))

// #elif || defined(__GNUC__ )
#else
/** @nodoc Unsupported compiler */
#define DSY_SYS_DEPRECATED_MACRO(entity, message)
#endif  // DSY_SYS_DEPRECATED_MACRO


#endif  // DSYSysMacForBuildtimeWarning_H
