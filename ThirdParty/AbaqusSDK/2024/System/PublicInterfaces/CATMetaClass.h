#include "CATMetaClass_required.h"
#ifndef __CATMetaClass
#define __CATMetaClass  42601

/* -*-C++-*- */
/* COPYRIGHT DASSAULT SYSTEMES 1997                                      */
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
/*=======================================================================*/

// Meta class attached to an implementation/extension class
// It shows the schema the corresponding class belongs to

#include "JS0CORBA.h"
#include "CATOMTypeOfClass.h"  // TypeOfClass
#include "DynamicLicensing.h"
#include "CATBoolean.h"
#include <atomic>

class CATBaseUnknown;

/**
 * GUID of the CATMetaClass class.
 */
extern ExportedByJS0CORBA const GUID CLSID_CATMetaClass;

/**
 * Structure returned by the ListOfSupportedInterface method.
 * @param Interface
 *   The interface name supported (implemented) by a given class.
 * @param Condition
 *   The Condition function.
 * @param suiv
 *   The pointer to the following SupportedInterface structure which contains another
 *   interface name supported by the given class.
 *   Set to NULL if there is no follower.
 */
struct SupportedInterface
{
   const char *Interface;
   int condition;
   SupportedInterface *suiv;
};

/**
 * Possible types of interface support for an implementation.
 * @param CATSysDoesNotSupportInterface
 *   The implementation does not support the interface.
 * @param CATSysSupportsInterface
 *   The implementation supports the interface.
 * @param CATSysSupportsInterfaceUsingPrecond
 *   The implementation supports the interface with a condition function.
 */
enum CATSysInterfaceSupportType {
  CATSysDoesNotSupportInterface = 0,
  CATSysSupportsInterface = 1,
  CATSysSupportsInterfaceUsingCondition = 2
};

/**
 * Structure returned by the ListOfSupportedClass method.
 * @param Class
 *   The class name that supports (implements) a given interface.
 * @param suiv
 *   A pointer to the following SupportedClass structure which contains another
 *   class name part that supports (implements) the same interface.
 *   Set to NULL if there is no follower.
 */
struct SupportedClass
{
   const char *Class;
   SupportedClass *suiv;
};

// Functions prototypes
/**
 * @nodoc
 */
typedef IUnknown *(*CATSysCreationFunc)(CATBaseUnknown *, CATBaseUnknown *);
/**
 * @nodoc
 */
typedef int (*CATSysConditionFunc)(CATBaseUnknown*, GUID const&);

/**
 * Structure returned by the GetInterfaceFactory method.
 * @param fct 
 *  The Creation function.
 * @param condition 
 *  The Condition function.
 * @param FWName
 *  The framework name
 * @param islicensed
 *  Licensing info
 */
struct RetFct
{
   CATSysCreationFunc fct;
   CATSysConditionFunc condition;
   const char * FwName;
   const void * reserved;   // nodoc
   int islicensed;// 2 pas licensiee
                  // 0 licenciee mais pas autorise
                  // 1 licenciee et autorisee
};

class CATSysMetaExtraData;
struct ChainInterfaceArray;

/**
 * Class used to store and manage data for classes that implement interfaces. 
 * There is one instance of CATMetaClass per class, called the meta object for 
 * that class. 
 * It contains data specific to the class, such as the GUID of the class, the class name,
 * a pointer to the meta object of the class of which this class is an extension,
 * if any, the implemented interfaces,
 * and other data required for the class management.
 */
class ExportedByJS0CORBA CATMetaClass {

	friend ExportedByJS0CORBA HRESULT ChangeLicensingInOM();

	public :

		/**
		 * Constructs a meta object, instance of the CATMetaClass class, for a given class.
		 * @param iGuid 
		 *   The GUID of the class for which a meta object construction is required.
		 * @param iName
		 *   The name of this class.
		 * @param iBaseClass
		 *   The pointer to the meta object associated with this class's base class,
		 *   that is the class from which this class derives.
		 * @param iBaseImplementation 
		 *   The pointer to the meta object associated with the class for which this class is an
		 *   extension, if any. Otherwise defaulted to NULL.
		 * @param iType
		 *   The class type. Valid values are Implementation, DataExtension, 
		 *   CodeExtension, CacheExtension or TransientExtension.
		 */
		CATMetaClass(const GUID   * iGuid,
					 const char   * iName, 
					 CATMetaClass * iBaseClass,
					 CATMetaClass * iBaseImplementation = nullptr, 
					 TypeOfClass    iType               = Implementation);

		/**
		 * Returns the class name for which the given object is an instance.
		 */
		inline const char * IsA() const;
		
		/**
		 * Determines whether the given object's class derives from a given class.
		 * @param iClassName
		 *   The class name from which the given object's class is supposed to derive.
		 * @return 1 if the given object's class derives from <tt>iClassName</tt>
		 * and 0 otherwise.
		 */
		int IsAKindOf(const char * iClassName) const;

		/**
		 * Sets the class name for the given object.
		 * @param iClassName
		 *  The class name to assign to the given object's class.
		 */
		void __stdcall SetClassName(const char * iClassName);
		
		/**
		 * Sets the class alias for the given object.
		 * @param iAlias 
		 *  The class alias to assign to the given object's class.
		 */
		void __stdcall SetAlias(const char *iAlias);
		
		/**
		 * Returns the class alias for which the given object is an instance.
		 */
		const char * __stdcall GetAlias() const;
      
		/**
		 * Returns the class identifier (GUID) for the given object.
		 */
		inline const GUID& __stdcall GetClassId() const;
      
		/**
		 * Determines whether the given object's class derives from a given class.
		 * @param iClsid
		 *   The class identifier (CLSID) from which the given object's class is supposed to derive.
		 * @return 1 if the given object's class derives from the class with
		 * <tt>iClsid</tt> as CLSID and 0 otherwise.
		 */
		int __stdcall IsAKindOf(const CLSID &iClsid) const;

		/**
		 * Sets the class identifier (CLSID) for the given object.
		 * @param iClsid
		 *   The class identifier (CLSID) to assign to the given object's class.
		 */
		void __stdcall SetClassId(const CLSID &iClsid);

		/**
		 * Determines whether the given object's class derives from a given class whose
		 * meta object is <tt>iMetaObject</tt>
		 * @param iMetaObject
		 *  The meta object of the class from which the given object
		 * class is supposed to derive.
		 * @return 1 if the given object's class derives from the class
		 * whose meta object is <tt>iMetaObject</tt> and 0 otherwise.
		 */
		int __stdcall IsAKindOf(const CATMetaClass * iMetaObject) const;

		/**
		 * Returns the meta object for the base class of the given object.
		 */
		inline CATMetaClass * __stdcall GetMetaObjectOfBaseClass() const;
      
		/**
		 * Sets the meta object for the base class of the given object.
		 * @param iMetaObject
		 *  The pointer to the meta object to set to the given object's base class.
		 */
		void __stdcall SetMetaObjectOfBaseClass(CATMetaClass * iMetaObject);

		/**
		 * Returns a pointer to the meta object of the class the given meta object's
		 * associated class is an extension of.
		 */
		CATMetaClass * __stdcall IsExtensionOf() const;
		
		/**
		 * Returns a pointer to the list of pointers to the meta objects of the classes
		 * that have for extension the class with the given meta object.
		 */
		const CATMetaClass ** __stdcall GetListOfSupportedImplementation() const;
		
		/**
		 * Adds the given meta object <tt>iMetaObjectOfClassToExtend</tt> to the list of 
		 * supported implementations.
		 */
		void __stdcall SetExtensionOf(CATMetaClass * iMetaObjectOfClassToExtend);

		/**
		 * Returns the meta object's associated class type.
		 */
		inline TypeOfClass __stdcall GetTypeOfClass() const;

		/**
		 * Sets the meta object's associated class type.
		 * @param iType
		 *   The class type. Can take the values <tt>Implementation</tt>, <tt>DataExtension</tt>,
		 *   <tt>CodeExtension</tt>, <tt>CacheExtension</tt> or <tt>TransientExtension</tt>.
		 */
		void __stdcall SetTypeOfClass(TypeOfClass iType);

	

		/**
		 * Adds an interface to the meta object's associated class.
		 * @param iid
		 *   The interface identifier to add
		 * @param iCreateFunction
		 *   The Create function of the object which implements the interface.
		 * @param iCondition
		 *   The Condition function.
		 * @return 0 if the interface is added and 1 otherwise.
		 */
		int __stdcall AddInterfaceFactory(const IID &iid,
										CATSysCreationFunc iCreateFunction,
										CATSysConditionFunc iCondition,
										const char * FwName,
										int defaut, 
										int islicensed,
										int OneLoadWasNotAllowed) const;
										
		/**
		 * Returns a structure that contains the create function and the condition function
		 * for an interface implemented by the meta object's associated class.
		 * @param iInterfaceIID
		 *   The interface identifier.
		 * @return A RetFct structure instance. 
		 */
		RetFct __stdcall GetInterfaceFactory(const IID &iInterfaceIID, int iDefaut = 0) const;

		/**
		 * Queries a meta object's class to determine whether it supports (implements)
		 * a given interface.
		 * @param iInterfaceIID
		 *   The interface identifier for which the meta object's class is queried.
		 * @param iOnlyFromDictionaries
		 *   Specify whether this method should only use the file dictionaries to perform the 
		 *   query, or if it should load the shared library to do it. 
		 *   By default, the shared library is loaded to check the interface support. 
		 *   For performance reason, if the dictionaries are correctly written 
		 *   (no not necessary entries nor missing ones), you can set <tt>iOnlyFromDictionaries</tt>
		 *   to TRUE to avoid the load of shared libraries. Warning, in this mode, incorrectly filled
		 *   dictionaries can lead to wrong results.
		 * @return <tt>CATSysSupportsInterface</tt> if the class supports <tt>iInterfaceIID</tt>,
		 *   <tt>CATSysSupportsInterfaceUsingCondition</tt> if it supports it with a condition function
		 *   and <tt>CATSysDoesNotSupportInterface</tt> if it does not support this interface.
		 */
		CATSysInterfaceSupportType __stdcall SupportInterface(
									const IID & iInterfaceIID, 
									CATBoolean iOnlyFromDictionaries = FALSE) const;

		/**
		 * Returns the list of interfaces supported by the given meta object's class.
		 * @return A pointer to a SupportedInterface structure instance that can be chained
		 *   to other instances of this structure, each containing a class name of an 
		 *   interface that matches the criterion and a pointer to another SupportedInterface
		 *   structure instance.
		 */
		const SupportedInterface * __stdcall ListOfSupportedInterface() const;

		/** 
		 * @deprecated R206 ListOfSupportingClasses
		 *
		 * Returns the list of classes that support (implement) a given interface,
		 * which must be part of the interfaces supported by the set of classes
		 * the class of the given meta object belongs to.
		 *
		 * @param iInterfaceName
		 *   The name of the interface class for which the list of classes that support it
		 *   is searched for.
		 * @return A pointer to a SupportedClass structure instance that can be chained
		 *   to other instances of this structure, each containing a class name that matches
		 *   the criterion and a pointer to another SupportedClass structure instance.
		 *   <br><br>
		 *
		 *   <b>Caution:</b>The returned list must be saved before to call once this method.
		 *   <pre>
		 *    Sample:
		 *
		 *       const SupportedClass * List = CATMetaObject::ListOfSupportedClass("xxx");
		 *       CATListOfCATString ListToSave ;
		 *       while ( List ) 
		 *       {
		 *          ListToSave.Append(List->Class);
		 *          
		 *          List = List->suiv;
		 *       } 
		 *       for ( int j= 1 ; j <= ListToSave.Size(); j++)
		 *       {
		 *        ....
		 *       }
		 *   </pre>
		 *   <br>
		 */
		static const SupportedClass * __stdcall ListOfSupportedClass(const char * iInterfaceName);

		/**
		 * Returns the list of classes that support (implement) a given interface
		 *
		 * @param iInterfaceName
		 *   The name of the interface class for which the list of classes that support it
		 *   is searched for.
		 * @return a chained list of SupportedClass structures. Each SupportedClass element of the list 
		 *   contains a class name that matches the criterion and a pointer to the next SupportedClass
		 *   element of the list.  
		 *   This returned list must be deallocated by the caller of the method ListOfSupportingClasses
		 *   using the method ListOfSupportingClassesCleaner.
		 */
		static SupportedClass * __stdcall ListOfSupportingClasses(const char * iInterfaceName);

		/**
		 * Full deallocation of a SupportedClass chained list.
		 *
		 * @param ioChainListToClean
		 *   The list to deallocate. This list is set to NULL by this method.
		 */
		static void  __stdcall ListOfSupportingClassesCleaner(SupportedClass *& ioChainedListToClean);
      

		// Internal use only
		void SetAuth(int iAuth);
		inline int GetAuth() const;		
		void SetDefault(int iDefault);
		inline int GetDefault() const;		
		void SetFWname(const char * iFwName);
		inline const char *GetFWname() const;		
		void SetIntroLibrary(const char *iIntroLibrary);
		const char *GetIntroLibrary() const;		
		void SetMandatoryAdapter(const char *iAdapterName);		
		
		static void *operator new(size_t iSize);
		static void operator delete(void *iAddr);

	private :
		// Destructor
		~CATMetaClass();

		/**
		 * Destructs all the created CATMetaClass instances.
		 * Must be called at exit only.
		 */
		static void Destruct();
		
		// GUID of the class 
		const GUID * _guid;
		// name of the class
		const char * _classname;
		// pointer on the meta-classes of the classes of which this class
		// is the extension
		CATMetaClass ** _implementations;
		// meta object of the base class
		CATMetaClass * _meta_of_base;
		// head of the chained interfaces
		mutable std::atomic<ChainInterfaceArray const *> _interface_head;
		// number of these interfaces
		unsigned short _interface_number;
		// allocated size for those interfaces
		unsigned short _interface_allocated;
		// type of the corresponding class
		TypeOfClass _class_type; 	/** sizeof(unsigned char) **/
		// internal purpose
		unsigned char _auth;
		unsigned char _defclass;
		// FrameWork name
		const char * _FWname;
		// Next CATMetaClass in the chain
		CATMetaClass * _next;
		
		// Additional information for internal use
		CATSysMetaExtraData * _sysMetaData;
		
		friend void CATMetaDestruct();
		friend class CATMetaClassInternal;
};

/**
 * @nodoc
 */
inline const char * CATMetaClass::IsA() const {return _classname;}
      
inline const GUID& __stdcall CATMetaClass::GetClassId() const {return *_guid;}

inline CATMetaClass * __stdcall CATMetaClass::GetMetaObjectOfBaseClass() const {return _meta_of_base;}
      
inline TypeOfClass __stdcall CATMetaClass::GetTypeOfClass() const {return(_class_type);};

inline int CATMetaClass::GetAuth() const {return _auth;}
		
inline int CATMetaClass::GetDefault() const {return _defclass;}
		
inline const char *CATMetaClass::GetFWname() const {return _FWname;}
		

/**
 * In order to find a meta object by its alias
 * @param alias
 *   The alias of the meta object
 * @return the meta object or NULL
 */
ExportedByJS0CORBA CATMetaClass * QueryByAlias(const char * iAlias);

/**
 * Class to instantiate in the macro CATImplementClass as fourth parameter
 * when the class which uses this macro is not an extension.
 */
class CATnull
{
public :
    /**
     * Returns the meta object associated with CATnull.
     */
    static CATMetaClass *MetaObject() { return nullptr; }
};

typedef CATnull CATNull;

// for migration
typedef CATMetaClass CATMetaObject;

#include "CATBaseUnknown.h"

#endif
