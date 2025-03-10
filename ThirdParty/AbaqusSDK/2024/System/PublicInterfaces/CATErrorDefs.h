#ifndef CATERRORDEFS_H
#define CATERRORDEFS_H  42500

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//-----------------------------------------------------------------------------
// Abstract:    Macro definitions for the Error Handling system
//-----------------------------------------------------------------------------

#include "JS0ERROR.h"
#include "CATMacForIUnknown.h"
#include "CATMetaClass.h"

/**
 * Error identifier.
 */
typedef unsigned int CATErrorId;

/** @nodoc */
typedef void (*CATTerminateFunction) ();

//
// Declare an error class: should be included in the class public part
//
/** @nodoc */
#define CATDeclareErrorClass(thisClass,baseClass)   \
    CATDeclareClass;                                \
    thisClass(CATErrorId errId);                    \
    thisClass(CATErrorId errId, const char *msgId,  \
          const char *catalog = NULL);              \
    CAT_DECLARE_ERR_CLASS(thisClass,baseClass)      \
    /*virtual*/ void GetErrorTable (unsigned int *, const CATErrDsc **) override;\
    static const unsigned int errorTableLength;     \
    static const CATErrDsc errorTable[];

/**
 * Creates an error class.
 * <br><b>Role</b>: This macro creates all the class method and data member
 * declaration.
 * It must be inserted in the error class header file, after the public keyword.
 * @param thisClass
 *   The name of the error class to create
 * @param baseClass
 *   The name of the error class base class.
 *   <br><b>Legal values</b>: This class must derive from CATError
 *   and the created error class must derive from it
 */
#define CATDeclareError(thisClass,baseClass)                \
    CATDeclareClass;                                        \
    thisClass(CATErrorId errId);                            \
    thisClass(CATErrorId errId, const char *msgId,          \
         const char *catalog = NULL);                       \
    thisClass( const char *msgId,const char *catalog );     \
    CAT_DECLARE_ERR_CLASS(thisClass,baseClass)              \
    /*virtual*/ void GetErrorTable (unsigned int *, const CATErrDsc **) override;\
    static const unsigned int errorTableLength;             \
    static const CATErrDsc errorTable[];
//
// Implement the methods declared in the macro above. Should be included
// in a source file
//
/** @nodoc */
#define CATImplementErrorClass(thisClass,baseClass)                 \
    CATImplementClass(thisClass,Implementation,baseClass,CATNull);  \
    CAT_IMPLEMENT_CTOR(thisClass,baseClass)                         \
    CAT_IMPLEMENT_SETUP(thisClass)                                  \
    CAT_IMPLEMENT_CLONESELF(thisClass)                              \
    CAT_IMPLEMENT_GETMESSAGETABLE(thisClass)                        \
    CAT_IMPLEMENT_RAISE(thisClass)

/**
 * Creates an error class.
 * <br><b>Role</b>: This macro creates all the class method bodies.
 * It must be inserted in the error class source file.
 * @param thisClass
 *   The name of the error class to create
 * @param baseClass
 *   The name of the error class base class.
 *   <br><b>Legal values</b>: This class must derive from CATError
 *   and the created error class must derive from it
 */
#define CATImplementError(thisClass,baseClass)                      \
    CATImplementClass(thisClass,Implementation,baseClass,CATNull);  \
    CAT_IMPLEMENT_CTOR_BIS(thisClass,baseClass)                     \
    CAT_IMPLEMENT_SETUP(thisClass)                                  \
    CAT_IMPLEMENT_CLONESELF(thisClass)                              \
    CAT_IMPLEMENT_GETMESSAGETABLE(thisClass)                        \
    CAT_IMPLEMENT_RAISE(thisClass)
    

//
// Declarations for a base Error Class (only used by CATError)
//
/** @nodoc */
#define CATDeclareBaseErrorClass(thisClass)         \
    CATDeclareClass;                                \
    thisClass *Setup (const char *, int line);      \
    thisClass *Self () { return this; }             \
    virtual CATError *CloneSelf ();                 \
    [[noreturn]] virtual void raise();              \
    [[noreturn]] virtual void raise_ptr();          \
    virtual void GetErrorTable (unsigned int *, const CATErrDsc **) = 0;

//
// Common declarations for all Error Classes
//
/** @nodoc */
#define CAT_DECLARE_ERR_CLASS(thisClass,baseClass)\
  private:                                          \
    using base_declared = baseClass;                \
  public:                                           \
    thisClass *Setup (const char *, int line);      \
    thisClass *Self () { return this; }             \
    /*virtual*/ CATError *CloneSelf () override;    \
    CAT_DECLARE_RAISE(thisClass,baseClass)          \
    /* ThrowSmartPtr: for native exception model, */\
    /* but declared in all modes as a mean to  */   \
    /* validate the "baseClass" macro argument */   \
    struct ThrowSmartPtr : public baseClass::ThrowSmartPtr  \
    {                                                       \
        /* Reuse parent class constructors */               \
        using baseClass::ThrowSmartPtr::ThrowSmartPtr;      \
        thisClass* get_CATError() const noexcept {          \
            return static_cast<thisClass*>(this->baseClass::ThrowSmartPtr::get_CATError());\
        }\
    };

//
// Implement methods for a base error class
//
/** @nodoc */
#define CATImplementBaseErrorClass(thisClass)                   \
    CATImplementClass(thisClass,Implementation,CATNull,CATNull);\
    CAT_IMPLEMENT_RAISE(thisClass)

//
// For use by msgp only
//
/** @nodoc */
#define CATDefineErrorTable(errClass,numErr)                \
    const unsigned int errClass::errorTableLength = numErr; \
    const CATErrDsc errClass::errorTable[numErr]

//
// If not using msg
//
/**
 * Enables an error class for NLS messages.
 * @param errClass
 *   The name of the error class to create
 */
#define CATImplementNLSErrorClass(errClass)                 \
    const unsigned int errClass::errorTableLength = 0;      \
    const CATErrDsc errClass::errorTable[1] { {0} };

//
// Constructor definition
//
/** @nodoc */
#define CAT_IMPLEMENT_CTOR(thisClass,baseClass)                                 \
    thisClass::thisClass (CATErrorId errId) : baseClass (errId) {               \
        __CATERROR_DIRECT_BASE_OF_CHECK(thisClass,baseClass)                    \
    }\
    thisClass::thisClass (CATErrorId errId, const char *cat, const char *m) :   \
    baseClass (errId,cat,m) {}
/** @nodoc */
#define CAT_IMPLEMENT_CTOR_BIS(thisClass,baseClass)                             \
    thisClass::thisClass (CATErrorId errId) : baseClass (errId) {               \
        __CATERROR_DIRECT_BASE_OF_CHECK(thisClass,baseClass)                    \
    }\
    thisClass::thisClass (CATErrorId errId, const char *cat, const char *m) :   \
        baseClass (errId,cat,m) {}                                              \
    thisClass::thisClass (const char *cat, const char *m) :                     \
    baseClass (cat,m) {} 
        
                

// This method is for the native throw, to setup the error object properly
// and return it for the throw. This method cannot be inherited because
// the signature of the throw will always be the base class!
/** @nodoc */
#define CAT_IMPLEMENT_SETUP(thisClass)                          \
    thisClass *thisClass::Setup (const char *file, int line)    \
    {                                                           \
        __CATERROR_DIRECT_BASE_OF_CHECK_EX(thisClass)              \
        CATError::Setup (file, line);                           \
        return this;                                            \
    }

//
// This method is used for native exception model only.
// With this exception model, errors must be thrown by calling the 
// polymorphic method raise to avoid type slicing issues (cf. C++ standard).
//
/** @nodoc */
# define CAT_DECLARE_RAISE(thisClass,baseClass)         \
    [[noreturn]] /*virtual*/ void raise() override;     \
    [[noreturn]] /*virtual*/ void raise_ptr() override;
/** @nodoc */
# define CAT_IMPLEMENT_RAISE(thisClass)     \
    void thisClass::raise()                 \
    {                                       \
        /* ThrowSmartPtr handles the lifecycle of the error object */\
        throw thisClass::ThrowSmartPtr(this);\
    }                                       \
    void thisClass::raise_ptr()             \
    {                                       \
        throw this;                         \
    }

//
// Should be removed
//
/** @nodoc */
#define CAT_IMPLEMENT_CLONESELF(thisClass)      \
    CATError *thisClass::CloneSelf ()           \
    {                                           \
        thisClass *obj = new thisClass (*this); \
        return (CATError *) obj;                \
    }

//
// Return the error table
//
/** @nodoc */
#define CAT_IMPLEMENT_GETMESSAGETABLE(thisClass)        \
    void thisClass::GetErrorTable (unsigned int *len,   \
                       const CATErrDsc **table)         \
    {                                                   \
        *len = errorTableLength;                        \
        *table = errorTable;                            \
    }

#include <type_traits>

/** 
 * @nodoc 
 * __CATERROR_DIRECT_BASE_OF_CHECK.
 * Goal: 
 *  1) Check that CATDeclareError(Class)/CATImplementError(Class) macros's "baseClass" 2nd argument are consistent
 *     Crucial for proper NATIVE_EXCEPTION support
 *  2) Check that the "baseClass" type is indeed the direct base of "thisClass" (C++ inheritance)
 *     The CATImplementError/CATImplementErrorClass macros already enforces this by defining constructors of "thisClass" 
 *     calling their base constructor. Should "baseClass" be wrong, the compiler would emit an error such as:
 *     "illegal member initialization: 'baseClass' is not a base or member"
 *     The problem is that some DS code that does not use the CATImplementError/CATImplementErrorClass standard macros,
 *     hence the creation of __CATERROR_DIRECT_BASE_OF_CHECK_EX
 */
#define __CATERROR_DIRECT_BASE_OF_CHECK(thisClass, baseClass) \
    static_assert(std::is_same<thisClass::base_declared, baseClass>::value, "CATDeclareError macro: wrong baseClass argument!");

// __CATERROR_DIRECT_BASE_OF_CHECK_EX: try to also validate DS code that does not use CATImplementError/CATImplementErrorClass
#if defined(__CATERROR_DIRECT_BASE_OF_DISABLE_CHECK) || !defined(_LINUX_SOURCE) || defined(__aarch64__)
/** @nodoc */
# define __CATERROR_DIRECT_BASE_OF_CHECK_EX(thisClass) 
#else 
#include <tr2/type_traits> // std::tr2::direct_bases - Not standard, used only for validation purposes!!! Available on Linux GCC for now at least

/** @nodoc Validate the baseClass argument of the CATDeclareError macro, which is very important for NATIVE_EXCEPTION support*/
# define __CATERROR_DIRECT_BASE_OF_CHECK_EX(thisClass)    \
    static_assert([]() constexpr -> bool {             \
        using Base = thisClass::base_declared;         \
        if constexpr (!std::tr2::direct_bases<thisClass>::type::empty::value) {\
            return std::is_same<typename std::tr2::direct_bases<thisClass>::type::first::type, Base>::value;\
        } else {\
            return false;\
        }\
    }(), "CATDeclareError macro: wrong baseClass argument!");
#endif

#endif
