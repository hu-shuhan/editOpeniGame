

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

#ifndef _WINDOWS_SOURCE

#ifndef __IErrorInfo
#define __IErrorInfo

#include "IUnknown.h"

#ifndef _WINDOWS_SOURCE
#include "CATBSTR.h"
typedef CATBSTR  BSTR;
typedef IID  GUID; 
#endif



extern IID IID_IErrorInfo;
class IErrorInfo : public IUnknown
{

    CATDeclareInterface;
 public:
  
   virtual HRESULT __stdcall GetDescription( BSTR* opbstrDescription )=0;
   virtual HRESULT __stdcall GetSource( BSTR* opbstrSource )=0;
   virtual HRESULT __stdcall GetHelpFile( BSTR* opbstrHelpFile )=0;
   virtual HRESULT __stdcall GetHelpContext( DWORD* opbstrHelpContext )=0;
   virtual HRESULT __stdcall GetGUID( GUID* opguid )=0;

};
#endif
#else
#pragma once
#include <objidl.h>
#endif
