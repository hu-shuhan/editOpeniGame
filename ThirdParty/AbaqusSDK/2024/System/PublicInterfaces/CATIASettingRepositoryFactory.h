// COPYRIGHT DASSAULT SYSTEMES 2005
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
#ifndef CATIASettingRepositoryFactory_h
#define CATIASettingRepositoryFactory_h

#include "JS0SETT.h"

class CATIASettingRepository;
class CATSysSettingRepository;

/**
 * Instanciates a CATIASettingRepository object.
 * <br><b>Role</b>: Creates an instance of the interface  CATIASettingRepository 
 * named with the given argument. 
 * @param iName
 *	The name of the CATIASettingRepository used by the applications 
 *	to retrieve it.
 *	<b>Legal values</b>: The length of the string must be less than or 
 *	equal to @href MAXSETTINGNAME.
 * @param oCtrl 
 *	a pointer to a CATIASettingRepository object.
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
HRESULT ExportedByJS0SETT CATSysInstantiateSettingRepository (const char *iName, 
						CATIASettingRepository **oCtrl);

/**
 * Instanciates a CATIASettingRepository object.
 * <br><b>Role</b>: Creates an instance of the interface  CATIASettingRepository 
 * from the CATSysSettingRepository object. 
 * @param iCtrl
 *	a valid pointer to a CATSysSettingRepository object
 * @param oCtrl 
 *	a pointer to a CATIASettingRepository object.
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
HRESULT ExportedByJS0SETT CATSysInstantiateSettingRepository (
					 CATSysSettingRepository *iCtrl, 
					 CATIASettingRepository **oCtrl);


/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of integers.
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created. To be used with settings only.
 * @param iObjectArray
 * a pointer to a C++ array of integers.
 * @param iSize
 * the size of the integers array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0SETT CATSafeArrayVariant *BuildSafeArrayVariant( const int *iArray,
							      CATLONG32 iSize);
/**
 * Creates a new CATSafeArrayVariant and initializes it with
 * a C++ array of unsigned integers
 * <br> Use the global function @href FreeVariantSafeArray to free
 * the <tt>CATSafeArrayVariant</tt> created. To be used with settings only
 * @param iObjectArray
 * a pointer to a C++ array of unsigned integers.
 * @param iSize
 * the size of the unsigned int array.
 * @return
 * a pointer to an initialized <tt>CATSafeArrayVariant</tt>.
 */
ExportedByJS0SETT CATSafeArrayVariant *BuildSafeArrayVariant( 
					    const unsigned int *iArray, 
					    CATLONG32 iSize);
#endif
