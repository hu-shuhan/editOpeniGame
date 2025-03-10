#ifndef CATIScriptMethodCall_h
#define CATIScriptMethodCall_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

#include "JS0LOGRP.h"
#include "CATBaseUnknown.h"
#include "CATBoolean.h"
#include "CATBSTR.h"
#include "CATVariant.h"
#include "CATSafeArray.h"
#include "CATScriptParameterDirection.h"

class CATBaseDispatch;
class CATString;

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0LOGRP IID IID_CATIScriptMethodCall;
#else
extern "C" const IID IID_CATIScriptMethodCall;
#endif

/**
 * Interface to represent a method call used for the macros generation.
 * <b>Role:</b>
 * A method call consists in :
 * <MENU>
 * <LI>a target on which the method will be invoked,
 * <LI>a method name,
 * <LI>a list of parameters (one of this parameters can be the returned value).
 * </MENU>
 * @see CATIScriptJournal, CATScriptParameterDirection
 */
class ExportedByJS0LOGRP CATIScriptMethodCall : public CATBaseUnknown {
  CATDeclareInterface;

  public :

		/**
		 * Sets the target on which the method will be invoked.
		 * @param iTarget
		 * The specified target.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT SetTarget(const CATBaseDispatch * iTarget) = 0;

		/**
		 * Sets the method name to invoke.
		 * @param iMethodName
		 * The specified method name.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT SetMethodName(const CATString & iMethodName) = 0;

		/**
		 * Adds to the method to invoke a boolean parameter with the specified direction.
		 * @param iBoolean
		 * The specified boolean parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(CATBoolean iBoolean, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a char parameter with the specified direction.
		 * @param iChar
		 * The specified char parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(char iChar, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a short parameter with the specified direction.
		 * @param iShort
		 * The specified short parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(short iShort, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a CATLONG parameter with the specified direction.
		 * @param iLong
		 * The specified CATLONG parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(CATLONG iLong, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke an unsigned long parameter with the specified direction.
		 * @param iULong
		 * The specified unsigned long parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(unsigned long iULong, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a float parameter with the specified direction.
		 * @param iFloat
		 * The specified float parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(float iFloat, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a double parameter with the specified direction.
		 * @param iDouble
		 * The specified double parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(double iDouble, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a string parameter with the specified direction.
		 * @param iString
		 * The specified string parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(const char * iString, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a CATBSTR parameter with the specified direction.
		 * @param iBSTR
		 * The specified CATBSTR parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(const CATBSTR & iBSTR, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a CATBaseDispatch parameter with the specified direction.
		 * @param iObject
		 * The specified CATBaseDispatch parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(const CATBaseDispatch * iObject, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a CATVariant parameter with the specified direction.
		 * @param iVariant
		 * The specified CATVariant parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(const CATVariant & iVariant, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

		/**
		 * Adds to the method to invoke a CATSafeArray parameter with the specified direction.
		 * @param iSafeArray
		 * The specified CATSafeArray parameter.
		 * @param iDirection
		 * The specified direction of the parameter.
		 * @param iReturnedParam
		 * The boolean iReturnedParam must be set to TRUE if this parameter is the returned
		 * parameter of the method (each method accepts at most one returned parameter).
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT AddParameter(const CATSafeArray & iSafeArray, CATScriptParameterDirection iDirection, CATBoolean iReturnedParam = FALSE) = 0;

};

#endif

