/**
 *  @quickReview EGD-JAC 01:11:29
 */
#ifndef CATSETTINGINFO_H
#define CATSETTINGINFO_H

// COPYRIGHT DASSAULT SYSTEMES 2001
/**
 * @CAA2Level L1
 * @CAA2Usage U1
*/


/** @nodoc */
typedef enum tagSettingInfo
{
  SETINFO_MAXINFO =0,
  SETINFO_NONAME =1,
  SETINFO_LOCK = 2
} SETTINGINFO;


#include "JS0SETT.h"
#include "IUnknown.h"
class CATSettingInfo;
class CATSysSettingController;
class CATString;
class CATSettingRepository;
class CATSysSettingRepository;

/** @nodoc */
typedef HRESULT (CATSysSettingController::*CATSysSettingCtrlInfoMethod)(CATSettingInfo *oInfo);

/** @nodoc */
typedef HRESULT (CATSysSettingController::*CATSysSettingCtrlLockingMethod)(unsigned char iLock);


/**
 * Defines the information corresponding to an attribute.
 * <br><b>Role</b>: Defines the informations that can be used by Tools Options' dialog
 * objects.
*/
class ExportedByJS0SETT CATSettingInfo
{
public:
 /** @nodoc */
  friend class CATSettingRepository;
 /** @nodoc */
  friend class CATSysSettingRepository;
  /** @nodoc */
  friend class CATIntSetting;

  /** 
   * Constructs an instance. 
   * In order to give the different values embedded in this class, you need to use the
   * @href CATSettingRepository#GetInfo method.
   */
  CATSettingInfo ();
 
  /**
   * Copy constructor
   */
  CATSettingInfo(const CATSettingInfo& iOriginal);

  /**
    * Equal operator
    */
  CATSettingInfo& operator = (const CATSettingInfo&);

  /**
   * Equality operator.
   * @param iInfo
   *   The SettingInfo to compare to the current one
   * @return 
   *   boolean
   *   <br><b>Legal values</b>: <tt>0: False</tt> 
   *   the condition is not fullfilled, or <tt>Other: True</tt> 
   *   if the condition is fullfilled.
   */
   int operator == ( const  CATSettingInfo& iInfo) const ;
 
  virtual ~CATSettingInfo ();

  /**
   * Retrieves the lock state of the attribute encapsulated in the CATSettingInfo.
   * @param oLock
   *	the state of the lock.
   * @return
   *  S_OK if the CATSettingInfo is correctly implemented, and the lock attribute 
   *  is correctly set.
   *  E_FAIL otherwise.
   */
  HRESULT GetLock(char& oLock);

  /**
   * Locks the encapsulated attribute into the encapsulated CATSettingRepository,
   * in order to prevent any further modifications of its value. 
   * This methods is only applicable in Administrator's mode. Otherwise it does nothing.
   * @return
   *  S_OK if the attribute was successfully locked
   *  E_FAIL otherwise.
   */
  HRESULT Lock();

  /**
   * Unlocks the encapsulated attribute into the encapsulated CATSettingRepository,
   * in order to allow further modifications of its value. 
   * This methods is only applicable in Administrator's mode. Otherwise it does nothing.
   * @return
   *  S_OK if the attribute was successfully unlocked
   *  E_FAIL otherwise.
   */
  HRESULT Unlock();

  /**
   * Retrieves the origine of the encapsulated attribute's value. 
   * <br><br>If the encapsulated attribute has been locked, this is the level 
   * of administration where the lock is located,
   * <br>If the attribute is not locked, this is the level that would impose 
   * its value to the encapsulated attribute in case of a reset.
   * @param oOrigineValue
   *       the path of the level of administration
   * @return
   *	S_OK on success
   *  E_FAIL if something went wrong
   */
  HRESULT GetOrigineValue(CATString*& oOrigineValue);

  /** @nodoc */
  void SetFunc ( CATSysSettingCtrlInfoMethod iInfoMeth, 
		 CATSysSettingCtrlLockingMethod iLockMeth,
		 CATSysSettingController *iCtrl, const char *iParamName);
  

  /** @nodoc */
  char _Lock;
  /** @nodoc */
  char _Lck;
  /** @nodoc */
  char _Explicit;
  /** @nodoc */
  short _Lv;
  /** @nodoc */
  short _LvLck;
  /** @nodoc */
  CATString * _Level;
  /** @nodoc */
  CATSysSettingCtrlLockingMethod  _LockFunc;
  /** @nodoc */
  CATSysSettingCtrlInfoMethod _InfoFunc;
  /** @nodoc */
  CATSysSettingController *_Ctrl;
 
private:
  /**
   * @nodoc
   * Sets the name of the _Name data. 
   * Internal use only. Refer to the @href CATSettingRepository#GetInfo method, 
   * if you wish to build a CATSettingInfo.
   * @return 
   *  S_OK if the method correctly worked. 
   *   This includes the case where iAttributeName is NULL.
   *  E_FAIL if there has been a problem in the initialization of the data.
   * @param iAttributeName
   *  Name of the attribute embedded in this class.
   */
  HRESULT SetName(const char* iAttributeName);

  /**
   * @nodoc
   * Returns the name of the attribute embedded in this class.
   * @return 
   *  S_OK if the method correctly worked. 
   *   This includes the case where the attribute is NULL.
   *  E_FAIL if there has been a problem.
   *   This includes the case where the oAttributeName pointer is already allocated.
   * @param oAttributeName
   *  Name of the attribute embedded in this class.
   */
  HRESULT GetName(char*& oAttributeName) const;

  /**
   * @nodoc
   * Sets the Repository of the CATSettingInfo. 
   * Internal use only. Refer to the @href CATSettingRepository#GetInfo method, 
   * if you wish to build a CATSettingInfo.
   * @return 
   *  S_OK if the method correctly worked. 
   *  E_FAIL if there has been a problem in the initialization of the data.
   *   This includes the case where iRepository is NULL.
   * @param iRepository
   *  Pointer to the Repository embedded in this class.
   */
  HRESULT SetRepository(const CATSettingRepository* iRepository);

  /**
   * @nodoc
   * Returns a pointer to the Repository embedded in this class.
   * IMPORTANT: Never delete or release the returned pointer.
   * @return 
   *  S_OK if the method correctly worked. 
   *  E_FAIL if there has been a problem.
   *   This includes the case where the Repository is NULL.
   * @param oRepository
   *  Pointer to the Repository embedded in this class.
   */
  HRESULT GetRepository(CATSettingRepository*& oRepository) const;
  
  HRESULT VB (int iType);
  
  char *_Name;
  CATSettingRepository *_Repository;
  char *_Param;
  CATSysSettingRepository *_SysRepo;

};
#endif



