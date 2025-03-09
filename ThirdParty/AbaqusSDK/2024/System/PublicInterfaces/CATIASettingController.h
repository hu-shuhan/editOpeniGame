#ifndef CATIASettingController_h
#define CATIASettingController_h
// COPYRIGHT DASSAULT SYSTEMES 2003
/**
 * @CAA2Level L1
 * @CAA2Usage U6
 */

#include "CATIABase.h"
#include "CATSafeArray.h"
#include "JS0SETT.h"

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0SETT IID IID_CATIASettingController;
#else // LOCAL_DEFINITION_FOR_IID
extern "C" const IID IID_CATIASettingController;
#endif // LOCAL_DEFINITION_FOR_IID

/**
 * Base interface to handle a setting controller for Automation.
 * <b>Role</b>: A setting controller manages the values of all or
 * only a part of the attributes available in a property page of the dialog
 * displayed using the Options command of the Tools menu.
 * <p>CATIASettingController supplies the methods common to all setting controllers
 * for the setting controller Automation implementation:</p>
 * <ul>
 *  <li><code>Commit</code> to make a memory copy of the setting attribute values</li>
 *  <li><code>Rollback</code> to restore the last memory copy of the setting attribute values</li>
 *  <li><code>ResetToAdminValues</code> to restore the administrated values of all the attributes</li>
 *  <li><code>ResetToAdminValuesByName</code> to restore the administrated values of a subset of the attributes</li>
 *  <li><code>SaveRepository</code> to make a persistent copy of the setting attribute values on file</li>
 * </ul>
 * <p>CATIASettingController must be implemented, through a derived interface that stands for
 * the setting controller for Automation, by a V5 component, as follows:
 * <ul>
 *  <li>Create an interface that derives from CATIASettingController with the
 *      methods appropriate to manage the attributes handled by the setting controller</li>
 *  <li>Create a class, data extension of your setting controller V5 component,
 *      that derives from @href CATSysAutoSettingController class
 *      to implement this interface. Usually the methods that manage the attributes should simply
 *      call their counterpart in your implementation of the interface derived from 
 *      the C++ @href CATISysSettingController interface.
 *      In addition, @href CATSysAutoSettingController supplies the implementation
 *      of the five methods of CATIASettingController.
 *      In most cases, you should not re-implement these methods, except if you create
 *      a bufferized setting controller. Refer to the CAA Encyclopedia to know
 *      about such controllers.</li>
 * </ul>
 * <p>Your setting controller V5 component should in addition implement the C++ interface
 * @see CATSysAutoSettingController, CATISysSettingController, CATSysSettingController
 */
class ExportedByJS0SETT CATIASettingController : public CATIABase
{
  CATDeclareInterface;
  
public:
  
/**
 * Makes a memory copy of the setting attribute values.
 * <br><b>Role</b>: <code>Commit</code> saves the current values of the setting attributes
 * managed by the setting controller in a specific memory area.
 * Successive calls to <code>Commit</code> overwrite the memory area.
 * The values saved by the last call to <code>Commit</code> can be restored from that memory area
 * using the @href #Rollback method.
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
  virtual HRESULT __stdcall Commit()=0;

/**
 * Restores the last memory copy of the setting attribute values.
 * <br><b>Role</b>: <code>Rollback</code> restores the values of the 
 * setting attributes managed by the setting controller from the
 * memory area.
 * All values of the setting attributes managed by the setting controller modified since the last call
 * to @href #Commit are restored to the values they had when this last @href #Commit was called.
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
  virtual HRESULT __stdcall Rollback()=0;
 
/**
 * Restores the administrated values of all the attributes.
 * <br><b>Role</b>: <code>ResetToAdminValues</code> restores all
 * the values of the setting attributes managed by the setting controller
 * to either the values set by the setting administrator, or to their default values
 * if the setting administrator did not change them. 
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
  virtual HRESULT __stdcall ResetToAdminValues()=0;

/**
 * Restores the administrated values of a subset of the attributes.
 * <br><b>Role</b>: <code>ResetToAdminValuesByName</code> restores
 * the values of a subset of the setting attributes managed by the setting controller
 * to either the values set by the setting administrator, or to their default values
 * if the setting administrator did not change them. 
 * @param [in] iAttList
 *  The attribute subset to which the administrated values are to be restored
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
  virtual HRESULT __stdcall ResetToAdminValuesByName(const CATSafeArrayVariant & iAttList)=0;
 
/**
 * Makes a persistent copy of the setting attribute values on file.
 * <br><b>Role</b>: <code>SaveRepository</code> saves the current values of the
 * setting attributes managed by the setting controller in a setting repository file.
 * To avoid inconsistencies, <code>SaveRepository</code> first saves the values in the memory area
 * used by the @href #Commit method by calling @href #Commit before writing the values
 * in the setting repository file.
 * @return
 * <b>Legal values</b>: S_OK on success, and E_FAIL otherwise
 */
  virtual HRESULT __stdcall SaveRepository()=0;
    
};

CATDeclareHandler(CATIASettingController, CATIABase);

#include "CATBSTR.h"
#include "CATBaseDispatch.h"
#include "CATBaseUnknown.h"
#include "CATVariant.h"
#include "IDispatch.h"
#include "IUnknown.h"


#endif
