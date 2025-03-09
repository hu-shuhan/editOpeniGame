#ifndef ITypeInfo_h
#define ITypeInfo_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

#include "IUnknown.h"
#include "CATBSTR.h"
#include "CATNTTypes.h"

typedef struct tagEXCEPINFO EXCEPINFO;

interface ITypeComp;
interface ITypeLib;

extern IID IID_ITypeInfo;

interface ITypeInfo : public IUnknown {
 public:
  /* read the description of the typeinfo only */
  virtual HRESULT GetTypeAttr(TYPEATTR ** ppTypeAttr) = 0;
  
  virtual HRESULT GetTypeComp(ITypeComp ** ppTComp) = 0;
  
  /* read the description of a function of the typeinfo */
  virtual HRESULT GetFuncDesc(unsigned int index, FUNCDESC ** ppFuncDesc) = 0;
  
  virtual HRESULT GetVarDesc(unsigned int index, VARDESC ** ppVarDesc) = 0;

  /* read name of a function and names of parameters */  
  virtual HRESULT GetNames(MEMBERID memid, 
			   CATBSTR * rgCATBSTRNames,
			   unsigned int cMaxNames,
			   unsigned int * pcNames) = 0;
  
  /* give a handle to father class */
  virtual HRESULT GetRefTypeOfImplType(unsigned int index,
				       HREFTYPE * pRefType) = 0;
  
  virtual HRESULT GetImplTypeFlags(unsigned int index,
				   int * pImplTypeFlags) = 0;
  
  virtual HRESULT GetIDsOfNames(CATLPOLESTR *rgszNames,
				unsigned int cNames,
				MEMBERID * pMemId) = 0;
  
  virtual HRESULT Invoke(void * pvInstance,
			 MEMBERID memid,
			 WORD wFlags,
			 DISPPARAMS * pDispParams,
			 VARIANT * pVarResult,
			 EXCEPINFO * pExcepInfo,
			 unsigned int * puArgErr) = 0;
  
  virtual HRESULT GetDocumentation(MEMBERID memid,
				   CATBSTR * pBstrName,
				   CATBSTR * pBstrDocString,
				   DWORD   * pdwHelpContext, 
				   CATBSTR * pBstrHelpFile) = 0;
  
  virtual HRESULT GetDllEntry(MEMBERID memid,
			      INVOKEKIND invKind,
			      CATBSTR * pBstrDllName,
			      CATBSTR * pBstrName,
			      WORD    * pwOrdinal) = 0;
  
  /* give a type info from a handle */
  virtual HRESULT GetRefTypeInfo(HREFTYPE hRefType,
				 ITypeInfo ** ppTInfo) = 0;
  
  virtual HRESULT AddressOfMember(MEMBERID memid,
				  INVOKEKIND invKind,
				  void ** ppv) = 0;
  
  virtual HRESULT CreateInstance(IUnknown * pUnkOuter,
				 const IID & riid,
				 void ** ppvObj) = 0;
  
  virtual HRESULT GetMops(MEMBERID memid,
			  CATBSTR * pBstrMops) = 0;
  
  virtual HRESULT GetContainingTypeLib(ITypeLib ** ppTLib,
				       unsigned int *pIndex) = 0;
  
  virtual void ReleaseTypeAttr(TYPEATTR * pTypeAttr) = 0;
  
  virtual void ReleaseFuncDesc(FUNCDESC * pFuncDesc) = 0;
  
  virtual void ReleaseVarDesc(VARDESC * pVarDesc) = 0;
  
};

#else
#pragma once
#include <objidl.h>
#endif
#endif
