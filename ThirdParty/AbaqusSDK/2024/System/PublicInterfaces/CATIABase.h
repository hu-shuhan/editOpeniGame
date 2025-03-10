#ifndef CATIABase_h
#define CATIABase_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U6
 */

#include "JS0INF.h"
#include "CATBSTR.h"
#include "CATBaseDispatch.h"

class ExportedByJS0INF CATIAApplication;

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0INF IID IID_CATIABase;
#else
extern "C" const IID IID_CATIABase;
#endif

/**
 * Represents the base object for all other objects except
 * collection and reference objects.
 * As a base object, it provides properties shared by any other object.
 * Use the @href CATBaseObject adaptor class to implement this interface.
 */
class ExportedByJS0INF CATIABase: public CATBaseDispatch
{
  CATDeclareInterface;

public:
    /**
     * Returns the application.
     * <br><b>Role</b>:
     * The application is the root object of the object structure and can be
     * retrieved from any object
     * in this object structure using the Application property.
     * The root object, also called top-level object, is the object located
     * at the top of the application's object structure.
     * It is used by clients to retrieve and navigate across all
     * the application's subordinate objects.
     * If the client runs in-process, it retrieves the object at the top of
     * the object structure. If the client runs out-process, it should call
     * the GetApplication method to retrieve the object at the top of the
     * object structure, which is the only object accessible from outside.
     * The Application property is thus the way to jump from any object
     * up to the
     * root of the object structure, allowing then to navigate downwards.
     * For in-process scripting, the application is always referred to as
     * <tt>CATIA</tt>.
     * Note that the Application property of the Application object returns the
     * Application object itself.
     */
virtual HRESULT  __stdcall get_Application(CATIAApplication *& oApplication)=0;
    /**
     * Returns the parent object.
     * <br><b>Role</b>:
     * The parent object of a given object is the object just above
     * in the object structure, usually the object that created this
     * object and that aggregates it.
     * In the case of an object part of a collection, the parent object
     * can be the collection object itself or the object that 
     * aggregates the collection object.
     * The Parent property is the way to step upwards in the object
     * structure. 
     * Note that the Parent property of the Application object returns the
     * Application object itself.
     */
virtual HRESULT  __stdcall get_Parent(CATBaseDispatch *& oParent)=0;
    /**
     * Returns the name of the object.
     * <br><b>Role</b>:
     * The name is a character string automatically assigned to any object to
     * handle it easier. Even if the Name property allows you to reassign an object name,
	 * this is not advised. Many objects, such as the application and the collections,
	 * have names that you must not change, and it's safer to use Name as a read only property.  
     * When an object is part of a collection, the object name can often
     * be used in place of the object rank to retrieve or remove the object,
     * providing the Item and Remove methods of the collection feature
     * an argument with the Variant type.
     * A name must start with a letter.
     * It can include numbers, but it can't include spaces.
     * If the object has no name set, the default name returned is the
     * object type. For example, the Name property of a Viewer3D object
     * with no name set returns Viewer3D.
     */
virtual HRESULT  __stdcall get_Name(CATBSTR & oNameBSTR)=0;
    /**
     * Sets the name of the object.
     * <br><b>Role</b>:
     * The name is a character string automatically assigned to any object to
     * handle it easier. Even if the Name property allows you to reassign an object name,
	 * this is not advised. Many objects, such as the application and the collections,
	 * have names that you must not change, and it's safer to use Name as a read only property.  
     * When an object is part of a collection, the object name can often
     * be used in place of the object rank to retrieve or remove the object,
     * providing the Item and Remove methods of the collection feature
     * an argument with the Variant type.
     * A name must start with a letter.
     * It can include numbers, but it can't include spaces.
     * If the object has no name set, the default name returned is the
     * object type. For example, the Name property of a Viewer3D object
     * with no name set returns Viewer3D.
     */
virtual HRESULT  __stdcall put_Name(const CATBSTR & iNameBSTR)=0;
  /**
   * Returns an object from its name.
   * <br><b>Role</b>: To retrieve an object when only its name is available.
   * @param IDName
   *   The searched obect name
   * @return
   *   The searched object
   */
virtual HRESULT  __stdcall GetItem(const CATBSTR & IDName,
CATBaseDispatch *& RealObj)=0;
};
CATDeclareHandler(CATIABase,CATBaseDispatch);

#endif
