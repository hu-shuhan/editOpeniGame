#ifndef _CATCommandGlobalUndo
#define _CATCommandGlobalUndo


// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "JS0FM.h"
#include <CATBaseUnknown.h>
#include <stdlib.h>


 /**
  * Prototype of the global undo/redo methods.
  * <b>Role</b>: These methods are passed as parameters of the 
  * @href CATCommandGlobalUndo class methods.
  * @param iData
  *   The data specified by the @href CATCommandGlobalUndo#SetData method.
  */
typedef void (*CATGlobalUndoMethod)(void * iData) ;

/**
 * Base class to create an undo/redo object for a command.
 * <b>Role</b>: This class provides methods to undo/redo the command global 
 * effects.
 * CNext records instances of this class in a command history that enables 
 * the end user to undo the previous command. So, each instance of this class
 * is created at the end of a command and is kept after the command deletion.  
 * <br>To create such an undo/redo object, you can derive this class and 
 * override the methods:
 * <ul>
 * <li>@href #BeforeUndo </li>
 * <li>@href #AfterUndo </li>
 * <li>@href #BeforeRedo </li>
 * <li>@href #AfterRedo </li>
 * </ul>
 * or instantiate a <tt>CATCommandGlobalUndo</tt> object and give as 
 * arguments the addresses of methods to execute to undo/redo the command.  
 * <br>Moreover, this undo/redo object must only undo/redo the objects that
 * are not transactional, that is to say that don't belong to a model which 
 * implements the CATIUndoTransaction interface, because such a model undoes 
 * and redoes its objects itself.
 * That's the reason why you can specify methods to be called before or after
 * the transactional model undo/redo.  
 */
class ExportedByJS0FM  CATCommandGlobalUndo:public CATBaseUnknown
{
  CATDeclareClass;

public:


 /**
  * Constructs an empty undo/redo object.
  */
     CATCommandGlobalUndo ();

 /**
  * Constructs an undo/redo object using undo/redo method addresses.
  * <br><b>Role</b>: The addresses of the methods to execute to undo or redo 
  * the command effects are given as arguments. These methods must be static 
  * methods of the command, because the command is deleted after its 
  * completion, whereas the undo/redo methods may be called a long time after.
  * @param iBeforeUndoMeth
  *   The method to undo the command effects before the transactional undo.
  * @param iBeforeRedoMeth
  *   The method to redo the command effect before the transactional redo.
  * @param iData
  *   An object that can be useful for the command undo/redo methods.
  *   <br><b>Cyclic reference</b>: A copy of <tt>iData</tt> is kept. This 
  *   object will be deallocated by the <tt>iDeallocateMeth</tt> method given
  *   as the fourth argument. 
  * @param iDeallocateMeth
  *   The method to deallocate the <tt>iData</tt> object passed as the third 
  *   argument.
  *   This method is called when the undo/redo object is removed from the 
  *   command history.
  */

     CATCommandGlobalUndo (CATGlobalUndoMethod iBeforeUndoMeth,
                           CATGlobalUndoMethod iBeforeRedoMeth, 
                           void * iData,
                           CATGlobalUndoMethod iDeallocateMeth = NULL);
 /**
  * @nodoc
  */
     CATCommandGlobalUndo (CATGlobalUndoMethod BeforeUndoMeth, void * data);
  
     ~CATCommandGlobalUndo ();


 /**
  * Sets the method to undo the command effects before the transactional undo.
  * @param iMeth
  *   The method which undoes the command effects.
  *   It must be a command's static method because the command is deleted 
  *   after its completion.
  */
    virtual void SetBeforeUndoMeth (CATGlobalUndoMethod iMeth);

 /**
  * Sets the method to redo the command effects before the transactional redo.
  * @param iMeth
  *   The method which redoes the command effects.
  *   It must be a command's static method because the command is deleted 
  *   after its completion.
  */
    virtual void SetBeforeRedoMeth (CATGlobalUndoMethod iMeth);

 /**
  * Sets the method to undo the command effects after the transactional undo.
  * @param iMeth
  *   The method which undoes the command effects.
  *   It must be a command's static method because the command is deleted 
  *   after its completion.
  */
    virtual void SetAfterUndoMeth (CATGlobalUndoMethod iMeth);

 /**
  * Sets the method to redo the command effects after the transactional redo.
  * @param iMeth
  *   The method which redoes the command effects.
  *   It must be a command's static method because the command is deleted 
  *   after its completion.
  */
    virtual void SetAfterRedoMeth (CATGlobalUndoMethod iMeth);

 /**
  * Sets data which may be useful to undo/redo the command effects.
  * <br><b>Role</b>: This data will be given as the argument of the 
  * <tt>CATGlobalUndoMethod</tt> methods.
  * @param iData
  *   An object that can be useful for the command undo and redo methods.
  *   <br><b>Cyclic reference</b>: A copy of <tt>iData</tt> is kept. This 
  *   object will be deallocated by the method given by the 
  *   @href #SetDeallocateMeth method.
  */
    virtual void SetData (void * iData);

 /**
  * Sets the method to deallocate the data given with the SetData method.
  * @param iMeth
  *   The method which deallocates the data.
  *   It must be a command's static method because the command is deleted 
  *   after its completion.
  */
    virtual void SetDeallocateMeth (CATGlobalUndoMethod iMeth);

 /**
  * Undoes the command effects before the transactional undo.
  * <br><b>Role</b>: Redefine this method when deriving <tt>
  * CATCommandGlobalUndo</tt>.
  * @return
  *   An HRESULT.
  *   But it is not taken into account. Whatever the method returns, the 
  *   dialog goes on.
  */
    virtual HRESULT          BeforeUndo();

 /**
  * Redoes the command effects before the transactional undo.
  * <br><b>Role</b>: Redefine this method when deriving <tt>
  * CATCommandGlobalUndo</tt>.
  * @return
  *   An HRESULT.
  *   But it is not taken into account. Whatever the method returns, the 
  *   dialog goes on.
  */
    virtual HRESULT          BeforeRedo();

 /**
  * Undoes the command effects after the transactional undo.
  * <br><b>Role</b>: Redefine this method when deriving <tt>
  * CATCommandGlobalUndo</tt>.
  * @return
  *   An HRESULT.
  *   But it is not taken into account. Whatever the method returns, the 
  *   dialog goes on.
  */
    virtual HRESULT          AfterUndo();

 /**
  * Redoes the command effects after the transactional redo.
  * <br><b>Role</b>: Redefine this method when deriving <tt>
  * CATCommandGlobalUndo</tt>.
  * @return
  *   An HRESULT.
  *   But it is not taken into account. Whatever the method returns, the 
  *   dialog goes on.
  */
    virtual HRESULT          AfterRedo();
 

    /*---------------------------------------------*/
    //  obsolete methods. Do not use it any more 
    /*---------------------------------------------*/

 /**
  * @nodoc
  */
     virtual void ExecuteUndo () ;

 /**
  * @nodoc
  */
     virtual void ExecuteRedo () ;

 /**
  * @nodoc
  */
     void SetRedoObject (CATCommandGlobalUndo * );

 /**
  * @nodoc
  */
     CATCommandGlobalUndo * GetRedoObject (); 


protected:

    /*------------------*/
    // internal data 
    /*------------------*/

 /** @nodoc */
   void *                     _data;
 /** @nodoc */
   CATGlobalUndoMethod        _BeforeUndoMeth;
 /** @nodoc */
   CATGlobalUndoMethod        _BeforeRedoMeth; 
 /** @nodoc */
   CATGlobalUndoMethod        _AfterUndoMeth;
 /** @nodoc */
   CATGlobalUndoMethod        _AfterRedoMeth;
 /** @nodoc */
   CATGlobalUndoMethod        _DeallocateMeth;
 /** @nodoc */
   CATCommandGlobalUndo *     _objectRedo ;
 
};
#endif


