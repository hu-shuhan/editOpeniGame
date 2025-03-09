//===========================================================================
// COPYRIGHT DASSAULT SYSTEMES 2003                                         
//===========================================================================
//                                                                            
//  CATISysLicenseSettingAtt                                                
//                                                                           
//  Usage Notes: Interface Definition                                          
//===========================================================================
//  Creation fevrier  2003                                 adt         	
//===========================================================================
#ifndef __CATISYSLICENSESETTINGATT
#define __CATISYSLICENSESETTINGATT
#include "JS0LIB0.h"
#include "CATBaseUnknown.h"   

// COPYRIGHT DASSAULT SYSTEMES 2003
/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

class CATUnicodeString;
class CATSettingInfo;


#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0LIB0 IID IID_CATISysLicenseSettingAtt;
#else
extern "C" const IID IID_CATISysLicenseSettingAtt;
#endif



/**
* Interface to handle the licensing settings.
* <b>Role</b>: This interface is implemented by a component which
* represents the controller of the static Licenses. 
* <br>To access this property page:
* <li>Click the Options command in the Tools menu</li>
* <li>Click General</li>
* <li>Click the Licensing Property Page</li>
* <br>This interface defines:
* <li>A method to set each License</li>
* <li>A method to get the value of each License</li>
* <li>A method to lock/unlock each parameter</li>
* <li>A method to retrieve the information concerning each parameter</li>
*/
class ExportedByJS0LIB0 CATISysLicenseSettingAtt : public CATBaseUnknown
{
  CATDeclareInterface; 
public :

/**
* Retrieves server time out .
* <br><b>Role</b>:  Retrieves the value of the parameter describing the 
* licensing server time out in minutes. For more information about the range and maximum, refers to the 
* Infrastructure documentation.
* @param oTimeOut
*     Licensing server time out.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetServerTimeOut (float &oTimeOut)=0;
  

/**
* Sets server time out .
* <br><b>Role</b>:  Sets the value of the parameter describing the
* licensing server time out in minutes. For more information about the range and maximum, refers to the 
* Infrastructure documentation.
* @param iTimeOut
*     Licensing server time out.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetServerTimeOut (float iTimeOut)=0;


/** 
* Retrieves information about the TimeOut setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT GetServerTimeOutInfo (CATSettingInfo *oInfo)=0;

  
/** 
* Locks or unlocks the TimeOut setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT SetServerTimeOutLock (unsigned char iLock)=0;


/**
* Retrieves nodelock expiry alert .
* <br><b>Role</b>:  Retrieves the value of the parameter describing the 
* nodelock expiry alert in day. For more information about the range and maximum, refers to the 
* Infrastructure documentation.
* @param oAlert
*     Licensing server time out.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetNodelockAlert (long &oAlert)=0;
  

/**
* Sets nodelock expiry alert .
* <br><b>Role</b>:  Sets the value of the parameter describing the
* nodelock expiry alert in days. For more information about the range and maximum, refers to the 
* Infrastructure documentation.
* @param iAlert
*     Nodelock expiry alert.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetNodelockAlert(long iAlert)=0;


/** 
* Retrieves information about the nodelock expiry alert setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT GetNodelockAlertInfo (CATSettingInfo *oInfo)=0;

  
/** 
* Locks or unlocks the nodelock expiry alert setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT SetNodelockAlertLock (unsigned char iLock)=0;


/**
* Retrieves the server contact frequency .
* <br><b>Role</b>:  Retrieves the value of the parameter describing the 
* licensing server contact frequency in minutes. Note that a null value represents the maximum
* contact frequency value. For more information about the range and maximum, refers to the 
* Infrastructure documentation.
* @param oFrequency
*     Licensing server contact frequency.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetFrequency (float &oFrequency)=0;
  

/**
* Sets the server contact frequency.
* <br><b>Role</b>:  Sets the value of the parameter describing the 
* licensing server contact frequency in minutes. Note that a null value represents the maximum
* contact frequency value.For more information about the range and maximum, refers to the 
* Infrastructure documentation.
* @param iFrequency
*     Licensing server contact frequency.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetFrequency (float iFrequency)=0;


/** 
* Retrieves information about the Frequency setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT GetFrequencyInfo (CATSettingInfo *oInfo)=0;

  
/** 
* Locks or unlocks the Frequency setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT SetFrequencyLock (unsigned char iLock)=0;




/**
* Retrieves if licenses have to be shown .
* <br><b>Role</b>:  Retrieves the value of the parameter describing if complete 
* license information has to be shown.
* When the parameter is set, the user gets more information about the reason of the failure to request a license.
* @param oRight
*     TRUE if the information has to be shown.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetShowLicense (unsigned char &oRight)=0;


/**
* Sets the value of the parameter ShowLicense.
* <br><b>Role</b>:  Sets the value of the parameter that discribing if complete
* license information has to be shown.
* When the parameter is set, the user gets more information about the reason of the failure to request a license.
* @param iRight
*     TRUE if the information has to be shown.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetShowLicense (unsigned char iRight)=0;


/** 
* Retrieves information about the ShowLicense setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT GetShowLicenseInfo (CATSettingInfo *oInfo)=0;

/** 
* Locks or unlocks the ShowLicense setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT SetShowLicenseLock (unsigned char iLock)=0;
  

/**
* Retrieves the List of the requested or locked Licenses .
* <br><b>Role</b>: Retrieves the list of the Licenses.
* @param oNbLic
*   The number of License
* @param iDefaultLicenses
*  If iDefaultLicenses!=0 and the settings are empty, returns the default licenses,
*  that is, the visible nodolocked licenses
*  If iDefaultLicenses == 0 and the settings are empty, returns the selected licenses
* (not yet stored, because not yet validated by OK button). 
* @param oTabLic
*	The array of Licenses.
*   You are responsible of the deallocation of oTabLic:
*	<br> if (oTabLic) 
*	<br>{
*	<br>for (int Index = 0; Index < oNbLic; Index++)
*	<br>delete oTabLic[Index];
*	<br>delete [] oTabLic;
*	<br>oTabLic = NULL;
*	<br>}
*	<br>character meaning inlicense name:
*   <br>"_": internal notation for a license configuration
*   <br>"+": you chose "Any license" mode, example of returned value: _ME1.slt+FS1
*	<br>When the return value is a serial number (_ME1.slt_SerialNumber), you have chosen the "Explicit"
*	license mode. In this case the add on product is not indicated in the license name.
*
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT GetLicensesList (int & NbLic, long iDefaultLicenses, CATUnicodeString **&oTabLic) =0;


/** 
* Retrieves information about the LicensesList setting parameter.
* <br><b>Role</b>: Retrieves information about the LicensesList setting locking state (global lock for the LicensesList).
* It is used to get the lock status of the List of the Licenses.
* If the LicensesList is locked all the licenses are locked.
* When the licenses are locked, it means that an administrator has locked the attribute.
* It does not means that an administrator has changed the value of the attribute.
* The value of the setting is not updatable because it refers to a lock on a list. 
* That is why the return value is false. 
* @param oInfo:
* Address of an object CATSettingInfo.
* <br>Information returned in the dump:
* <li>Parameter 1 : "Value taken in case of reset" : useless. Default value : "Default value" </li>
* <li>Parameter 2 : "Locking state" value : unlocked / locked / locked at Admin Level n </li>
* <li>Parameter 3 : "Returned value" : useless, default value : False </li>
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT GetLicensesListInfo (CATSettingInfo * oInfo)=0;
  
  
/** 
* Locks or unlocks the LicensesList setting parameter.
* <br><b>Role</b>:Locks or unlocks the LicensesList setting parameter.
* Locks or unlocks the parameter describing the list of 
* installed licenses, if the  operation is allowed in the current
* administrated environment.
* It is the global lock on all the licenses.
* In user mode  this method will always return E_FAIL.
* When the LicenseList is locked all the licenses are locked.
* When the LicenseList is unlocked all the licenses are unlocked.
* @param iLock
*	the locking operation to be performed:
*	<br><tt>1 :</tt>   to lock the parameter.
* 	<br><tt>0:</tt>   to unlock the parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT SetLicensesListLock ( unsigned char iLock)=0;


/**
* Retrieves the value of the license.
* <br><b>Role</b>: Retrieves the mapping between a name of a license and 
* the value of the license. 
* The license does not need to be returned by GetLicensesList().
* 
* @param iLicense
*  the name of the license: "PMG.prd", "_MD2.slt+", "_MD2.slt+GSD" for example.
*  <br>"PMG.prd" represent the license of the product PMG.
*  <br>"_MD2.slt+" represent the license of the solution MD2.
*  <br>"_MD2.slt+GSD" represent the license of the solution MD2, with the
*  AddOn product GSD.
* @param oValue
*	the value of the License:
*	<br><tt> NotRequested :</tt>   License is not Requested.
* 	<br><tt> key :</tt>  the name of the license, the default available license has been chosen by the user. License is Requested.
*   <br><tt> License Number :</tt> a specific license number has been chosen by the user. License is Requested.
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure. A problem occured during setting file access. 
*/
  virtual HRESULT GetLicense(const CATUnicodeString *iLicense, 
			     CATUnicodeString *oValue)=0;


/**
* Sets the License.
* <br><b>Role</b>: Sets the value of the license.
* @param iLicense
*  the name of the license: "PMG.prd", "_MD2.slt+", "_MD2.slt+GSD" for example.
*  "PMG.prd" represent the license of the product PMG.
*  "_MD2.slt+" represent the license of the solution MD2.
*  "_MD2.slt+GSD" represent the license of the solution MD2, with the
*  AddOn product GSD.
* @param iValue
*	the value of the License:
*	<br><tt> NotRequested :</tt>   License is not Requested
* 	<br><tt> key :</tt>  the name of the license, the default available license has been chosen by the user. License is Requested.
*  <br><tt> License Number :</tt> a specific license number has been chosen by the user. License is Requested.
* 
* @return
*	<b>Legal values</b>:
*	<br><tt>S_OK :</tt>   on success
* 	<br><tt>E_FAIL:</tt>  on failure
*/
  virtual HRESULT SetLicense(const CATUnicodeString *iLicense, 
			     const CATUnicodeString *iValue)=0;


/** 
* Retrieves information about the License setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT GetLicenseInfo (const CATUnicodeString *iLicense,
				  CATSettingInfo* oInfo)=0;


/** 
* Locks or unlocks the License setting parameter.
* <br>Refer to @href CATSysSettingController for a detailed description.
*/
  virtual HRESULT SetLicenseLock (const CATUnicodeString *iLicense,
				  unsigned char iLock)=0;


  
};




#endif


