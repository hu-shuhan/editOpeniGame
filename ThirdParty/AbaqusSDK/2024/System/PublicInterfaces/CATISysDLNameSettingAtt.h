//===========================================================================
// COPYRIGHT DASSAULT SYSTEMES 2003                                         
//===========================================================================
//                                                                           
//  CATISysDLNameSettingAtt                                                
//                                                                           
//  Usage Notes: Interface Definition                                          
//===========================================================================
//  Creation fevrier  2003                                 adt         	
//===========================================================================
#ifndef __CATISYSDLNAMESETTINGATT
#define __CATISYSDLNAMESETTINGATT
#include "JS0STDLIB.h"
#include "CATBaseUnknown.h"   


// COPYRIGHT DASSAULT SYSTEMES 2003
/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

class CATUnicodeString;
class CATSettingInfo;


#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0STDLIB IID IID_CATISysDLNameSettingAtt;
#else
extern "C" const IID IID_CATISysDLNameSettingAtt;
#endif




/**
* Interface to handle the DLNames.
* <b>Role</b>: This interface is implemented by a component which
* represents the controller of the DLNames. 
* <br>This interface defines:
* <ul>
* <li>A method to set each DLNamw</li>
* <li>A method to get the value of each DLName</li>
* <li>A method to lock/unlock each parameter</li>
* <li>A method to retrieve the informations concerning each parameter</li>
* </ul>
*/
class ExportedByJS0STDLIB CATISysDLNameSettingAtt : public CATBaseUnknown
{
  CATDeclareInterface;
  
public :

/**
 * Retrieves if one has the right to create or delete new DLNames .
 * <br><b>Role</b>:  Retrieves the value of the parameter describing if one has 
 * the right to create or deletenew DLNames in the current list.
 * @param oRight
 *     TRUE if the creation is authorized.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetDLNameCreationRight (unsigned char &oRight)=0;
  

/**
 * Sets the creation right.
 * <br><b>Role</b>:  Sets the right to create new DLNames in the current list.
 * @param iRight
 *     TRUE if the creation is authorized, FALSE to forbid it
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetDLNameCreationRight (unsigned char iRight)=0;


/**
 * Retrieves the state of the parameter DLNameCreationRight.
 * <br>Refer to @href CATSysSettingController for a detailled description.
*/
  virtual HRESULT GetDLNameCreationRightInfo (CATSettingInfo *oInfo)=0;
  
/**
 * Locks or unlocks the parameter DLNameCreationRight. 
 * <br><b>Role</b>:  Retrieves the state of the parameter describing if one has 
*/
  virtual HRESULT SetDLNameCreationRightLock (unsigned char iLock)=0;


/**
 * Retrieves if one has the right to create new Root DLNames.
 * <br><b>Role</b>:  Retrieves the value of the parameter describing if one has 
 * the right to create new DLNames in the current list.
 * @param oRight
 *     TRUE if the creation is authorized.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetRootDLNameCreationRight (unsigned char &oRight)=0;
  

/**
 * Sets the creation right of Root DLNames.
 * <br><b>Role</b>:  Sets the right to create new Root DLNames in the current 
 * list.
 * @param iRight
 *     TRUE if the creation is authorized, FALSE to forbid it
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetRootDLNameCreationRight (unsigned char iRight)=0;


/**
 * Retrieves the state of the parameter RootDLNameCreationRight.
 * <br>Refer to @href CATSysSettingController for a detailled description.
*/
  virtual HRESULT GetRootDLNameCreationRightInfo (CATSettingInfo *oInfo)=0;
  
/**
 * Locks or unlocks the parameter RootDLNameCreationRight.
 * <br>Refer to @href CATSysSettingController for a detailled description.
*/
  virtual HRESULT SetRootDLNameCreationRightLock (unsigned char iLock)=0;


/**
 * Retrieves the list of the DLNames.
 * <br><b>Role</b>: Retrieves the list of the defined DLNames.
 * @param oNbDLname
 *	The number of defined DLNames.
 * @param oTabDLName
 *	The array of DLNames
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetDLNameList ( unsigned int *oNbDLname,
				  CATUnicodeString **&oTabDLName) =0;

/**
 * Retrieves the list of the Sub-DLNames.
 * <br><b>Role</b>: Retrieves the list of the DLNames created in a given DLName.
 * @param iDLName
 *      The Father DLName. if iDLName=NULL all DLNames created at the root level
 *      are return.
 * @param oNbDLname
 *	The number of defined DLNames.
 * @param oTabDLName
 *	The array of DLNames
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 *	<br><tt>E_OUTOFMEMORY:</tt> on allocation failure
 * 	<br><tt>E_FAIL:</tt>  on other failures
*/
  virtual HRESULT GetDLNameSubList ( CATUnicodeString* iDLName,
				     unsigned int *oNbDLname,
				     CATUnicodeString **&oTabDLName) =0;
  
/**
 * Retrieves the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Retrieves the mapping between a logical name and 
 * the physical path.
 * @param iDLName
 *	the logical name.
 * @param oRealNameUnix
 *	the real physical path corresponding to the logical name on Unix.
 * @param oRealNameNT
 *	the real physical path corresponding to the logical name on Windows.
 *@param iFather
 *	if applicable the Name of the parent DLName
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetDLName(const CATUnicodeString *iDLName, 
			    CATUnicodeString **oRealNameUnix,
			    CATUnicodeString **oRealNameNT,
			    CATUnicodeString **oFather)=0;

/**
 * Retrieves the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Retrieves the mapping between a logical name and 
 * the physical path in a litteral form.
 * @param iDLName
 *	the logical name.
 * @param oRealNameUnix
 *	the real physical path corresponding to the logical name on Unix.
 * @param oRealNameNT
 *	the real physical path corresponding to the logical name on Windows.
 *@param iFather
 *	if applicable the Name of the parent DLName
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetDLNameExp(const CATUnicodeString *iDLName, 
			       CATUnicodeString **oRealNameUnix,
			       CATUnicodeString **oRealNameNT,
			       CATUnicodeString **oFather)=0;
  


/**
 * Sets the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Sets the value of the cache maximum size in Mo
 * @param iDLName
 *	the logical name.
 * @param oRealNameUnix
 *	the real physical path corresponding to the logical name on Unix.
 * @param oRealNameNT
 *	the real physical path corresponding to the logical name on Windows.
 *@param iFather
 *	if applicable the Name of the parent DLName
 *@param iVerifDirectory
 *	if VerifDirectory is set the existence of the directory on the current
 *      platform will be check.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetDLName(const CATUnicodeString *iDLName, 
			    const CATUnicodeString *iRealNameUnix,
			    const CATUnicodeString *iRealNameNT,
			    const CATUnicodeString *iFather,
			    int iVerifDirectory=0)=0;



/**
 * Retrieves the state of the for a given DLName.
 * @param oInfo
 *	Address of an object CATSettingInfo.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetDLNameInfo (const CATUnicodeString *iDLName,
				 CATSettingInfo* oInfo)=0;

/** 
 * Locks or unlocks the DLName.
 * <br><b>Role</b>: Locks or unlocks the given DLName if the
 * operation is allowed in the current administrated environment. In user mode 
 * this method will always return E_FAIL.
 * @param iLocked
 *	the locking operation to be performed
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>   to lock the parameter.
 * 	<br><tt>0:</tt>   to unlock the parameter.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetDLNameLock (const CATUnicodeString *iDLName,
				 unsigned char iLock)=0;




/**
 * Remove a logical name.
 * <br><b>Role</b>: Remove a DLName in the current set if it is possible.
 * @param iDLName
 *	the logical name.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT RemoveDLName(const CATUnicodeString *iDLName)=0;

/**
 * Rename an existing DLName.
 * <br><b>Role</b>: Rename a DLName in the current set if it is possible.
 * @param iDLName
 *	the logical name to rename.
 * @param iNewName
 *	the new logical name.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>   on Success
 * 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT RenameDLName(const CATUnicodeString *iDLName,
			       const CATUnicodeString *iNewName)=0;



  

};




#endif


