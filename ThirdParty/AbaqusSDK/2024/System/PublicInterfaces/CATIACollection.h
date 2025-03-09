#ifndef CATIACollection_h
#define CATIACollection_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U6
 */

#ifndef ExportedByJS0INF
/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0INF)
/** @nodoc */
# define ExportedByJS0INF   DSYExport
#else // __JS0INF
/** @nodoc */
# define ExportedByJS0INF   DSYImport
#endif  // __JS0INF

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
#endif  // !ExportedByJS0INF

#include "CATBaseDispatch.h"

interface IUnknown;
class CATIABase;
class CATIAApplication;
#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0INF IID IID_CATIACollection;
#else
extern "C" const IID IID_CATIACollection;
#endif

/**
 * Represents the base object for collections.
 * As a base object, it provides properties shared by any other object.
 * Use the @href CATBaseCollection adaptor class to implement this interface.
 */
class ExportedByJS0INF CATIACollection: public CATBaseDispatch {
  CATDeclareInterface;

	public:
	
		/**
		 * Returns the application.
		 * <br><b>Role</b>:
		 * The application is the root object of the object structure and can be
		 * retrieved from any object in this object structure.
		 * @param oApplication
		 *   The returned application
		 */
		virtual HRESULT  __stdcall get_Application(CATIAApplication *& oApplication)=0;

		/**
		 * Returns the parent object.
		 * <br><b>Role</b>:
		 * The parent object of a given object is the object just above
		 * in the object structure, usually the object that created this
		 * object and that aggregates it.
		 * @param oParent
		 *   The returned parent
		 */
		virtual HRESULT  __stdcall get_Parent(CATBaseDispatch *& oParent)=0;
		
		/**
		 * Retrieves an object with the specified name.
		 * @param iIDName
		 *   The specified name of the object to retrieve
		 * @param oObject
		 *   The returned object
		 */
		virtual HRESULT  __stdcall GetItem(const CATBSTR & iIDName, CATBaseDispatch *& oObject)=0;
		
		/**
		 * Returns the name of the collection.
		 * @param oName
		 *   The returned name
		 */
		virtual HRESULT  __stdcall get_Name(CATBSTR & oName)=0;
		
		/**
		 * Returns the number of objects in the collection.
		 * @param oNbItems
		 *   The returned count
		 */
		virtual HRESULT  __stdcall get_Count(CATLONG & oNbItems)=0;
		
		/**
		 * Returns an iterator on the collection.
		 * <br><b>Role</b>:
		 * This method must be implemented to allow the caller to easily iterate
		 * through the collection in VB scripts with the <tt>For Each</tt> statement.
		 * @param oEnumIter
		 *   The returned iterator
		 */
		virtual HRESULT  __stdcall get__NewEnum(IUnknown *& oEnumIter)=0;
	
};

CATDeclareHandler(CATIACollection,CATBaseDispatch);

#endif
