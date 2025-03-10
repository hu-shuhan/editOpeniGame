#ifndef XHMACROS_H
#define XHMACROS_H  42500

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//-----------------------------------------------------------------------------
// Abstract: Definition of error handling macros
//-----------------------------------------------------------------------------
// Usage:    Please refer to the manual for the description of CATTry, CATCatch,
//           CATCatchOthers, CATEndTry, CATThrow and CATRethrow
//-----------------------------------------------------------------------------
// Note:     The implementation of the above macros can use directly the
//           C++ try, catch, throw if the compiler supports it. However,
//           it is advised not to mix code compiled with different
//           implementations
//-----------------------------------------------------------------------------
#include "CATError.h"
#include "CATXHContext.h"   // CATXHContextCxx

#if defined(__clang__)
/** @nodoc clang may pretend to be another compiler, so test it first
__has_feature is expected to be defined */
# if __has_feature(cxx_exceptions)
/** @nodoc */
#define CATXH_CXX_SUPPORT 1
# endif
#elif defined(__GNUC__) && defined(__EXCEPTIONS)
/** @nodoc */
#define CATXH_CXX_SUPPORT 1
#elif defined(_MSC_VER) && defined(_CPPUNWIND)
/** @nodoc */
#define CATXH_CXX_SUPPORT 1
#endif

#if !defined(CATXH_CXX_SUPPORT)
/** @nodoc */
#define CATXH_CXX_SUPPORT 0
#endif


#if defined(_MSC_VER)
/** @nodoc Visual C++ options: /W4 /analyze */
#define CATXH_VAR_SUPPRESS_WARN(VarName, /*DeclareInstruction*/...)\
    __pragma(warning( push ))\
    /* 4456: cf. /W4 warning shadows local variable */\
    /* 6246: cf. /analyze warning shadows local variable */\
    __pragma(warning(disable : 4456 6246))\
    __VA_ARGS__\
    __pragma(warning( pop ))\
    /* Suppress warning unused variable */\
    (void)sizeof(VarName)
#else
/** @nodoc GCC/Clang options: -Wall -Wextra -Wshadow (neither -W nor -Wall enables -Wshadow) */
#define CATXH_VAR_SUPPRESS_WARN(VarName, /*DeclareInstruction*/...)\
    _Pragma("GCC diagnostic push")\
    /* Suppress warning shadows local variable */\
    _Pragma("GCC diagnostic ignored \"-Wshadow\"")\
    __VA_ARGS__\
    _Pragma("GCC diagnostic pop")\
    /* Suppress warning unused variable */\
    (void)sizeof(VarName)
#endif

/** @nodoc required to take the address of variables which type has an overload for the & operator */
template< class T >
T* CATXH_AddressOf(T& arg) 
{
    // cf. std::addressof (but avoids the inclusion of the header <memory>)
    return reinterpret_cast<T*>(
               &const_cast<char&>(
                  reinterpret_cast<const volatile char&>(arg)));
}


/** @nodoc */
namespace CATErrorMacrosTrue
{
    // Used to detect context at compile-time while avoiding polluting the stack with local variables
    [[maybe_unused]] constexpr bool __CATCatchBlockType__ = true;
}

/** @nodoc */
namespace CATErrorMacrosFalse
{
    // Used to detect context at compile-time while avoiding polluting the stack with local variables
    [[maybe_unused]] constexpr bool __CATCatchBlockType__ = false;
}


#if !defined(__CATXH_CXX_SUPPORT_DISABLE_CHECK)
    /** @nodoc */
    #define __CATXH_CXX_SUPPORT_CHECK   static_assert(CATXH_CXX_SUPPORT, "Requires C++ Exception Handling to be enabled!")
#else
    /** @nodoc */
    #define __CATXH_CXX_SUPPORT_CHECK   do { continue; } while(false) /* To prevent any compiler warning */
#endif


#if !defined(CATXH_CGMTkCompat_Disable) && defined(_MOBILE_SOURCE) && !defined(CATXH_CGMTkCompat_Enable)
#define CATXH_CGMTkCompat_Disable
#endif

#if defined(CATXH_CGMTkCompat_Disable)
/** @nodoc */
#define CGMTkCompatTry
/** @nodoc */
#define CGMTkCompatEndTry
#else   // CATXH_CGMTkCompat_Disable
/** @nodoc */
#define CGMTkCompatTry  try {\
                            CATXH_VAR_SUPPRESS_WARN(_cgmTkCompat_, CGMTkCompatCtx _cgmTkCompat_;);

/** @nodoc */
#define CGMTkCompatEndTry   } catch(CATException<CATError> & _spErr_) {\
                                CGMTkCompatCtx::Rethrow(_spErr_);\
                            }
#endif  // CATXH_CGMTkCompat_Disable


//
// eXception Handling macros
//

/**
 * Begin of block CNEXT exceptions.
 * CATTry is similar to the C++ 'try' keyword, but is necessary to handle CNEXT exceptions through CATCatch/CATCatchOthers.
 * @see CATEndTry
 */
#define CATTry                      CGMTkCompatTry\
                                    {\
                                        __CATXH_CXX_SUPPORT_CHECK;\
                                        try {\

/**
 * CNEXT catch exception handler.
 * @param errclass
 *   CATError-derived type to catch
 * @param errobj
 *   AddRef'd pointer to an object of type 'errclass'
 */
#define CATCatch(errclass,errobj)       } catch (CATException<errclass> & _spErr_##errobj) {\
                                            errclass *errobj = static_cast<errclass *>(CATXHContextCxx::CatchBlockProlog(/*&_xhctx_*/nullptr,0,_spErr_##errobj));\
                                            CATXH_VAR_SUPPRESS_WARN(_ctxtCatch_, _err_catcatch_info const _ctxtCatch_ = { CATXH_AddressOf(_spErr_##errobj), errobj };);\
                                            using CATErrorMacrosFalse::__CATCatchBlockType__;

/**
 * CNEXT catch-all exception handler.
 */
#define CATCatchOthers                  } catch (...) {\
                                            CATXH_VAR_SUPPRESS_WARN(_ctxtCatch_, CATError *const _ctxtCatch_ = CATXHContextCxx::CatchBlockProlog(/*&_xhctx_*/nullptr,0););\
                                            using CATErrorMacrosTrue::__CATCatchBlockType__;
                                        

/**
 * End of block CNEXT exceptions.
 * @see CATTry
 */
#define CATEndTry                       }\
                                    }\
                                    CGMTkCompatEndTry

// Beware that errobj expression can have side-effects (a new exp. for ex.)
// and thus must evaled only ONCE!
/** @nodoc must support being used as a comma operator operand */
#define CATThrow(errobj)                ([]{ __CATXH_CXX_SUPPORT_CHECK; }(), (errobj)->CATError::Throw(__FILE__,__LINE__))

/** @nodoc */
#define CATRethrow                      (CATXHContextCxx::DoRethrow(/*&_xhctx_*/nullptr,0,_ctxtCatch_))



/** nodoc NOT(CATXH_CXX_SUPPORT) => NOT(CATXH_CXX_UNWINDING_ON_CATTHROW) */
#define CATXH_CXX_UNWINDING_ON_CATTHROW     CATXH_CXX_SUPPORT

/** @nodoc compatibility macro */
#define CATXH_ENSURE_DESTRUCTION(var)               __CATXH_CXX_SUPPORT_CHECK
/** @nodoc compatibility macro */
#define CATXH_ENSURE_DESTRUCTION_EXPL(class, var)   CATXH_ENSURE_DESTRUCTION(var)


/** @nodoc */
struct _err_catcatch_info {
    CATException<CATError> *m_pExcObj;
    CATError *m_pErr;
};


/** @nodoc */
struct CGMTkCompatCtx
{
    ExportedByJS0ERROR  CGMTkCompatCtx() noexcept;
    ExportedByJS0ERROR ~CGMTkCompatCtx() noexcept;
    [[noreturn]] ExportedByJS0ERROR static void Rethrow(CATException<CATError> &);
};


#endif  // XHMACROS_H
