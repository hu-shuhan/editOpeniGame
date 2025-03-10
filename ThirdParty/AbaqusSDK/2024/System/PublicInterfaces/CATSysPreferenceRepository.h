#ifndef CATSYSPREFERENCEREPOSITORY_H
#define CATSYSPREFERENCEREPOSITORY_H

// COPYRIGHT DASSAULT SYSTEMES 2003

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0SETT.h"
#include "CATEventSubscriber.h"
#include "CATCallbackManager.h"
#include "CATLib.h"
#include "CATDataType.h"

class CATString;
class CATUnicodeString;
class CATSettingRepository;


/**
 * Maximun length for  the name of the Repositories and their attributes.
 */
#define MAXSETTINGNAME 255
/**
 * @nodoc 
 */
#define MAXSETTINGPATH  CATMaxPathSize




/**
 * Class for managing the user's preferences.
 * <br><b>Role</b>: CATSysPreferenceRepositories manage aggregates of attributes
 * that are used to store and retrieve values and parameters set by the user. 
 * They are stored between working sessions in one preference file. 
 * <p>
 * Each CATSysPreferenceRepository has a unique name used by the application to 
 * access it. Each attribute within one CATSysPreferenceRepository is identified
 * by its name and type. 
 * <p>
 * Setting attributes can store the values of simple types such as integers or
 * floats. In addition, a marshallable object should be saved as a setting 
 * attribute with less or no impact on the application. 
 * <p>
 * A CATSysPreferenceRepository instance can be saved in memory for temporary storage
 * using a commit/rollback mechanism, and in a file for persistent storage. 
 */
class ExportedByJS0SETT CATSysPreferenceRepository : public CATEventSubscriber
{
  CATDeclareClass;
public:

/**
 * Creates a CATSysPreferenceRepository.
 * <br><b>Role</b>: Creates the unique instance of the class CATSysPreferenceRepository
 * named with the given argument. If a setting file exists, it is read.
 * @param iPrefName
 *	The name of the CATSysPreferenceRepository used by the applications 
 *	to retrieve it.
*/
  static CATSysPreferenceRepository* GetPreferenceRepository( const char* iPrefName);


/** @nodoc */
  static CATSysPreferenceRepository* GetPreferenceRepositoryEx( const char* iPrefName);

/** @nodoc */
  virtual ~CATSysPreferenceRepository();

 

/**
 * Retrieves the value of a setting attribute.
 * <br><b>Role</b>: Retrieves using its name, the value of an attribute 
 * constituted by one CATBaseUnknown. A pointer on a intialized and constructed
 * object of type CATBaseUnknown must be given as argument, because this object 
 * is used to set the default value of the attribute, if it does not yet exist. 
 * In this last case the method calls WritePreference () to initialize the setting
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
  CATLONG32 ReadPreference(const char* iSettingName, CATBaseUnknown * ioSetting);

/**
 * Sets the value of a setting attribute.
 * <br><b>Role</b>: Sets the value of an attribute of type CATBaseUnknown
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than
 *	or equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	An initialized CATBaseUnknow. 
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, CATBaseUnknown *iSetting);


/**
 * Retrieves the value of a setting attribute which is an array of CATBaseUnknown.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of pointers on 
 * constructed CATBaseUnknown, because it is used to set the default value of 
 * the attribute, if it does not yet exist. In this last case the method calls
 * WritePreference () to initialize the setting attribute.
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
  CATLONG32 ReadPreference(const char* iSettingName, CATBaseUnknown ** ioSetting, 
			   CATLONG32 iSize);

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
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, CATBaseUnknown **iSetting, 
			    CATLONG32 iSize);



/**
 * Retrieves the value of a setting attribute.
 * <br><b>Role</b>: Retrieves using its name, the value of an attribute 
 * constituted by one CATString. A pointer on a intialized and constructed 
 * object of type CATString must be given as argument, because this object is 
 * used to set the default value of the attribute, if it does not yet exist. 
 * In this last case the method calls WritePreference () to initialize the settings 
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
  CATLONG32 ReadPreference(const char* iSettingName, CATString * ioSetting);

/**
 * Sets the value of a setting attribute.
 * <br><b>Role</b>: Sets the value of an attribute of type CATString
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than
 *	or equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	pointer on a constructed CATString. 
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, CATString * iSetting);



/**
 * Retrieves the value of a setting attribute which is an array of CATString.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of pointers 
 * on constructed CATString, because it is used to set the default value of 
 * the attribute, if it does not yet exist. In this last case the method calls
 * WritePreference () to initialize the setting attribute.
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
  CATLONG32 ReadPreference(const char* iSettingName, CATString ** ioSetting, 
			   CATLONG32 iSize);

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
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, CATString ** iSetting, 
			    CATLONG32 iSize);


 
/**
 * Retrieves the value of a setting attribute.
 * <br><b>Role</b>: Retrieves using its name, the value of an attribute 
 * constituted by one CATUnicodeString. A pointer on a intialized and constructed
 * object of type CATUnicodeString must be given as argument, because this object
 * is used to set the default value of the attribute, if it does not yet exist.
 * In this last case the method calls WritePreference () to initialize the setting 
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
  CATLONG32 ReadPreference(const char* iSettingName, CATUnicodeString * ioSetting);

/**
 * Sets the value of a setting attribute.
 * <br><b>Role</b>: Sets the value of an attribute of type CATUnicodeString
 * @param iSettingName
 *	The name of the attribute to be set.
 *	<br><b>Legal values</b>: The length of the string must be less than or 
 *	equal to  @href MAXSETTINGNAME.
 * @param ioSetting
 *	pointer on a constructed CATUnicodeString. 
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, CATUnicodeString * iSetting);



/**
 * Retrieves the value of a setting attribute which is an array of 
 * CATUnicodeString.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of pointers on 
 * constructed CATUnicodeString, because it is used to set the default value of 
 * the attribute, if it does not yet exist. In this last case the method calls
 * WritePreference () to initialize the setting attribute.
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
  CATLONG32 ReadPreference(const char* iSettingName, CATUnicodeString ** ioSetting, 
			   CATLONG32 iSize);

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
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, CATUnicodeString ** iSetting, 
			    CATLONG32 iSize);
  


/**
 * Retrieves the value of a setting attribute which is an array of char.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of initialized
 * char because it is used to set the default value of the attribute, if it does
 * not yet exist. In this last case the method calls WritePreference () to initialize 
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
  CATLONG32 ReadPreference( const char* iSettingName, char * ioSetting, 
			    CATLONG32 iLength=1);


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
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference( const char* iSettingName, char *iSetting, 
			     CATLONG32 iLength=1);



/**
 * Retrieves the value of a setting attribute which is an array of doubles.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of 
 * initialized doubles because it is used to set the default value of the 
 * attribute, if it does not yet exist. In this last case the method calls
 * WritePreference () to initialize the setting attribute.
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
  CATLONG32 ReadPreference(const char* iSettingName, double * ioSetting,
			   CATLONG32 iLength=1);

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
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, double *iSetting, 
			    CATLONG32 iLength=1);


/**
 * Retrieves the value of a setting attribute which is an array of floats.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * floats because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WritePreference () to 
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
  CATLONG32 ReadPreference(const char* iSettingName, float * ioSetting, 
			   CATLONG32 iLength=1);

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
 * @return
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, float *iSetting, 
			    CATLONG32 iLength=1);


/**
 * Retrieves the value of a setting attribute which is an array of ints.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * ints because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WritePreference () to 
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
  CATLONG32 ReadPreference(const char* iSettingName, int * ioSetting, 
			   CATLONG32 iLength=1);

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
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, int * iSetting, 
			    CATLONG32 iLength=1);



/**
 * @nodoc 
 * 64 bits
 * Retrieves the value of a setting attribute which is an array of longs.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the
 * array constituting an attribute. The array is defined as an array of initialized 
 * longs because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WritePreference () to 
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
  CATLONG32 ReadPreference(const char* iSettingName, long * ioSetting, 
			   CATLONG32 iLength=1);

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
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, long * iSetting, 
			    CATLONG32 iLength=1);


/**
 * Retrieves the value of a setting attribute which is an array of shorts.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * shorts because it is used to set the default value of the attribute, if it 
 * does not yet exist. In this last case the method calls WritePreference () to 
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
  CATLONG32 ReadPreference(const char* iSettingName, short * ioSetting, 
			   CATLONG32 iLength=1);

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
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, short *iSetting, 
			    CATLONG32 iLength=1);



/**
 * Retrieves the value of a setting attribute which is an array of unsigned ints.
 * <br><b>Role</b>: Retrieves using its name, the values of each element of the 
 * array constituting an attribute. The array is defined as an array of initialized 
 * unsigned ints because it is used to set the default value of the attribute, 
 * if it does not yet exist. In this last case the method calls WritePreference () to 
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
  CATLONG32 ReadPreference(const char* iSettingName, unsigned int* ioSetting, 
			   CATLONG32 iLength=1);

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
 * @return 
 *	<b>Legal values</b>:
 *	<br><tt>1 :</tt>  on Success
 *	<br><tt>0  :</tt>  if a class conflict on the attribute has occured.
 * 	<br><tt>-1:</tt>  on failure
*/
  CATLONG32 WritePreference(const char* iSettingName, unsigned int* iSetting, 
			    CATLONG32 iLength=1);
 



  


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
  HRESULT SaveRepository(const char* iReseved=NULL); 
 

/**
 * Restores a previous persistent copy of the CATSysPreferenceRepository in Memory.
 * <br><b>Role</b>: Restores the persistent memory copy of the 
 * CATSysPreferenceRepository, done by the lastest call to @href #Commit .
 * All modifications that have occured on any attributes of the 
 * CATSysPreferenceRepository, since the lastest call to Commit,  are discarded.
 * @return
 * <b>Legal values</b> S_OK on Success, otherwise negative.
*/
  HRESULT Rollback();


/**
 * Makes a persistent copy of the CATSysPreferenceRepository in Memory.
 * <br><b>Role</b>: Makes a persistent copy of the CATSysPreferenceRepository in Memory
 * The values of the attributes could be restore after that by doing a call to
 * @href #Rollback. 
 * @return
 * <b>Legal values</b> S_OK on Success, otherwise -1.
*/
  HRESULT Commit(); 


/**
 * Indicates the existence of a given attribute.
 * <br><b>Role</b>: Searches within the CATSysPreferenceRepository for an attribute
 * named <tt>iAttributeName</tt> and of type <tt>iAttributeClass</tt>.
 * @param iAttributeName
 *	The name of the attribute to be set.
 *	<b>Legal values</b>: The length of the string must be less than or equal 
 *	to @href MAXSETTINGNAME.
 * @param iAttributeClass
 *	The type of the requested attribute, that could be "int", "float",...
 * @return
 *	the number of elements constituting the attribute, otherwise negative.
*/
  CATLONG32 IsPresent( const char *iAttributeName ,const char *iAttributeClass );
  

/**
 * Removes a given preference.
 * <br><b>Role</b>: Removes in the CATSysPreferenceRepository the attribute
 * named <tt>iAttributeName</tt>.
 * @param iAttributeName
 *	The name of the attribute to be removed.
 *	<b>Legal values</b>: The length of the string must be less than or equal 
 *	to @href MAXSETTINGNAME.
 * @return
 * <b>Legal values</b> 0 on Success, otherwise negative.
*/
  HRESULT DelPreference(const char *iAttributeName);
  
 
/**
 * Browses the CATSysPreferenceRepository.
 * <br><b>Role</b>: Returns the Name, the type and the number of constituting 
 * elements of an attribute. The strings are copied but their allocation
 * must be done by the caller. 
 * @param oAttributeName
 *	a pointer to an allocated buffer where the attribute's name will be copied
 * @param oAttributeClass
 *	a pointer to an allocated buffer where the attribute's type will be copied
 * @param oAttributeSize
 *	a pointer to a integer to put the number of constituting elements
 * @param iReset
 *	iReset = 1 forces to begin with the first Attribute,reseting the state
 *	reached with the last call to this method, otherwise 0
 * @return
 *	<b>Legal values</b> 0 until reaching the last Attribute and returning 
 *      -1 when the last attribute is reached
*/
  CATLONG32 NextAttribute( char *oAttributeName, char *oAttributeClass, 
			   CATLONG32 *oAttributeSize,
			   short iReset = 0); 


  


private: 
/** @nodoc */
  CATSysPreferenceRepository ();
/** @nodoc */
  CATSettingRepository * _Pref;
/** @nodoc */
  static CATSysPreferenceRepository * _RootPref;
/** @nodoc */
  CATSysPreferenceRepository * _Next;
/** @nodoc */
  CATSysPreferenceRepository * _Prev;
  
}; 



#endif





