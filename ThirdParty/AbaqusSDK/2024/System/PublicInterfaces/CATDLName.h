//*===========================================================================*
//* COPYRIGHT DASSAULT SYSTEMES 2000                                          *
//*===========================================================================*
//*                                                                           *
//*  V5 Logical Naming Facilities                                             *
//*                                                                           *
//*  Usage Notes:                                                             *
//*===========================================================================*
//*  Creation mai 2000                                       adt              *
//*===========================================================================*
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
#ifndef CATDLNAME_H
#define CATDLNAME_H
#include "IUnknown.h"
#include "JS0STDLIB.h"

class CATUnicodeString;
class CATString;

#ifdef __cplusplus
extern "C" {
#endif


/** @nodoc
 * Creation of a logical Name.
 * <br><b>Role</b>: Creates a logical Name or modifies its value. The valuation
 * is only done if the iRealName represents a valid directory path. 
 * This functionis intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param iRealName
 *	the real physical path corresponding to the logical name on the current
 *	plateform.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATSetDLName ( const CATUnicodeString *iDLName, 
					   const CATUnicodeString *iRealName);


/** @nodoc
 * Creation of a logical Name.
 * <br><b>Role</b>: Creates a logical Name or modifies its value. The valuation
 * is only done if the iRealName represents a valid directory path. 
 * This functionis intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param iRealName
 *	the real physical path corresponding to the logical name on the current
 *	plateform.
 *@param iFather
 *	if applicable the Name of the parent DLName
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATSetDLNameEx ( const CATUnicodeString *iDLName, 
					     const CATUnicodeString *iRealName,
					     const CATUnicodeString *iFather);


/** @nodoc
 * Changes a logical Name.
 * <br><b>Role</b>: Renames an existing logical Name without modifing its value.
 * @param iDLName
 *	the logical name.
 * @param iNewDLName
 *	the new logical name.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATRenameDLName ( const CATUnicodeString *iDLName, 
					      const CATUnicodeString *iNewDLName);



/** @nodoc
 * Removes a logical Name.
 * <br><b>Role</b>: Remove a logical Name if it is not locked. 
 * This function is intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATRemoveDLName ( const CATUnicodeString *iDLName);


/** @nodoc
 * Creation of a logical Name.
 * <br><b>Role</b>: Creates a logical Name or modifies its value. The valuation
 * is only done if the iRealName represents a valid directory path. 
 * This function is intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param iRealNameU
 *	the real physical path corresponding to the logical name on Unix
 *@param iRealNameNT
 *	the real physical path corresponding to the logical name on NT
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATSetDLNameMulti ( const CATUnicodeString *iDLName, 
						const CATUnicodeString *iRealNameU,
						const CATUnicodeString *iRealNameNT);


/** @nodoc
 * Creation of a logical Name.
 * <br><b>Role</b>: Creates a logical Name or modifies its value. The valuation
 * is only done if the iRealName represents a valid directory path. 
 * This function is intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param iRealNameU
 *	the real physical path corresponding to the logical name on Unix
 *@param iRealNameNT
 *	the real physical path corresponding to the logical name on NT
 *@param iFather
 *	if applicable the Name of the parent DLName
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATSetDLNameMultiEx ( const CATUnicodeString *iDLName, 
						  const CATUnicodeString *iRealNameU,
						  const CATUnicodeString *iRealNameNT,
						  const CATUnicodeString *iFather);
/** @nodoc
 * Creation of a logical Name.
 * <br><b>Role</b>: Creates a logical Name or modifies its value. The valuation
 * is only done if the iRealName represents a valid directory path. 
 * This function is intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param iRealNameU
 *	the real physical path corresponding to the logical name on Unix
 *@param iRealNameNT
 *	the real physical path corresponding to the logical name on NT
 *@param iFather
 *	if applicable the Name of the parent DLName
 *@param iNocheck
 *	in order to avoid to verify the existence of the physical path and to
 *      to create it.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure 
*/
ExportedByJS0STDLIB HRESULT CATSetDLNameExt ( const CATUnicodeString *iDLName, 
					      const CATUnicodeString *iRealNameU,
					      const CATUnicodeString *iRealNameNT,
					      const CATUnicodeString *iFather,
					      int iNocheck=0);

/** @nodoc
 * Save the current DLNames.
 * <br><b>Role</b>: Save the current DLNames on disk.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATSaveDLName ();


/** @nodoc
 * Commits the current DLNames.
 * <br><b>Role</b>: Save the current state of DLNames in memory.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATCommitDLName ();


/** @nodoc
 * Restores the administrated state of the DLNames.
 * <br><b>Role</b>:  Restores the administrated state of the DLNames.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATResetDLName ();


/** @nodoc
 * Restores the last commited state of the DLNames.
 * <br><b>Role</b>:  Restores the last commited state of the DLNames.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATRollbackDLName ();


/**
 * Retrieves the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Retrieves the mapping between a logical name and the 
 * physical path, that has been defined with CATSetDLName. This function
 * is intended to be used in the context of the editor of logical names.
 * @param iDLName
 *	the logical name.
 * @param oRealName
 *	the real physical path corresponding to the logical name.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetDLName ( const CATUnicodeString *iDLName, 
					   CATUnicodeString **oRealName);

/**
 * Retrieves the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Retrieves the mapping between a logical name and the 
 * physical expansed path, that has been defined with CATSetDLName. This
 * function is intended to be used in the internal transformation of 
 * logical names into their physical representation.
 * @param iDLName
 *	the logical name.
 * @param oRealName
 *	the real physical path corresponding to the logical name.
 * @param oReserved
 *	not to be used. Reserved for internal use.
 * @param oType
 *	not to be used. Reserved for internal use.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetDLNameExp ( const CATUnicodeString *iDLName, 
					      CATUnicodeString **oRealName,
					      void **oReserved,
					      int *oType = 0);


/**
 * Retrieves the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Retrieves the mapping between a logical name and the 
 * physical path for all platforms, that has been defined with CATSetDLName.
 * This function is intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param oRealNameUnix
 *	the real physical path corresponding to the logical name on Unix
 * @param oRealNameNT
 *	the real physical path corresponding to the logical name on NT
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetDLNameMulti ( const CATUnicodeString *iDLName, 
						CATUnicodeString **oRealNameUnix,
						CATUnicodeString **oRealNameNT);


/** @nodoc
 * Retrieves the mapping between a logical name and the physical path.
 * <br><b>Role</b>: Retrieves the mapping between a logical name and the 
 * physical path for all platforms, that has been defined with CATSetDLName.
 * This function is intended to be used in the context of the editor of logical
 * names.
 * @param iDLName
 *	the logical name.
 * @param oRealNameUnix
 *	the real physical path corresponding to the logical name on Unix
 * @param oRealNameNT
 *	the real physical path corresponding to the logical name on NT
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetDLNameMultiEx ( const CATUnicodeString *iDLName, 
						  CATUnicodeString **oRealNameUnix,
						  CATUnicodeString **oRealNameNT,
						  CATUnicodeString **oFather);


/** @nodoc
 * Locks a DLName.
 * <br><b>Role</b>: Locks a DLName in order to prevent further modification.
 * This function is intended to be used in the context of the editor of logical
 * names in administrator's mode.
 * @param iDLName
 *	the logical name to lock
 * @param iLock
 *	the lock command to be performed.
 *      <b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  1 to Lock .
 *	<br><tt>E_FAIL  :</tt>0 to Unlock.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATSetLockDLName ( const CATUnicodeString *iDLName, 
					       int iLock);


/** @nodoc
 * Retrieves the state of the lock for a DLName.
 * <br><b>Role</b>:Retrieves the state of the lock for a DLName. This function
 * is intended to be used in the context of the editor of logical names.
 * @param iDLName
 *	the logical name.
 * @param oLock
 *	the state of the lock.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetLockDLName ( const CATUnicodeString *iDLName, 
					       char *oLock);


/**
 * Retrieves all the defined logical names.
 * <br><b>Role</b>: Allocates an array of CATUnicodeString to be filled with all
 * the already defined logical names corresponding to the given search criteria
 * defined through the function iFCriteria
 * @param iFCritere
 *      pointer on a fonction to search the DLName. If NULL then all DLName
 *      are retrieved.The first argument of this function is the string
 * @param iCompare
 *	the first argument of iFCritere, used as comparaison string
 * @param oNbDLName
 *	the number of defined logical names 
 * @param oTabDLName
 *	the array to be created.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetDLNameList ( int(*iFCriteria)( CATUnicodeString*,
								 CATUnicodeString*),
					       CATUnicodeString* iCompar,
					       unsigned int *oNbDLname,
					       CATUnicodeString ***oTabDLName);



/** @nodoc
 * Retrieves all the defined logical names.
 * <br><b>Role</b>: Allocates an array of CATUnicodeString to be filled with all
 * the already defined logical names inside the given father DLname.
 * @param iDLName
 *	the Name of the father DLName
 * @param oNbDLName
 *	the number of defined logical names 
 * @param oTabDLName
 *	the array to be created.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
*/
ExportedByJS0STDLIB HRESULT CATGetDLNameSubList ( CATUnicodeString* iDLName,
						  unsigned int *oNbDLname,
						  CATUnicodeString ***oTabDLName);
  

/**
 * Creates the logical Path for a file.
 * <br><b>Role</b>:Creates the logical Path of a file that will be use
 * by the CATIA files'soft. The resulting string will be interpreted by
 * calls such  as CATFileInfo CATFileAccess etc...
 * @param iDLName
 *	the logical name.
 * @param iFileName
 *	the file's name. Null in order to make the DLPath for a directory
 * @param oLogicalPath
 *	the logical path of the file.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
 */
ExportedByJS0STDLIB HRESULT CATMakeLogicalPath ( const CATUnicodeString *iDLName, 
						 const CATUnicodeString *iFileName,
						 CATUnicodeString **oLogicalPath);



/**
 * Retrieves the physical path for a file.
 * <br><b>Role</b>:Retrieves the physical path of a file from a logical path
 * that has been previously constructed by CATMakeLogicalPath.
 * This method should not be used by application and will probably be defined
 * in PrivateInterfaces.
 * @param iDLName
 *	the logical name.
 * @param iFileName
 *	the file's name. Null in order to make the DLPath for a directory
 * @param oLogicalPath
 *	the logical path of the file.
 * @param oReserved
 *	not to be used. Reserved for internal use.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
 */
ExportedByJS0STDLIB HRESULT CATGetRealPath(const CATUnicodeString *iLogicalPath, 
					   CATUnicodeString **oPhysicalPath,
					   void ** oReserved=0);


/**
 * Retrieves the logical printable path for a file.
 * <br><b>Role</b>:Retrieves the logical printable path of a file from a logical
 * path that has been previously constructed by CATMakeLogicalPath.
 * This method should be used to display a logical path made with 
 * CATMakeLogicalPath
 * @param iLPath
 *	the logical path obtained with CATMakeLogicalPath
 * @param oPrintablePath
 *	the printable view of the  logical path
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
 */
ExportedByJS0STDLIB HRESULT CATGetPrintablePath(const CATUnicodeString *iLPath, 
						CATUnicodeString **oPrintablePath);

/** @nodoc
 * Retrieves the logical printable path for a file.
 * <br><b>Role</b>:Retrieves the logical printable path of a file from a logical
 * path that has been previously constructed by CATMakeLogicalPath.
 * This method should be used to display a logical path made with 
 * CATMakeLogicalPath
 * @param iLPath
 *	the logical path obtained with CATMakeLogicalPath
 * @param oPrintablePath
 *	the url view of the  logical path
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>S_OK :</tt>  on Success 
 *	<br><tt>E_FAIL  :</tt>on failure
 */
ExportedByJS0STDLIB HRESULT CATGetURLPath(const CATUnicodeString *iLPath, 
					  CATUnicodeString **oUrlPath);


/** @nodoc*/
ExportedByJS0STDLIB  HRESULT CATCleanDLName ();

/** @nodoc*/
ExportedByJS0STDLIB HRESULT CATReversePathToDLName( CATUnicodeString *iPath, 
						    CATUnicodeString **oDLName);
  
  
#ifdef __cplusplus
}
#endif
#endif
