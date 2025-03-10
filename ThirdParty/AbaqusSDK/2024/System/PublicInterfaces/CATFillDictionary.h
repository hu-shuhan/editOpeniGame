#include "CATFillDictionary_required.h"
#ifndef __CATFillDictionary
#define __CATFillDictionary
/* -*-C++-*- */
/* COPYRIGHT DASSAULT SYSTEMES 1997                                      */
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
/*=======================================================================*/

// this class is used to put information into the dictionary at runtime */

#include <stdio.h>
#include "CATBaseUnknown.h"
#include "CATCreateClassInstance.h"
#include "CATMetaClass.h"  // CATSysCreationFunc, CATSysConditionFunc

/* The type of factory functions */
typedef CATBaseUnknown* (*OMFactoryFunctionType) ();

/**
 * Do not use this class. For internal use only.
 * Class used to fill in the interface dictionary at runtime.
 */
class CATFillDictionary
{
    struct CATSysConditionFunc_t
    {
        CATSysConditionFunc m_pFunc;
        
        CATSysConditionFunc_t(CATSysConditionFunc ipFunc) : m_pFunc(ipFunc) {}
        CATSysConditionFunc_t(decltype(nullptr)) : m_pFunc(nullptr) {}
        
        template<typename PFuncCondition>
        CATSysConditionFunc_t(PFuncCondition ipFunc) : m_pFunc((CATSysConditionFunc)ipFunc) {
            // To enable better diagnostics in case of invalid input type (cf. macro CATImplementCondition...), 
            // accept to construct with all types to be able to validate:
            static_assert(IsOMCondition<PFuncCondition>::value, 
            "[CATFillDictionary] Invalid condition function argument! It's type should match 'CATSysConditionFunc'");
        }
    };
    
    typedef void *(*CATSysCreatCommandFunc)(void *);

public:
      // to put class name, interface name and functions in the dictionary
/**
 * Constructs a CATFillDictionary instance by means of a class name that
 * implements an interface.
 * @param iClass      Class implementing the interface
 * @param iInterface  Interface
 * @param iCreation   Creation function
 * @param iCondition  Conditional function
 */
 CATFillDictionary(const char *iClass, const char *inter, 
    CATSysCreationFunc iCreation, CATSysConditionFunc_t ipFuncCondition = nullptr) {
    Register(iClass, inter, iCreation, ipFuncCondition.m_pFunc);
 }
 
      // to put a meta object in the dictionary
/**
 * Constructs a CATFillDictionary instance by means of a class GUID and its meta object.
 * @param iClass      GUID of the class
 * @param iMetaObject Meta object of the class
 */
 CATFillDictionary(const GUID & iClass, const CATMetaClass * iMetaObject) {
    Register(iClass, iMetaObject);
 }

/**
 * Constructs a CATFillDictionary instance by means of the meta objects of
 * both the interface class and the class that implements it.
 * @param iClassMetaObject      Meta object of the class implementing the interface.
 * @param iInterfaceMetaObject  Meta object of the interface class.
 * @param iCreation   Creation function.   
 * @param iCondition  Conditional function.
 * @param iInheritanceForInterfaces  Inheritance For Interfaces.
 */
 CATFillDictionary( const CATMetaClass * iClassMetaObject,
                    const CATMetaClass * iInterfaceMetaObject,
                    CATSysCreationFunc iCreation,
                    CATSysConditionFunc_t iCondition = nullptr,
                    int iInheritanceForInterfaces = 1) {
    Register(const_cast<CATMetaClass*>(iClassMetaObject), const_cast<CATMetaClass*>(iInterfaceMetaObject), 
             iCreation, iCondition.m_pFunc, iInheritanceForInterfaces);
 }
 
      // to put commands in the dictionary
/**
 * Constructs a CATFillDictionary instance by means of a class that implements an
 * interface and a creation function.
 * @param iName Class implementing an interface.      
 * @param iCreation Creation function.
 */
 template<typename _Ty, typename _Tz>
 CATFillDictionary(const char * iname, _Tz*(*iCreation)(_Ty*)) {
    Register(iname, reinterpret_cast<CATSysCreatCommandFunc>(iCreation));
 }
 
/**
 * Constructs a CATFillDictionary instance by means of a class that implements an
 * interface inside a shared library.
 * @param iName Class that supports the interface.      
 * @param iInterface Interface name.
 * @param iLibrary Library containing the interface implementation code.
 */
 CATFillDictionary(const char * iname, const char * iinterface, const char * ilibrary) {
    Register(iname, iinterface, ilibrary);
 }
      
/**
 * Constructs a CATFillDictionary instance by means of an extension class
 * that implements an interface for an implementation.
 * @param iExtensionMetaObject Meta object of the extension.
 * @param iInterfaceMetaObject Meta object of the interface.
 * @param iImplementation Implementation name.
 * @param iCreation Creation function.
 */
 CATFillDictionary( const CATMetaClass * iExtensionMetaObject,
                    const CATMetaClass * iInterfaceMetaObject,
                    const char * iImplementation,
                    CATSysCreationFunc iCreation) {
    Register(const_cast<CATMetaClass*>(iExtensionMetaObject), iInterfaceMetaObject, iImplementation, iCreation);
 }

/**
 * @nodoc 
 * Fills the functions dictionary with a function pointer and its keyname and function type. 
 * @param iKeyName          : the keyname of the registered function 
 * @param iPtrFunct         : the pointer on the function
 * @param iFunctionTypeName : the type of the registered function
 * @return 0 if the call has succeeded else 1
 */
 ExportedByJS0CORBA
 static char RegisterFunction(const char * iKeyName, const void * iPtrFunct, const char * iFunctionTypeName);

/**
 * @nodoc 
 * Fills the functions dictionary with a function creation pointer and its keyname. 
 * For more information about creation functions, see CATCreateClassInstance.h
 * @param iKeyName          : the keyname of the registered function 
 * @param iPtrFunct         : the pointer on the creation function 
 * @return 0 if the call has succeeded else 1
 */
 ExportedByJS0CORBA
 static char RegisterFunctionCreation(const char * iKeyName, OMFactoryFunctionType iPtrFunct);
 
private:

    // Exported methods are static to avoid passing "this" (useless with since no data member)
    ExportedByJS0CORBA 
    static bool Register(const char *iClass, const char *inter, CATSysCreationFunc iCreation, CATSysConditionFunc ipFuncCondition);
    ExportedByJS0CORBA
    static bool Register(const GUID & iClass, const CATMetaClass * iMetaObject);
    ExportedByJS0CORBA
    static bool Register(CATMetaClass * iClassMetaObject, CATMetaClass * iInterfaceMetaObject,
                    CATSysCreationFunc iCreation, CATSysConditionFunc iCondition, int iInheritanceForInterfaces);
    ExportedByJS0CORBA
    static bool Register(const char * iname, CATSysCreatCommandFunc iCreation);
    ExportedByJS0CORBA
    static bool Register(const char * iname, const char * iinterface, const char * ilibrary);
    ExportedByJS0CORBA
    static bool Register(CATMetaClass * iExtensionMetaObject, const CATMetaClass * iInterfaceMetaObject,
                    const char * iImplementation, CATSysCreationFunc iCreation);
    
    
    template<typename Callable>
    struct IsOMCondition : std::false_type {};
    
    template<typename _Ty>
    struct IsOMCondition<int(*)(_Ty*, GUID const&)>  // cf. CATSysConditionFunc
      : public dsy::internal::is_accessible_base_of<CATBaseUnknown,std::remove_const_t<_Ty>> { };
};
#endif
