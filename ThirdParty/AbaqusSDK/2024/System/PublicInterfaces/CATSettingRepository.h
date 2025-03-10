#ifndef CATSETTINGREPOSITORY_H
#define CATSETTINGREPOSITORY_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0SETT.h"
#include "CATEventSubscriber.h"
#include "CATCallbackManager.h"
#include "CATUnicodeString.h"
#include "CATListOfCATUnicodeString.h"
#include "CATLib.h"


class CATUuid;
class CATString;
class CATUnicodeString;
class CATDLName;
class CATSettingInfo;
class CATISysSettingController;
class CATSettingAttribute;
class CATSettingEnv;
class CATSysSettingController;
class DSYSettingSessionManager;

/**
 * Maximun length for  the name of the Repositories and their attributes.
 */
#define MAXSETTINGNAME 255
/**
 * @nodoc 
 */
#define MAXSETTINGPATH  CATMaxPathSize




/**
 * Class for managing the user's customization.
 * <br><b>Role</b>: CATSettingRepositories manage aggregates of setting attributes
 * that are used to store and retrieve values and parameters set by the user. 
 * They are stored between working sessions in one setting file. 
 * <p>
 * Each CATSettingRepository has a unique name used by the application to access
 * it. Each attribute within one CATSettingRepository is identified by its name 
 * and type. 
 * <p>
 * Setting attributes can store the values of simple types such as integers or
 * floats. In addition, a marshallable object should be saved as a setting 
 * attribute with less or no impact on the application. 
 * <p>
 * A CATSettingRepository instance can be saved in memory for temporary storage
 * using a commit/rollback mechanism, and in a file for persistent storage. 
 */
class ExportedByJS0SETT CATSettingRepository : public CATEventSubscriber
{
  CATDeclareClass;
  
public:
  /** @nodoc */
  friend class CATMarshSettingRepository; 
  /** @nodoc */
  friend class CATSysSettingController;
  /** @nodoc */
  friend class CATSysPreferenceRepository;
   /** @nodoc */
  friend class CATSysSettingRepository; 
   /** @nodoc */
  friend class CATIntSetting; 
 /** @nodoc */
  friend class DSYSettingAdminSessionManager;

  /** @nodoc */
  friend HRESULT ExportedByJS0SETT CATSysLogonDone(char * iProviderName);
  /** @nodoc */
  friend HRESULT ExportedByJS0SETT CATSysPrepareLogOff(int iFlag);
  /** @nodoc */
  friend HRESULT ExportedByJS0SETT CommitSettings(int iFlag);

  
  
  enum AccessType { Usr, Prj };
  
  
  //------------------------
  // Events Declaration
  //------------------------
  /** @nodoc */
  CATDeclareCBEvent(Attribute_Deleted);
  /** @nodoc */
  CATDeclareCBEvent(Attribute_Renamed);
  /** @nodoc */
  CATDeclareCBEvent(Attribute_Created);
  /** @nodoc */
  CATDeclareCBEvent(Repository_Modified);

 /**
  * @nodoc
  * The constructors of the class must not be used. 
  * To Create a CATSettingRepository, one must use GetRepository()
*/
  CATSettingRepository(); 
/**
 * @nodoc
*/
  CATSettingRepository(const char* iRepositoryName, int iMode=0, 
		       AccessType iType=Prj, 
		       CATISysSettingController *iControler=NULL);


/**
 * Creates a CATSettingRepository.
 * <br><b>Role</b>: Creates the unique instance of the class 
 * CATSettingRepository named with the given argument. If a setting file 
 * exists, it is read.
 * <br><b>Lifecycle rules deviation</b>: The Applications must not call release
 * on the returned pointer.
 * @param iRepositoryName
 *	The name of the CATSettingRepository used by the applications 
 *	to retrieve it.
 * @param iMode 
 *	Reserved for internal use. Must always be set to 0.
 *	<b>Legal values</b>: The length of the string must be less than or 
 *	equal to @href MAXSETTINGNAME.
 * @param iType 
 *	Defines the type of the setting.
 * @param iCtrl 
 *	pointer to the controler Interfaces.
*/
  static CATSettingRepository* GetRepository(const char* iRepositoryName,
					     int iMode=0,AccessType iType=Prj, 
					     CATISysSettingController *iCtrl=0); 


 
/** @nodoc */
  static long SetMode (char  iMode ='\0');
/** @nodoc */
  static long GetMode (char &oMode);
  
/** @nodoc */
  virtual ~CATSettingRepository();

/** @nodoc */
  CATSettingRepository & operator=( CATSettingRepository& Object) ;

 

/**
 * Retrieves the value of a setting attribute.
 * <br><b>Role</b>: Retrieves using its name, the value of an attribute 
 * constituted by one CATBaseUnknown. A pointer on a intialized and constructed
 * object of type CATBaseUnknown must be given as argument, because this object 
 * is used to set the default value of the attribute, if it does not yet exist. 
 * In this last case the method calls WriteSetting () to initialize the setting
 * attribute.
 * @param iSettingName
 *	The name of the attribute to be retrieved or created.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	A pointer on an initialized CATBaseUnknow. If the attribute has not
 *	been yet created, then <tt>ioSetting</tt> is used as default, otherwise
 *	<tt>ioSetting</tt> is overwritten by the retrieved value.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success 
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, CATBaseUnknown * ioSetting);

/**
 * Sets the value of a setting attribute.
 * <br><b>Role</b>: Sets the value of an attribute of type CATBaseUnknown
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than
 *	or equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	An initialized CATBaseUnknow. 
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, CATBaseUnknown *iSetting, int iFlag=0);


/**
 * Retrieves the value of a setting attribute which is an array of CATBaseUnknown.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of pointers on 
 * constructed CATBaseUnknown, because it is used to set the default value of 
 * the attribute, if it does not yet exist. In this last case the method calls
 * WriteSetting () to initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than
 *	or equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of pointers on initialized CATBaseUnknow.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iSize
 *	The number of elements to read.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, CATBaseUnknown ** ioSetting, 
		   long iSize);

/**
 * Sets the value of a setting attribute which is an array of CATBaseUnknown.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of pointers on constructed 
 * CATBaseUnknown.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of pointeur on initialized CATBaseUnknow.
 * @param iSize
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, CATBaseUnknown **iSetting, 
		    long iSize, int iFlag=0);



/**
 * Retrieves the value of a setting attribute.
 * <br><b>Role</b>: Retrieves using its name, the value of an attribute 
 * constituted by one CATString. A pointer on a intialized and constructed 
 * object of type CATString must be given as argument, because this object is 
 * used to set the default value of the attribute, if it does not yet exist. 
 * In this last case the method calls WriteSetting () to initialize the settings 
 * attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	A pointer on a construted CATString. If the attribute has not
 *	been yet created, then <tt>ioSetting</tt> is used as default values,
 *	otherwise <tt>ioSetting</tt> is overwritten by the retrieved value.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, CATString * ioSetting);

/**
 * Sets the value of a setting attribute.
 * <br><b>Role</b>: Sets the value of an attribute of type CATString
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than
 *	or equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	pointer on a constructed CATString. 
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, CATString * iSetting, int iFlag=0);



/**
 * Retrieves the value of a setting attribute which is an array of CATString.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of pointers 
 * on constructed CATString, because it is used to set the default value of 
 * the attribute, if it does not yet exist. In this last case the method calls
 * WriteSetting () to initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of pointeur on constructed CATString.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iSize
 *	The number of elements to read
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, CATString ** ioSetting, long iSize);

/**
 * Sets the value of a setting attribute which is an array of CATString.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of pointers on constructed 
 * CATString.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of pointeur on initialized CATString.
 * @param iSize
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, CATString ** iSetting, long iSize, int iFlag=0);


 
/**
 * Retrieves the value of a setting attribute.
 * <br><b>Role</b>: Retrieves using its name, the value of an attribute 
 * constituted by one CATUnicodeString. A pointer on a intialized and constructed
 * object of type CATUnicodeString must be given as argument, because this object
 * is used to set the default value of the attribute, if it does not yet exist.
 * In this last case the method calls WriteSetting () to initialize the setting 
 * attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	A pointer on a construted CATString. 
 *	If the attribute has not been yet created, then <tt>ioSetting</tt>
 *	is used as default value, otherwise <tt>ioSetting</tt> is overwritten 
 *	by the retrieved value.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Succes
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, CATUnicodeString * ioSetting);

/**
 * Sets the value of a setting attribute.
 * <br><b>Role</b>: Sets the value of an attribute of type CATUnicodeString
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	pointer on a constructed CATUnicodeString. 
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, CATUnicodeString * iSetting, int iFlag=0);




 
/**
 *@nodoc
 */
  long ReadSetting(const char* iSettingName, CATDLName * ioDL);
  
/**
 *@nodoc
 */
  long WriteSetting(const char* iSettingName, CATDLName * iDL, int iFlag=0);




/**
 * Retrieves the value of a setting attribute which is an array of 
 * CATUnicodeString.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of pointers on 
 * constructed CATUnicodeString, because it is used to set the default value of 
 * the attribute, if it does not yet exist. In this last case the method calls
 * WriteSetting () to initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than 
 *	or equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of pointeur on constructed CATUnicodeString.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iSize
 *	The number of elements to read.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, CATUnicodeString ** ioSetting, 
		   long iSize);
/**
  *@nodoc
*/
long ReadSetting(const char* iSettingName, CATUnicodeString ** ioSetting, 
		   long iSize, int iDef);
/**
 * Sets the value of a setting attribute which is an array of CATUnicodeString.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of pointers on constructed 
 * CATUnicodeString.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of pointeur on initialized CATUnicodeString.
 * @param iSize
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, CATUnicodeString ** iSetting, 
		    long iSize, int iFlag =0);
 


/**
 * Retrieves the value of a setting attribute which is an array of char.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of initialized
 * char because it is used to set the default value of the attribute, if it does
 * not yet exist. In this last case the method calls WriteSetting () to initialize 
 * the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized char.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, char * ioSetting, long iLength=1);


/**
 * Sets the value of a setting attribute which is an array of chars.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized char.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized chars.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, const char *iSetting, long iLength=1, int iFlag=0);



/**
 * Retrieves the value of a setting attribute which is an array of doubles.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of 
 * initialized doubles because it is used to set the default value of the 
 * attribute, if it does not yet exist. In this last case the method calls
 * WriteSetting () to initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized doubles.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, double * ioSetting, long iLength=1);

/**
 * Sets the value of a setting attribute which is an array of doubles.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized doubles.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized doubles.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, double *iSetting, long iLength=1, int iFlag=0);


/**
 * Retrieves the value of a setting attribute which is an array of floats.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * floats because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WriteSetting () to 
 * initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized floats.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, float * ioSetting, long iLength=1);

/**
 * Sets the value of a setting attribute which is an array of floats.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized floats.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized floats.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, float *iSetting, long iLength=1, int iFlag=0);


/**
 * Retrieves the value of a setting attribute which is an array of ints.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * ints because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WriteSetting () to 
 * initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized ints.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
 */
  long ReadSetting(const char* iSettingName, int * ioSetting, long iLength=1);

/**
 * Sets the value of a setting attribute which is an array of ints.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized ints.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized ints.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, int * iSetting, long iLength=1, int iFlag=0);



/**
 * @nodoc 
 * 64 bits
 * Retrieves the value of a setting attribute which is an array of longs.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of initialized 
 * longs because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WriteSetting () to 
 * initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized longs.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, long * ioSetting, long iLength=1);

/**
 * @nodoc 
 * 64 bits
 * Sets the value of a setting attribute which is an array of longs.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized longs.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized longs.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, long * iSetting, long iLength=1, int iFlag=0);


/**
 * Retrieves the value of a setting attribute which is an array of shorts.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * shorts because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WriteSetting () to 
 * initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized shorts.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, short * ioSetting, long iLength=1);
/**
 * Sets the value of a setting attribute which is an array of shorts.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized shorts.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized shorts.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, short *iSetting, long iLength=1, int iFlag=0);



/**
 * Retrieves the value of a setting attribute which is an array of unsigned ints.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * unsigned ints because it is used to set the default value of the attribute, 
 * if it does not yet exist. In this last case the method calls WriteSetting () to 
 * initialize the setting attribute.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized unsigned ints.
 *	If the attribute has not been yet created, then the elements of
 *	<tt>ioSetting</tt> are used as default values, otherwise the elements of 
 *	<tt>ioSetting</tt> are overwritten by the retrieved values.
 * @param iLength
 *	The number of elements to read.
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>>0:</tt>  on Success returns the number of elements really read
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long ReadSetting(const char* iSettingName, unsigned int* ioSetting, 
		   long iLength=1);
/**
 * Sets the value of a setting attribute which is an array of unsigned ints.
 * <br><b>Role</b>: Sets the values of each element of the array constituting the
 * attribute. The array is defined as an array of initialized unsigned ints.
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	an array of initialized unsigned ints.
 * @param iLength
 *	The size of the array.
 * @param iFlag
 *	reserved for internal use. do not use other values than the default one.
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  long WriteSetting(const char* iSettingName, unsigned int* iSetting, 
		    long iLength=1, int iFlag=0);
 



  

/**
 * Retrieves the lock's state of a given attribute within the 
 * CATSettingRepository.
 * <br><b>Role</b>:Retrieves the state of the lock of a given attribute, in order
 * to give it to the @href CATDlgLock#ViewLock method. 
 * An attribute can have been  lock by an CATIA administrator, using the Tools 
 * Options UI. This methods and @href #Lock and @href #Unlock should be 
 * used when coding a Tools Option Dialog.
 * @param iAttributeName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than 
 *	or equal to  @href MAXSETTINGNAME.
 * @param oLock
 *	the state of the lock.
 * @return
 * <b>Legal values</b> 0 OK or -1 no <tt>iAttributeName</tt> was found.
*/	
  long GetLock ( const char* iAttributeName, char & oLock);


/**
 * Retrieves the infos for a given attribute within the 
 * CATSettingRepository.
 * <br><b>Role</b>:Retrieves the infos for a given attribute within 
 * the CATSettingRepository.
 * @param iAttributeName
 *	The name of the attribute.
 *	<br><b>Legal values</b>: The length of the string must be less than 
 *	or equal to  @href MAXSETTINGNAME.
 * @param oInfo
 *	the info Structure.
 * @return
 * <b>Legal values</b> 0 OK or -1 no <tt>iAttributeName</tt> was found.
*/	
  long GetInfo ( const char* iAttributeName, CATSettingInfo* oInfo, int iFlag=0);
  
/**
 *@nodoc
 */
  long GetInfo (char** iAttList, int iNbAtt, CATSettingInfo** oInfo, int iFlag=0);
  

/**
 * Locks a given attribute within the CATSettingRepository.
 * <br><b>Role</b>: Locks a given attribute, in order to prevent any further
 * modifications of its value. This methods is only applicable in Administrator's
 * mode, otherwise it does nothing.
 * This methods and @href #UnLock and @href #GetLock should be used when
 * coding a Tools Option Dialog.
 * @param iAttributeName
 *	The name of the attribute to be locked.
 *	<br><b>Legal values</b>: The length of the string must be less than
 *	or equal to  @href MAXSETTINGNAME.
 * @return
 * <b>Legal values</b> 0 if the attribute was successfully locked, otherwise -1.
*/	
  long Lock  ( const char* iAttributeName);

/**
 * Unlocks a given attribute within the CATSettingRepository.
 * <br><b>Role</b>: Unlocks a given attribute, in order to authorize  further
 * modifications of its value. This methods is only applicable in Administrator's
 * mode, otherwise it does nothing, especially if the attribute is locked.
 * This methods and <tt>Lock()</tt> and <tt>GetLock</tt> should be used when coding a Tools
 * Option Dialog
 * @param iAttributeName
 *	The name of the attribute to unlocked.
 *	<br><b>Legal values</b>: The length of the string must be less than 
 *	or equal to  @href MAXSETTINGNAME.
 * @return
 * <b>Legal values</b> 0 if the attribute was successfully unlocked, otherwise -1.
*/	
  long Unlock ( const char* iAttributeName);

/**
 * Gives the state of a CATSettingRepository.
 * <br><b>Role</b>: Returns the global state of the CATSettingRepository, which
 * is said protected if all its attributes are locked and thus could not be 
 * modified.
 * @return
 * <b>Legal values</b> 1 if all attributes are currently locked, otherwise 0.
*/	
  long IsProtected();
  
/**
 * Makes a persistent copy of the CATsettingRepository in Memory.
 * <br><b>Role</b>: Makes a persistent copy of the CATsettingRepository in Memory
 * The values of the attributes could be restore after that by doing a call to
 * @href #Rollback. 
 * @param iNoEvent
 *	In order to emit the commit event
 *	<br><b>Legal values</b>: 0 to emit the event (the default) and 1 for no emission
 * @return
 * <b>Legal values</b> 1 on Success, otherwise -1.
*/
  long Commit(int iNoEvent=0); 

/**
 * Restores a previous persistent copy of the CATsettingRepository in Memory.
 * <br><b>Role</b>: Restores the persistent memory copy of the 
 * CATsettingRepository, done by the lastest call to @href #Commit .
 * All modifications that have occured on any attributes of the 
 * CATSettingRepository, since the lastest call to Commit,  are discarded.
 * @return
 * <b>Legal values</b> 0 on Success, otherwise -1.
*/
  long Rollback();

/**
 * Makes a persistent copy of the CATsettingRepository on file.
 * <br><b>Role</b>: Makes first a persistent copy of the CATsettingRepository 
 * in Memory by calling @href #Commit , then writes on file the persistent 
 * representation
 * @param iReserved
 *	reserved for internal use. Not to be used.
 * @return
 * <b>Legal values</b> 0 on Success, otherwise negative.
*/
  long SaveRepository(const char* iReseved=NULL); 
 


/**
 * Restores the administrated values of the attributes.
 * <br><b>Role</b>: Restores the administrated values of all the attributes of 
 * the CATSettingRepository. 
 * @return
 * <b>Legal values</b> 1 on Success, otherwise negative.
*/
  long ResetToAdminValues();

/**
 * @nodoc
*/
  long ResetToAdminValues(char** iAttList, int iNbAtt);
  
/**
 * @nodoc
*/
  void GetName(char * oRepositoryName);
/**
 * @nodoc
 */
  long GetState (char ** oStateInfo);
  

/**
 * Indicates the existence of a given attribute.
 * <br><b>Role</b>: Searches within the CATSettingRepository for an attribute
 * named <tt>iAttributeName</tt> and of type <tt>iAttributeClass</tt>.
 * @param iAttributeName
 *	The name of the attribute to be set.
 *	<b>Legal values</b>: The length of the string must be less than or equal 
 *	to @href MAXSETTINGNAME.
 * @param iAttributeClass
 *	The type of the requested attribute, that could be "int", "float",...
 * @return
 *	 <b>Legal values</b> the number of elements constituting the attribute, or 0 if not found,
 *       -1 if the attribute has is default value, -2 found with another type.
 *       If the attribute has its default value only a ReadSetting can determine the real number
 *       of elements constituting the attribute.
 *      
*/
  long IsPresent( const char *iAttributeName ,const char *iAttributeClass );

/**
 * @nodoc
*/    
  long DelAttribute ( const char *iAttributeName);
/**
 * @nodoc
*/    
  long RenameAttribute(const char *iCurrent_AttributeName, 
		       const char *iNew_AttributeName);
 
 
/**
 * Retrieves the origine of an attribute's value.
 * <br><b>Role</b>: Retrieves the level of administration where the given 
 * attribute has been locked or if the attribute is not locked the level that 
 * will give its value to the attribute during the reset process.
 * @param iAttributeName
 *	The name of the attribute.
 * @param oOrigineLevel
 *       the Path of the level of administration
 * @return
 *	<b>Legal values</b> 0 on success -1 in cas of failure. 
 */
 
  long OrigineValue(const char * iAttributeName, CATString **oOrigineLevel);
  
  

/**
 * Browses the CATSettingRepository.
 * <br><b>Role</b>: Returns the Name, the type and the number of constituting 
 * elements of an attribute. The strings are copied but their allocation
 * must be done by the caller. 
 * @param oAttributeName
 *	a pointer to an allocated buffer where the attribute's name will be copied
 * @param oAttributeClass
 *	a pointer to an allocated buffer where the attribute's type will be copied
 * @param oAttributeSize
 *	a pointer to a long int to put the number of constituting elements
 * @param iReset
 *	iReset = 1 forces to begin with the first Attribute,reseting the state
 *	reached with the last call to this method, otherwise 0
 * @return
 *	<b>Legal values</b> 0 until reaching the last Attribute and returning -1 
 *	when the last attribute is reached
*/
  long NextAttribute( char *oAttributeName, char *oAttributeClass, 
		      long *oAttributeSize,
		      short iReset = 0); 


/**
 * @nodoc
*/
  HRESULT GetController( CATISysSettingController **oCtrl); 

/**
 * @nodoc
*/
  AccessType GetAccessType () { return _Type;};
  
      
/**
 * @nodoc
*/
  static long CleanSettings ();
  

protected:
/**
 * @nodoc
 * Method giving the address of the  buffer where the data from an Attribute
 * are saved  to be used when implementing a derivated class
*/
  long WhereToUnstream(const char* iAttributeName, const char* iAttributeClass,
		       char & oMachineType, 
		       long & oVersion,
		       char *& oAddressBuffer, 
		       long& oBufferLength, long& oNbElem);
  
 /**
 * @nodoc
 * Method inserting in the Repository the datas of an Attribute saved in a 
 * buffer from adress iBufferAddress and length iBufferLength
 * to be used when implementing a derivated class
*/
  long StreamedAt (const char* iAttributeName, const char* iAttributeClass,
		   char * iBufferAddress, long iBufferLength, long iSize=1);

/**
 * @nodoc
*/
  CATCallbackManager * GetCallbackManager();


/**
 * @nodoc
*/
  static CATSettingRepository* _RootObject;  
/**
 * @nodoc
*/
  CATSettingRepository* _Next; 
/**
 * @nodoc
*/
  CATSettingRepository* _Previous; 
/**
 * @nodoc
*/
  CATSettingAttribute* _FirstAttribute; 
/**
 * @nodoc
*/
  CATSettingAttribute* _CurrentAttribute;  
/**
 * @nodoc
*/ 
  char* _ObjectId; 
/**
 * @nodoc
*/
  int _Protected;
/**
 * @nodoc
*/
  int _Version;
/**
 * @nodoc
*/
  int _Ind;	
/**
 * @nodoc
 * Used for versionning purpose.....
*/	     
  CATUuid *_Uuid;
  char * _CreationBuild;
  char * _ModifBuild;
   


private: 
/**
 * @nodoc
*/	       
  long CopyForReset();
/**
 * @nodoc
*/	       
  HRESULT GetAttrToUnstream(const char* iAttributeName, const char* iAttributeClass,
			    CATSettingAttribute ** oAttribute);
/**
 * @nodoc
*/	     
  long StreamedAt (const char* iAttributeName, const char* iAttributeClass,
		   char * iBufferAddress, long iBufferLength, 
		   char * iObj, unsigned long iLg, long iSize=1, 
		   int iMode=0, int iInitAtt =0);

/**
 * @nodoc
*/	     
  long Copie( CATSettingAttribute* , CATSettingAttribute* );
/**
 * @nodoc
*/	     
  CATBaseUnknown * ReadRepository(CATBaseUnknown * oSetting, char* Filename); 
/**
 * @nodoc
*/	     
  long WriteRepository(CATBaseUnknown * ObjToMrsh, char * Filename) ;
/**
 * @nodoc
*/	     
  CATBaseUnknown * UnstreamTab(CATBaseUnknown ** oSetting, char* StreamBuffer,int length,
			   char machine, long Version, long &Size); 
/**
 * @nodoc
*/	     
  char * StreamTab(CATBaseUnknown **ObjToMrsh, long &lg_Buffer, long Size) ;
/**
 * @nodoc
*/	     
  CATString * UnstreamTab(CATString ** oSetting, char* StreamBuffer,int lenght,
			   char machine, long Version, long &Size); 
/**
 * @nodoc
*/	     
  char * StreamTab(CATString **ObjToMrsh, long &lg_Buffer, long Size) ;
/**
 * @nodoc
*/	      
  CATUnicodeString * UnstreamTab(CATUnicodeString ** oSetting, char* StreamBuffer,int length,
			   char machine, long Version, long &Size); 
/**
 * @nodoc
*/	     
  char * StreamTab(CATUnicodeString **ObjToMrsh, long &lg_Buffer, long Size) ;


/**  @nodoc */	    
  CATCallbackManager *_Dispatcher;
/** @nodoc */	     
  static CATSettingEnv *_CATRefPath;
/** @nodoc */	   
  static CATSettingEnv *_CATUserPath;
/** @nodoc */
  CATSysSettingController *_Ctrl;

/** 
 * @nodoc 
 * version for the level of marshalling
*/	    
  static int _MrshlVersion;
/**
 * @nodoc 
 * Administrative or user mode
*/	   
  static char _Mode ;
/**
 * @nodoc
 * Current state of the object
*/
  int _State;
/**
 * @nodoc 
*/	
  AccessType _Type;
  
/** @nodoc */
  static DSYSettingSessionManager * _pSessionManager; 
  
  static HRESULT ReloadSettings(CATListOfCATUnicodeString iSettingsToReload, 
				CATListOfCATUnicodeString iSettingsNeedingRestart, int iReloadLevel );
  
}; 




/**
 * @nodoc
 * Notfication send by the @href CATSettingRepository.
 * <br><b>Role</b>: Objets of this class convey informations about the modifications that
 * have been occured inside a CATSettingRepository which send event when it is commited
*/
class ExportedByJS0SETT CATSettingNotif : public CATNotification
{
  CATDeclareClass;
  
public:
  CATSettingNotif ();
  CATSettingNotif ( CATCallbackEvent iEvent, const char *iAttributeName);
  virtual ~CATSettingNotif();
/**
 * @nodoc
*/
  CATCallbackEvent *_Evenement;
/**
 * @nodoc
*/
  char *_Attribute;
};

#endif





