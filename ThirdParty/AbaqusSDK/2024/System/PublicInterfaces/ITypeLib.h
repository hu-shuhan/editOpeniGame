#ifndef ITypeLib_h
#define ITypeLib_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

#include "IUnknown.h"
#include "CATUnicodeString.h"
#include "CATBSTR.h"
#include "CATNTTypes.h"

interface ITypeInfo;
interface ITypeComp;

extern IID IID_ITypeLib;

interface ITypeLib : public IUnknown {
 public:
 
	/* give number of typeinfos of the typelib)*/
  virtual unsigned int GetTypeInfoCount() = 0;
        
	/* give a typeinfo with its number in the typelib)*/
  virtual HRESULT GetTypeInfo(unsigned int index,
															ITypeInfo ** ppTInfo) = 0;
        
	/* give the type of the type info of the given number*/
  virtual HRESULT GetTypeInfoType(unsigned int index,
				    											TYPEKIND * pTKind) = 0;
        
	/* give the type info from its GUID */
  virtual HRESULT GetTypeInfoOfGuid(const GUID & guid, 
  																	ITypeInfo ** ppTinfo) = 0;
        
  virtual HRESULT GetLibAttr(TLIBATTR ** ppTLibAttr) = 0;
        
  virtual HRESULT GetTypeComp(ITypeComp ** ppTComp) = 0;

  virtual HRESULT GetDocumentation(int index,
																   CATBSTR * pBstrName,
																   CATBSTR * pBstrDocString,
																   DWORD   * pdwHelpContext,
																   CATBSTR * pBstrHelpFile) = 0;
        
  virtual HRESULT IsName (CATLPOLESTR szNameBuf, 
												  ULONG lHashVal, 
												  int * pfName) = 0;

  virtual HRESULT FindName(CATLPOLESTR szNameBuf,
												   ULONG lHashVal,
												   ITypeInfo ** ppTInfo,
												   MEMBERID  * rgMemId,
												   unsigned short * pcFound) = 0;
        
  virtual void ReleaseTLibAttr(TLIBATTR * pTLibAttr) = 0;
        
};

#ifndef CATMAINWIN
HRESULT LoadTypeLib(const CATLPOLESTR szFile, ITypeLib ** tplib);
#else /* CATMAINWIN */
extern "C" HRESULT CATMWLoadTypeLib(const wchar_t * szFile, ITypeLib ** tplib);
#endif /* CATMAINWIN */

void ExecCall(int numinfo, const char * TypeLibName, void ** tabvar);
void ExecCall(const CATUC2Bytes * nameinfo, const char * TypeLibName, void ** tabvar);

#else
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <objidl.h>
#endif
#endif
