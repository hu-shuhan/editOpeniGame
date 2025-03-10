#include "CATBaseUnknown_required.h"
#ifndef __CATBaseUnknown
#define __CATBaseUnknown    42601

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */


#include "JS0CORBA.h"
#include "IDispatch.h"
#include "CATOMTypeOfClass.h"  // TypeOfClass
#include <stdio.h>
#include <type_traits>


#if !defined(CATMetaClassStaticCheck_Disable)
/** @nodoc The macro CATMetaClassStaticCheck defines internal functions 
 * used only for static compile-time checks:
 *  - ALL templates, to avoid useless exports 
 *  - names chosen to reflect the fact that they are internal (capital letters, prefix close to the end of the alphabet)
 */
#define CATMetaClassStaticCheck \
private:\
    /* Method used only to extract the type of the current class at compile time (from its return type) */  \
    template<typename _Ty> /* template to prevent useless symbol export */                                  \
    auto SYS_RET_CLASS_TYPE() /*-> decltype(this)*/ {                                                       \
        /* Discourage calling this method by returning "nullptr" instead of "this" */                       \
        return static_cast<decltype(this)>(nullptr);                                                        \
    }\
public:\
    template<typename _Ty>                                                                                      \
    static constexpr bool SYS_OM_META_CLASS_CHECK() {                                                           \
        /* Returns true if the method "SYS_RET_CLASS_TYPE" is defined in the class "_Ty"  */                    \
        /* (in which case other CATMetaClass-related methods are expected to be defined too), false otherwise */\
        return std::is_same<_Ty,                                                                                \
            /* Extract the type of the first class which defines the "SYS_RET_CLASS_TYPE" method */             \
            /* in the inheritance tree of "_Ty" (including itself)  */                                          \
            typename std::remove_pointer<decltype(std::declval<_Ty>().template SYS_RET_CLASS_TYPE<void>())>::type \
            >::value;\
    } 
#else
/** @nodoc */
#define CATMetaClassStaticCheck \
public: 
#endif


/**
 * @nodoc
 */
class CATSysChangeComponentStateContext;

/**
 * GUID of the class CATBaseUnknown.
 */
extern ExportedByJS0CORBA const GUID CLSID_CATBaseUnknown;
/**
 * IID of the interface CATBaseUnknown.
 */
#define IID_CATBaseUnknown CLSID_CATBaseUnknown

/**
 * @nodoc
 * Size of a GUID.
 */
extern ExportedByJS0CORBA const int GUIDsize;

class CATMetaClass;

class CATBaseUnknown;

/**
 * @nodoc
 * For migration compatibility
 */
#define CATExtendable CATBaseUnknown
/**
 * @nodoc
 */
#define CATDataExtendable CATBaseUnknown
/**
 * @nodoc
 */
#define CATInterfaceObject CATBaseUnknown
/**
 * @nodoc
 */
typedef CATBaseUnknown* CATInterfaceObject_ptr;
/**
 * Type to define a pointer on a CATBaseUnknown object.
 */
typedef CATBaseUnknown* CATBaseUnknown_ptr;
/**
 * @nodoc
 */
#define GiveMetaObject GetMetaObject
/**
 * @nodoc
 */
#define AddRefForPtr   AddRef
/**
 * @nodoc
 */
#define ReleaseForPtr  Release
/**
 * @nodoc
 */
#define CATListValCATInterfaceObject_var CATListValCATBaseUnknown_var
/**
 * @nodoc
 */
#define _SEQUENCE_CATInterfaceObject_ptr _SEQUENCE_CATBaseUnknown_ptr
/**
 * @nodoc
 */
#define CATListPtrCATInterfaceObject     CATListPtrCATBaseUnknown

/**
 * @nodoc
 * to chain the extensions on the implementation classes
 */
struct DataForImpl;
/**
 * @nodoc
 */
struct ChainExtension;
class CATSysWeakRef;
/**
 * @nodoc
 */
class CATDelegation;

/**
 * Type to define a class by its name.
 */
typedef const char *CATClassId;
/**
 * Type to define a class by its name.
 */
typedef const char *CATIdent;

/**
 * @nodoc
 * data for extension/TIE/implementation
 */
union TypeOfData
{
   ChainExtension * ForImplementation;
   CATBaseUnknown * ForExtension;
   CATBaseUnknown * ForTIE;
};

/**
 * @nodoc
 * Chained list containing the list of the objects that implement a delegated
 * interface for the current instance.
 * <dl>
 * <dt>DelegatedInterface <dd>pointer on the object that implement the 
 *                  delegated interface.
 * <dt>next               <dd>pointer on next structure, last pointer is NULL.
 * </dl>
 */
struct ListOfDelegated
{
   CATBaseUnknown  *DelegatedInterface;
   ListOfDelegated *next;
};

/**
 * @nodoc
 */
struct info_dic;

/**
 * Base class for creating interfaces and for implementing interfaces.
 * <b>Role</b>: CATBaseUnknown supplies the infrastructure and the basic mechanisms
 * to create interface abstract classes and to manage interface pointers.
 * It is also the base class for classes which implements interfaces
 * and for their extension classes because it supplies the code for 
 * the interface methods @href #QueryInterface, @href #AddRef and @href #Release.
 */
class ExportedByJS0CORBA CATBaseUnknown : public IDispatch
{
    public:
        /**
         * Retrieves a pointer to a given interface.
         * @param iIID
         *   The interface identifier for which a pointer is requested.
         * @param oPPV
         *   The address where the returned pointer to the interface is located.
         * @return
         *   An HRESULT value. 
         *   <br><b>Legal values</b>:
         *   <dl>
         *     <dt>S_OK</dt>
         *     <dd>The query succeeds.</dd>
         *     <dt>E_NOINTERFACE </dt>
         *     <dd>The interface does not exist.</dd>
         *     <dt>E_FAIL </dt>
         *     <dd>The object is not valid.</dd>
         *     <dt>E_OUTOFMEMORY </dt>
         *     <dd>One memory allocation fails</bb>
         *     <dt>E_UNEXPECTED </dt>
         *     <dd>The query fails for any other reason</dd>
         *   </dl>
         */
        /*virtual*/ HRESULT __stdcall QueryInterface(const IID &iIID, void **oPPV) override;
        /**
         * Increments the reference count for the given interface. 
         * @return The reference count value.
         * <br>This information is meant to be used for diagnostic/testing purposes only, because, in certain situations, the value may be unstable.
         */
        /*virtual*/ ULONG   __stdcall AddRef() override;
        /**
         * Decrements the reference count for the given interface. 
         * @return The reference count value.
         * <br>This information is meant to be used for diagnostic/testing purposes only, because, in certain situations, the value may be unstable.
         */
        /*virtual*/ ULONG   __stdcall Release() override;

        /**
         * @nodoc
         * IDispatch methods.
         * @return E_NOTIMPL
         */
        /*virtual*/ HRESULT __stdcall GetTypeInfoCount(unsigned int *oNbOfTypeInfo) override;
        /**
         * @nodoc
         */
        /*virtual*/ HRESULT __stdcall GetTypeInfo(unsigned int iIndex, ULONG iLangId, ITypeInfo **oPtTypeInfo) override;
        /**
         * @nodoc
         */
        /*virtual*/ HRESULT __stdcall GetIDsOfNames(const IID &forFutur, CATWCHAR_T **iArrayOfNames, unsigned int iNbNames, ULONG iLangId, LONG *oArrayOflong) override;
        /**
         * @nodoc
         */
        /*virtual*/ HRESULT __stdcall Invoke(LONG ilongMember, const IID &forFutur,ULONG iLangId, unsigned short iFlags, DISPPARAMS *iPdisparams, VARIANT *oPvaresult, EXCEPINFO *oPexcepinfo, unsigned int *oPuArgErr) override;

        /**
         * @nodoc
         * Retrieves a pointer to a given interface.
         * <b>Warning</b>: This is a static method.
         * This method does not comply with the Microsoft(R) COM's QueryInterface
         * signature and is used for marshalling purpose.
         * @param cid 
         *   The class identifier of the object implementing the interface.
         * @param iid 
         *   The interface identifier for which a pointer is requested.
         * @param ppv
         *   The address where the returned pointer to the interface is located.
         * @return
         *   <dl>
         *   <dt><tt>S_OK</tt>          <dd>if the query succeeds
         *   <dt><tt>E_NOINTERFACE</tt> <dd>if the interface does not exist
         *   </dl> 
         */
        static  HRESULT __stdcall QueryInterface(const CLSID &cid,
                       const IID &iid,
                       void **ppv);
        /**
         * @nodoc
         * Retrieves a pointer to a given interface.
         * <b>Warning</b>: This is a static method.
         * This method does not comply with the Microsoft(R) COM's QueryInterface
         * signature and is used for marshalling purpose.
         * @param iname
         *   The class name passed as a pointer to a character string.
         * @param iid 
         *   The interface identifier for which a pointer is requested.
         * @param ppv
         *   The address where the returned pointer to the interface is located.
         * @return
         *   <dl>
         *   <dt><tt>S_OK</tt>          <dd>if the query succeeds
         *   <dt><tt>E_UNEXPECTED</tt>  <dd>if the interface call meta object does not exist
         *   <dt><tt>E_NOINTERFACE</tt> <dd>if the interface does not exist
         *   <dt><tt>E_FAIL</tt>        <dd>if <tt>iname</tt> is <tt>IUnknown</tt>'s GUID.
         *   </dl>
         */
        static  HRESULT __stdcall QueryInterface(const char *iname,
                       const IID &iid,
                       void **ppv);

        /**
         * Constructs a empty instance. 
         */
        CATBaseUnknown();
        /**
         * Copy Constructor.
         * @param iObj The CATBaseUnknown instance to copy
         */
        CATBaseUnknown(const CATBaseUnknown &iObj);
        virtual ~CATBaseUnknown();

        /**
         * @nodoc
         * Returns a pointer to the class meta object.
         * There is one meta object per class.
         */
        virtual CATMetaClass * __stdcall GetMetaObject() const;
        /**
         * @nodoc
         * Returns a pointer to the class meta object.
         * There is one meta object per class.
         */
        static  CATMetaClass * __stdcall MetaObject();

        /**
         * @nodoc
         * Returns the class name for which the given object is an instance.
         */
        virtual const char * IsA() const;
        /**
         * Determines whether the given object's class derives from a given class.
         * @param iName
         *   The class name from which the given object's class is supposed to derive.
         * @return 0 if the given object's class doesn't derive from <tt>iName</tt>
         * and a non null value otherwise.
         * @return
         * <br><b>Legal values</b>:
         *   <dl>
         *     <dt>0 </dt>
         *        <dd>Current onject does not derive from iName.</dd>
         *     <dt>1 </dt>
         *        <dd>Otherwise.</dd>
         *   </dl>
         */
        virtual int IsAKindOf(const char *iName) const;
        /**
         * Returns the class name for which the given object is an instance.
         */
        static  const char *  __stdcall ClassName();
        /**
         * @nodoc
         * Returns the class identifier for which the given object is an instance.
         */
        static  const CLSID & __stdcall ClassId();

        /**
         * @nodoc
         * Returns a pointer to the object that implements the given interface.
         * @param iFlag
         *   Reserved for TIE management.
         */
        virtual CATBaseUnknown * __stdcall GetImpl(int iFlag = 0) const;

        /**
         * @nodoc
         * Adds an extension to an object that already implements an interface.
         * @param iExtension
         *   The pointer to the object created as an extension to add to the
         *   given object. 
         * @return 0 if the extension is added successfully and 1 otherwise.
         */
        int __stdcall AddExtension(CATBaseUnknown *iExtension, int internal_use = 0);

        /**
         * @nodoc
         * Removes an extension from an object that already implements interfaces.
         * @param iExtension
         *   The extension to remove from the given object.
         * @return 0 if the extension is removed successfully and 1 otherwise.
         */
        int __stdcall RemoveExtension(CATBaseUnknown *iExtension);

        /**
         * @nodoc
         * Delegates an interface to another class.
         * @param iObject
         *   The pointer to the object that implements the delegated interface.
         * @return 0 if the object is added successfully and 1 otherwise.
         */
        int __stdcall AddDelegatedInterface(CATBaseUnknown *iObject);
        /**
         * @nodoc
         * Removes a delegation to another class.
         * @param iObject
         *   The pointer to the object that should be removed.
         * @return 0 if the object is removed successfully and 1 otherwise.
         */
        int __stdcall RemoveDelegatedInterface(CATBaseUnknown *iObject,
                     int internal_use = 0);
        /**
         * @nodoc
         * Returns the list of objects that implement a delegated interface for
         * the current instance.
         * @return a chained list of these objects.
         */
        ListOfDelegated * __stdcall ListDelegatedInterface();

        /**
         * @nodoc
         * Test if the real object pointed by an interface is deleted.
         * @return 1 if the object is deleted and 0 otherwise.
         */
        virtual int IsNull() const;

        /**         
         * Compares implementations pointed by interfaces.
         * @param iobject
         *   The second object to be compared to.
         * @return 1 if the implementations are identical.
         */
        virtual int IsEqual(const CATBaseUnknown *iobject) const;

        /**
         * @nodoc
         * Same as QueryInterface but here classes names are used.
         * Try not to use this call because of performances.
         * @param iInterface
         *   The name of the searched interface.
         * @return a pointer on the interface.
         */
        virtual CATBaseUnknown *QueryInterface(CATClassId iInterface) const;

        /**
         * @nodoc
         * Same as static QueryInterface but here classes and interfaces names are used.
         * Try not to use this call because of performances.
         * @param iName
         *   The name of the implementation class.
         * @param iInterface
         *   The name of the searched interface.
         * @return a pointer on the interface.
         */
        static  CATBaseUnknown *QueryInterface(CATClassId iName,
                     CATClassId iInterface);

        /**         
         * @nodoc
         * Specifies the state of the component.
         * <br><b>Role:</b>
         * @param Activated 
         *   The component is activated
         * @param Garbaged 
         *   The component is garbaged 
         * @param Destoyed 
         *   The component is destroyed
         * @see #ChangeComponentState, #InvokeChangeComponentState
         */
        enum ComponentState{Activated,Garbaged,Destroyed};              

        /** 
         * @nodoc
         * When the state’s component is changing.
         * <br><b>Role:</b> This method can be overwritten only by a data extension of a component
         * to execute some tasks when the component is changing of state.
         * This method must not be called directly. It is invoked
         * by @href #InvokeChangeComponentState.
         * This method will have to propagate the call to its super class.
         * @param iFromState
         *   The initial state of the component.
         * @param iToState
         *   The final state of the component.
         * @param iContext
         *   Useless, set it to null.
         */
        virtual HRESULT ChangeComponentState(    ComponentState iFromState,
                                            ComponentState iToState,
                                            const CATSysChangeComponentStateContext * iContext);

        /**
         * @nodoc
         * Invokes the change component state.
         * <br><b>Role:</b>This method
         * invokes the @href #ChangeComponentState method on all  Data Extension of the 
         * component. This member must only be called inside the implementation component.
         * @param iFromState
         *   The initial state of the component.
         * @param iToState
         *   The final state of the component.
         * @param iContext
         *   The context in which the action is performed.
         */
        HRESULT InvokeChangeComponentState(    ComponentState iFromState,
                                    ComponentState iToState,
                                    const CATSysChangeComponentStateContext * iContext);

        /**
         * @nodoc
         * Returns a weak reference to the component.
         * @return CATSysWeakRef
         *   the weak reference to the component
         */
        CATSysWeakRef *GetComponentWeakRef() const;

        /** @nodoc */
        static void *operator new(size_t iSize);
        /** @nodoc */
        static void operator delete(void *iAddr);
        
        CATMetaClassStaticCheck

    protected:
        /** @nodoc */
        TypeOfData NecessaryData;
        /** @nodoc */
        LONG m_cRef;
        
    private:
#if defined(PLATEFORME_DS64)
        /** @nodoc */
        ULONG m_reserved;
#endif
        /** @nodoc */
        CATBaseUnknown *delegate;

        // friend functions
        // internal QueryInterface, internal use
        friend HRESULT StaticQueryInterface(
                                        CATBaseUnknown *, 
                                        CATBaseUnknown *, 
                                        CATMetaClass const*, 
                                        int, 
                                        const IID &, 
                                        void **);
        // internal Tie QueryInterface, internal use
        friend ExportedByJS0CORBA HRESULT Tie_Query(
                                        IUnknown *, 
                                        CATBaseUnknown *, 
                                        CATBaseUnknown *, 
                                        CATMetaClass const*, 
                                        int, 
                                        const IID &, 
                                        void **);
        friend ExportedByJS0CORBA HRESULT Tie_Query(
                                        CATBaseUnknown&, 
                                        CATMetaClass const*, 
                                        int, const IID &, 
                                        void **);
        // internal Tie destructor, internal use
        friend ExportedByJS0CORBA void Tie_Destruct(
                                        IUnknown *, 
                                        CATBaseUnknown **, 
                                        TypeOfClass, 
                                        LONG);
        friend ExportedByJS0CORBA void Tie_Destruct(
                                        CATBaseUnknown&,
                                        TypeOfClass);
        // internal Tie destructor, internal use
        friend ExportedByJS0CORBA ULONG Tie_Release(
                                        int *Destruct, 
                                        CATBaseUnknown **Data, 
                                        CATBaseUnknown *delegate,
                                        TypeOfClass type, 
                                        LONG *ref);
        friend ExportedByJS0CORBA ULONG Tie_Release(
                                        int *Destruct, 
                                        CATBaseUnknown&,
                                        TypeOfClass type);
        // internal Tie constructor, internal use
        friend ExportedByJS0CORBA CATBaseUnknown *Tie_Construct(
                                        IUnknown *,
                                        CATMetaClass const*,
                                        CATBaseUnknown **,
                                        int,
                                        CATBaseUnknown *,
                                        const GUID &,
                                        TypeOfClass,
                                        CATBaseUnknown *&,
                                        CATBaseUnknown*(*)(),
                                        CATBaseUnknown *,
                                        CATBaseUnknown **);
        friend ExportedByJS0CORBA void Tie_Construct(
                                        CATBaseUnknown&,
                                        CATMetaClass const*,
                                        CATBaseUnknown *,
                                        const GUID &,
                                        TypeOfClass,
                                        CATBaseUnknown *&,
                                        CATBaseUnknown*(*)(),
                                        CATBaseUnknown *);
        // to link chained TIE, internal use
        friend ExportedByJS0CORBA IUnknown *Tie_Link(
                                        CATBaseUnknown *,
                                        CATBaseUnknown *,
                                        const GUID &);
        // to create BOAs, internal use
        friend ExportedByJS0CORBA IUnknown *ToCreateBOA(
                                        CATBaseUnknown *,
                                        CATBaseUnknown *,
                                        CATMetaClass const*, const GUID &,
                                        CATBaseUnknown *(*)());
        
        // get and set, internal use
        friend ExportedByJS0CORBA CATBaseUnknown *GetData(const CATBaseUnknown *);
        friend ExportedByJS0CORBA void SetData(CATBaseUnknown *, CATBaseUnknown *);
        
        friend class Tie_StackCtx;  // to call methods from the TIE, internal use
        friend class CATSysWeakRef;
        friend class CATDelegation;
        friend struct DataForImpl;
};


/**
 * @nodoc
 * Creates an class instance by its CLSID.
 * @param iclsid
 *   The class identifier for which an instance is requested.
 * @param iouter
 *   The aggregated component pointer ( only for Windows ).
 * @param icontext
 *   The component context ( only for Windows ).
 * @param iid
 *   The identifier of the interface which is queryied on the implementation.
 * @param ppv
 *   The address where the returned pointer to the interface is located.
 * @return
 *   <dl>
 *   <dt><tt>S_OK</tt>          <dd>if the query succeeds
 *   <dt><tt>E_UNEXPECTED</tt>  <dd>for an unexpected failure
 *   <dt><tt>E_NOINTERFACE</tt> <dd>if the interface does not exist
 *   </dl>
 */
ExportedByJS0CORBA HRESULT __stdcall CATCoCreateInstance(const CLSID &iclsid,
                                                         IUnknown *iouter,
                                                         DWORD icontext,
                                                         const IID &iid,
                                                         void **ppv);
/**
 * @nodoc
 */
ExportedByJS0CORBA HRESULT __stdcall CATCreateClassInstance(const CLSID &iclsid,
                                                            IUnknown *iouter,
                                                            DWORD icontext,
                                                            const IID &iid,
                                                            void **ppv);

/**
 * @nodoc
 * Creates an class instance by its name.
 * @param iname
 *   The class name for which an instance is requested.
 * @param iouter
 *   The aggregated component pointer ( only for Windows ).
 * @param icontext
 *   The component context ( only for Windows ).
 * @param iid
 *   The identifier of the interface which is queryied on the implementation.
 * @param ppv
 *   The address where the returned pointer to the interface is located.
 * @return
 *   <dl>
 *   <dt><tt>S_OK</tt>          <dd>if the query succeeds
 *   <dt><tt>E_UNEXPECTED</tt>  <dd>for an unexpected failure
 *   <dt><tt>E_NOINTERFACE</tt> <dd>if the interface does not exist
 *   </dl>
 */
ExportedByJS0CORBA HRESULT __stdcall CATCoCreateInstance(const char *iname,
                                                         IUnknown *iouter,
                                                         DWORD icontext,
                                                         const IID &iid,
                                                         void **ppv);
/**
 * @nodoc
 * Creates an class instance by its name.
 * @param iname
 *   The class name for which an instance is requested.
 * @param iouter
 *   Reserved for future use, Must be NULL.
 * @param icontext
 *   The component context ( only for Windows ).
 * @param iid
 *   The identifier of the interface which is queryied on the implementation.
 * @param oppv
 *   The address where the returned pointer to the interface is located.
 * @return
 *   <dl>
 *   <dt><tt>S_OK</tt>          <dd>if the query succeeds
 *   <dt><tt>E_UNEXPECTED</tt>  <dd>for an unexpected failure
 *   <dt><tt>E_NOINTERFACE</tt> <dd>if the interface does not exist
 *   </dl>
 */
ExportedByJS0CORBA HRESULT __stdcall CATCreateClassInstance(const char *iname,
                                                            IUnknown *iouter,
                                                            DWORD icontext,
                                                            const IID &iid,
                                                            void **oppv);
/**
 * @nodoc
 */
ExportedByJS0CORBA const info_dic *AddDictionary (const GUID *, const GUID *,
                                                            const char *, const char *,
                                                            const char *, void *, 
                                                            const char * = NULL, void * = NULL,
                                                            int definitif = 1,
                                                            int defaut = 0,
                                                            int islicensed = 2,
                                                            const char*FwName = NULL);

/**
 * @nodoc
 */
ExportedByJS0CORBA CATMetaClass *QueryMetaObject(const GUID *iguid);
/**
 * @nodoc
 */
ExportedByJS0CORBA CATMetaClass *QueryMetaObject(const char *iname);


#include "CATMacForIUnknown.h"
#include "CATBaseUnknown_var.h"

#include "CATMacInterface.h"

#endif  // __CATBaseUnknown
