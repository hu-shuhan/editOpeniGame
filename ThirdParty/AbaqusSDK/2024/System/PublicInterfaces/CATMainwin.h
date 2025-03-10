#ifndef _CATMainwin_h
#define _CATMainwin_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2001

/* This file declares all required types, APIs and interfaces
 used by automation that were not already declared in the existing
 System header files 
 Use the flag CATMAINWIN to use it.
 cf oleauto.h of Mainwin
*/

#ifndef _WINDOWS_SOURCE

/*---------------------------------------------------------------------*/
/*                   Contents of CATJHNTypeLib.h                       */
/*---------------------------------------------------------------------*/

// Use the header of System
#include "CATJHNTypeLib.h"

/*---------------------------------------------------------------------*/
/*                           Misc macros                               */
/*---------------------------------------------------------------------*/

#ifndef SUCCEEDED
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#endif

#ifndef FAILED
#define FAILED(Status) ((HRESULT)(Status)<0)
#endif

#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE 
#endif

#ifndef __RPC_FAR
#define __RPC_FAR
#endif

#ifndef FAR
#define FAR
#endif

/* Default System and User IDs for language and locale. */

#define SORT_DEFAULT                     0x0     /* // sorting default */
#define LANG_NEUTRAL                     0x00
#define SUBLANG_NEUTRAL                  0x00    /* // language neutral */
#define SUBLANG_DEFAULT                  0x01    /* // user default */
#define SUBLANG_SYS_DEFAULT              0x02    /* // system default */

#define MAKELANGID(p, s)       ((((USHORT)(s)) << 10) | (USHORT)(p))
#define MAKELCID(lgid, srtid)  ((ULONG)((((ULONG)((USHORT)(srtid))) << 16) |  \
                                         ((ULONG)((USHORT)(lgid)))))

#define LANG_SYSTEM_DEFAULT    (MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))

#define LOCALE_SYSTEM_DEFAULT  (MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT))

/*---------------------------------------------------------------------*/
/*                      Contents of IUnknown.h                         */
/*---------------------------------------------------------------------*/

// Use the header of System
#include "IUnknown.h"

/*---------------------------------------------------------------------*/
/*                         Primitive types                             */
/*---------------------------------------------------------------------*/

typedef int BOOL;
typedef DWORD *LPDWORD;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;
typedef double DOUBLE;

typedef short VARIANT_BOOL;
typedef VARIANT_BOOL _VARIANT_BOOL;

typedef LONG SCODE;

typedef CLSID *LPCLSID;

typedef double DATE;

typedef void *PVOID;
typedef void *LPVOID;

typedef LONG DISPID;

/*---------------------------------------------------------------------*/
/*                            CHAR types                               */
/*---------------------------------------------------------------------*/

typedef char CHAR;
typedef unsigned char UCHAR;

typedef wchar_t CATMWOLECHAR;

#ifdef  _UNICODE
typedef wchar_t TCHAR;
#else 
typedef char            TCHAR;
#endif

typedef CHAR *LPSTR;
typedef wchar_t *LPWSTR;

typedef CATMWOLECHAR *CATMWLPOLESTR;
typedef const CATMWOLECHAR *LPCOLESTR;

typedef CATMWOLECHAR *BSTR;

typedef BSTR *LPBSTR;

typedef const CHAR *LPCSTR;

/*---------------------------------------------------------------------*/
/*                          Enums                                      */
/*---------------------------------------------------------------------*/

typedef enum tagTYPEFLAGS
    {	TYPEFLAG_FAPPOBJECT	= 0x1,
	TYPEFLAG_FCANCREATE	= 0x2,
	TYPEFLAG_FLICENSED	= 0x4,
	TYPEFLAG_FPREDECLID	= 0x8,
	TYPEFLAG_FHIDDEN	= 0x10,
	TYPEFLAG_FCONTROL	= 0x20,
	TYPEFLAG_FDUAL	= 0x40,
	TYPEFLAG_FNONEXTENSIBLE	= 0x80,
	TYPEFLAG_FOLEAUTOMATION	= 0x100,
	TYPEFLAG_FRESTRICTED	= 0x200,
	TYPEFLAG_FAGGREGATABLE	= 0x400,
	TYPEFLAG_FREPLACEABLE	= 0x800,
	TYPEFLAG_FDISPATCHABLE	= 0x1000,
	TYPEFLAG_FREVERSEBIND	= 0x2000,
	TYPEFLAG_FPROXY	= 0x4000
    }	TYPEFLAGS;

typedef enum tagFUNCFLAGS
    {	FUNCFLAG_FRESTRICTED	= 0x1,
	FUNCFLAG_FSOURCE	= 0x2,
	FUNCFLAG_FBINDABLE	= 0x4,
	FUNCFLAG_FREQUESTEDIT	= 0x8,
	FUNCFLAG_FDISPLAYBIND	= 0x10,
	FUNCFLAG_FDEFAULTBIND	= 0x20,
	FUNCFLAG_FHIDDEN	= 0x40,
	FUNCFLAG_FUSESGETLASTERROR	= 0x80,
	FUNCFLAG_FDEFAULTCOLLELEM	= 0x100,
	FUNCFLAG_FUIDEFAULT	= 0x200,
	FUNCFLAG_FNONBROWSABLE	= 0x400,
	FUNCFLAG_FREPLACEABLE	= 0x800,
	FUNCFLAG_FIMMEDIATEBIND	= 0x1000
    }	FUNCFLAGS;

typedef enum tagVARFLAGS
    {	VARFLAG_FREADONLY	= 0x1,
	VARFLAG_FSOURCE	= 0x2,
	VARFLAG_FBINDABLE	= 0x4,
	VARFLAG_FREQUESTEDIT	= 0x8,
	VARFLAG_FDISPLAYBIND	= 0x10,
	VARFLAG_FDEFAULTBIND	= 0x20,
	VARFLAG_FHIDDEN	= 0x40,
	VARFLAG_FRESTRICTED	= 0x80,
	VARFLAG_FDEFAULTCOLLELEM	= 0x100,
	VARFLAG_FUIDEFAULT	= 0x200,
	VARFLAG_FNONBROWSABLE	= 0x400,
	VARFLAG_FREPLACEABLE	= 0x800,
	VARFLAG_FIMMEDIATEBIND	= 0x1000
    }	VARFLAGS;

typedef /* [v1_enum] */ 
enum tagLIBFLAGS
    {	LIBFLAG_FRESTRICTED	= 0x1,
	LIBFLAG_FCONTROL	= 0x2,
	LIBFLAG_FHIDDEN	= 0x4,
	LIBFLAG_FHASDISKIMAGE	= 0x8
    }	LIBFLAGS;


/*---------------------------------------------------------------------*/
/*                     Contents of CATNTTypes.h                        */
/*---------------------------------------------------------------------*/

// Use the headers of System
#include "CATNTTypes.h"

/*---------------------------------------------------------------------*/
/*     Contents of TypeDesc.h, ArrayDesc.h, ParamDesc.h, ElemDesc.h,   */
/*     FuncDesc.h, VarKind.h, VarDesc.h, TypeAttr.h and TLibAttr.h     */
/*---------------------------------------------------------------------*/

typedef struct tagVARIANT CATVariant;

// Use the headers of System
#include "TypeDesc.h"
#include "ArrayDesc.h"
#include "ParamDesc.h"
#include "IdlDesc.h"
#include "ElemDesc.h"
#include "FuncDesc.h"
#include "VarKind.h"
#include "VarDesc.h"
#include "TypeAttr.h"
#include "TLibAttr.h"

/*---------------------------------------------------------------------*/
/*                          SAFEARRAY type                             */
/*---------------------------------------------------------------------*/

#include "CATSafeArray.h"

/*---------------------------------------------------------------------*/
/*                          SafeArray API                              */
/*---------------------------------------------------------------------*/

extern "C" HRESULT SafeArrayAllocDescriptor(UINT cDims, 
					    SAFEARRAY ** ppsaOut);
extern "C" HRESULT SafeArrayAllocData(SAFEARRAY * psa);
extern "C" SAFEARRAY * SafeArrayCreate(VARTYPE vt,
				       UINT cDims, 
				       SAFEARRAYBOUND * rgsabound);
extern "C" SAFEARRAY * SafeArrayCreateVector(VARTYPE vt, 
					     LONG lLbound, 
					     ULONG cElements);
extern "C" HRESULT SafeArrayCopyData(SAFEARRAY *psaSource, 
				     SAFEARRAY *psaTarget);
extern "C" HRESULT SafeArrayDestroyDescriptor(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayDestroyData(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayDestroy(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayRedim(SAFEARRAY * psa, 
				  SAFEARRAYBOUND * psaboundNew);
extern "C" UINT SafeArrayGetDim(SAFEARRAY * psa);
extern "C" UINT SafeArrayGetElemsize(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayGetUBound(SAFEARRAY * psa, 
				      UINT nDim, 
				      LONG * plUbound);
extern "C" HRESULT SafeArrayGetLBound(SAFEARRAY * psa, 
				      UINT nDim, 
				      LONG * plLbound);
extern "C" HRESULT SafeArrayLock(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayUnlock(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayAccessData(SAFEARRAY * psa, 
				       void** ppvData);
extern "C" HRESULT SafeArrayUnaccessData(SAFEARRAY * psa);
extern "C" HRESULT SafeArrayGetElement(SAFEARRAY * psa, 
				       LONG * rgIndices, 
				       void * pv);
extern "C" HRESULT SafeArrayPutElement(SAFEARRAY * psa, 
				       LONG * rgIndices, 
				       void * pv);
extern "C" HRESULT SafeArrayCopy(SAFEARRAY * psa, 
				 SAFEARRAY ** ppsaOut);
extern "C" HRESULT SafeArrayPtrOfIndex(SAFEARRAY * psa, 
				       LONG * rgIndices, 
				       void ** ppvData);

/*---------------------------------------------------------------------*/
/*                           VARIANT type                              */
/*---------------------------------------------------------------------*/

#include "CATVariant.h"

interface IUnknown;
typedef IUnknown *LPUNKNOWN;
interface IDispatch;
interface IRecordInfo;

/*---------------------------------------------------------------------*/
/*                           VARIANT API                               */
/*---------------------------------------------------------------------*/

typedef VARIANT *LPVARIANT;
typedef VARIANT VARIANTARG;

#define V_VT(X)          ((X)->vt)

extern "C" void VariantInit(VARIANTARG * pvarg);
extern "C" HRESULT VariantClear(VARIANTARG * pvarg);
extern "C" HRESULT VariantCopy(VARIANTARG * pvargDest, 
			       VARIANTARG * pvargSrc);
extern "C" HRESULT VariantCopyInd(VARIANT * pvarDest, 
				  VARIANTARG * pvargSrc);
extern "C" HRESULT VariantChangeType(VARIANTARG * pvargDest,
				     VARIANTARG * pvarSrc, 
				     USHORT wFlags, 
				     VARTYPE vt);
extern "C" HRESULT VariantChangeTypeEx(VARIANTARG * pvargDest,
				       VARIANTARG * pvarSrc, 
				       LCID lcid, 
				       USHORT wFlags, 
				       VARTYPE vt);
extern "C" UINT VarCmp(LPVARIANT pvarLeft, 
		       LPVARIANT pvarRight, 
		       LCID lcid);

/* // Return values for VarCmp */
#define VARCMP_LT 0
#define VARCMP_EQ 1
#define VARCMP_GT 2
#define VARCMP_NULL 3

/*---------------------------------------------------------------------*/
/*                    Contents of IDispatch.h                          */
/*---------------------------------------------------------------------*/

// Use the header of System
// Wait to change to wcahr_t ** for GetIDsOfNames
#ifndef CATMAINWIN
#include "IDispatch.h"
#else /* CATMAINWIN */

#ifndef __IDispatch
#define __IDispatch

typedef wchar_t CATWCHAR_T;

#include "IUnknown.h"

#ifndef _WINDOWS_SOURCE

//
// Uuid of IDispatch
//
extern const GUID IID_IDispatch;

// forward
interface ITypeInfo;

typedef struct tagVARIANT VARIANT;
typedef struct tagDISPPARAMS DISPPARAMS;
typedef struct tagEXCEPINFO EXCEPINFO;

//
// Definition of the IDispatch interface for UNIX
//
/**
 * Base interface for all Automation interfaces.
 * <b>Role</b>: All Automation interfaces derive from <tt>IDispatch</tt> which replaces for UNIX the
 * native Microsoft(R) <tt>IDispatch</tt> interface.
 * This interface supplies basic methods to be Microsoft(R) Automation compliant.
 */
interface IDispatch : public IUnknown
{
	//
	// Give the number of TypeInfo that the object provides
    //
	/** @nodoc */
	virtual HRESULT __stdcall GetTypeInfoCount(unsigned int *oNbOfTypeInfo)
									    = 0;
	
	//
	// Retrieves the type information for an object, which can 
	// then be used to get the type information for an interface. 
	//
    /** @nodoc */
	virtual HRESULT __stdcall GetTypeInfo(unsigned int iIndex,
					      ULONG iLangId, 
					      ITypeInfo **oPtTypeInfo)=0;
	
	//
	// Maps a single member and an optional set of argument names
	// to a corresponding set of integer DISPIDs
	//
    /** @nodoc */
	virtual HRESULT __stdcall GetIDsOfNames(const IID & forFutur,
						CATWCHAR_T **iArrayOfNames,
						unsigned int iNbNames,
						ULONG iLangId,
						LONG *oArrayOflong)=0;
	
	// 
	// Provides access to properties and methods exposed by an object.
	//
	/** @nodoc */
	virtual HRESULT __stdcall Invoke(LONG ilongMember, const IID & forFutur,
					 ULONG iLangId,
					 unsigned short iFlags,
					 DISPPARAMS *iPdisparams,
					 VARIANT *oPvaresult,
					 EXCEPINFO *oPexcepinfo,
					 unsigned int *oPuArgErr)=0;
};

#endif //_WINDOWS_SOURCE

#endif //__IDispatch

#endif /* CATMAINWIN */

/*---------------------------------------------------------------------*/
/*                    Contents of ITypeInfo.h                          */
/*---------------------------------------------------------------------*/

// Use the header of System
#include "ITypeInfo.h"

/* Flags for IDispatch::Invoke */
#define DISPATCH_METHOD         0x1
#define DISPATCH_PROPERTYGET    0x2
#define DISPATCH_PROPERTYPUT    0x4
#define DISPATCH_PROPERTYPUTREF 0x8

/*---------------------------------------------------------------------*/
/*                    Contents of ITypeLib.h                           */
/*---------------------------------------------------------------------*/
  
// Use the header of System
#include "ITypeLib.h"

/*---------------------------------------------------------------------*/
/*                            BSTR API                                 */
/*---------------------------------------------------------------------*/

extern "C" BSTR SysAllocString(const CATMWOLECHAR *);
extern "C" INT SysReAllocString(BSTR *, const CATMWOLECHAR *);
extern "C" BSTR SysAllocStringLen(const CATMWOLECHAR *, UINT);
extern "C" INT SysReAllocStringLen(BSTR *, const CATMWOLECHAR *, UINT);
extern "C" void SysFreeString(BSTR);
extern "C" UINT SysStringLen(BSTR);

extern "C" UINT SysStringByteLen(BSTR bstr);
extern "C" BSTR SysAllocStringByteLen(LPCSTR psz, UINT len);

/*---------------------------------------------------------------------*/
/*                       Other used structs                            */
/*---------------------------------------------------------------------*/

typedef void *HANDLE;
typedef HANDLE HINSTANCE ;
typedef HINSTANCE HMODULE;
typedef HANDLE HWND ;

typedef struct tagDISPPARAMS
 {
 VARIANTARG *rgvarg;
 DISPID *rgdispidNamedArgs;
 UINT cArgs;
 UINT cNamedArgs;
 } DISPPARAMS;

#include "Excepinfo.h"

/*---------------------------------------------------------------------*/
/*                 Active Object Registration API                      */
/*---------------------------------------------------------------------*/

#ifndef REFCLSID
#define REFCLSID            const CLSID &
#endif

typedef  GUID & REFGUID;

#include "CATCreateInstance.h"

/* // class registration flags; passed to CoRegisterClassObject */
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,       /* // class object only generates one instance */
    REGCLS_MULTIPLEUSE = 1,     /* // same class object genereates multiple inst. */
                                /* // and local automatically goes into inproc tbl. */
    REGCLS_MULTI_SEPARATE = 2,  /* // multiple use, but separate control over each */
                                /* // context. */
    REGCLS_SUSPENDED      = 4,  /* // register is as suspended, will be activated */
                                /* // when app calls CoResumeClassObjects */
    REGCLS_SURROGATE      = 8   /* // must be used when a surrogate process */
                                /* // is registering a class object that will be */
                                /* // loaded in the surrogate */
} REGCLS;

extern "C" HRESULT CoGetClassObject(const CLSID & rclsid, 
				    DWORD dwClsContext, 
				    LPVOID pvReserved,
				    const IID & riid, 
				    LPVOID * ppv);
extern "C" HRESULT CoRegisterClassObject(const CLSID & rclsid, 
					 LPUNKNOWN pUnk,
					 DWORD dwClsContext,
					 DWORD flags, 
					 LPDWORD lpdwRegister);
extern "C" HRESULT CoRevokeClassObject(DWORD dwRegister);

/* flags for RegisterActiveObject */
#define ACTIVEOBJECT_STRONG 0x0
#define ACTIVEOBJECT_WEAK 0x1

extern "C" HRESULT RegisterActiveObject(IUnknown * punk,
					REFCLSID rclsid,
					DWORD dwFlags, 
					DWORD * pdwRegister);

extern "C" HRESULT RevokeActiveObject(DWORD dwRegister, 
				      void * pvReserved);

extern "C" HRESULT GetActiveObject(REFCLSID rclsid, 
				   void * pvReserved,
				   IUnknown ** ppunk);

/*---------------------------------------------------------------------*/
/*                      Ole initialization API                         */
/*---------------------------------------------------------------------*/

extern "C" HRESULT CoInitialize(LPVOID pvReserved);
extern "C" void CoUninitialize(void);
extern "C" HRESULT OleInitialize(LPVOID pvReserved);
extern "C" void OleUninitialize(void);

/*---------------------------------------------------------------------*/
/*                        Active Script API                            */
/*---------------------------------------------------------------------*/

/*
extern "C" const GUID CATID_ActiveScript;
extern "C" const GUID CATID_ActiveScriptParse;
extern "C" const GUID IID_IActiveScript;
extern "C" const GUID IID_IActiveScriptParse;
extern "C" const GUID IID_IActiveScriptSite;
extern "C" const GUID IID_IActiveScriptSiteWindow;
extern "C" const GUID IID_IActiveScriptError;
*/
extern "C" GUID CATID_ActiveScript;
extern "C" GUID CATID_ActiveScriptParse;
extern "C" GUID IID_IActiveScript;
extern "C" GUID IID_IActiveScriptParse;
extern "C" GUID IID_IActiveScriptSite;
extern "C" GUID IID_IActiveScriptSiteWindow;
extern "C" GUID IID_IActiveScriptError;


/* IActiveScript::AddNamedItem() input flags */

#define SCRIPTITEM_ISVISIBLE            0x00000002
#define SCRIPTITEM_ISSOURCE             0x00000004
#define SCRIPTITEM_GLOBALMEMBERS        0x00000008
#define SCRIPTITEM_ISPERSISTENT         0x00000040
#define SCRIPTITEM_CODEONLY             0x00000200
#define SCRIPTITEM_NOCODE               0x00000400

#define SCRIPTITEM_ALL_FLAGS            (SCRIPTITEM_ISSOURCE | \
                                         SCRIPTITEM_ISVISIBLE | \
                                         SCRIPTITEM_ISPERSISTENT | \
                                         SCRIPTITEM_GLOBALMEMBERS | \
                                         SCRIPTITEM_NOCODE | \
                                         SCRIPTITEM_CODEONLY)

/* IActiveScript::AddTypeLib() input flags */

#define SCRIPTTYPELIB_ISCONTROL         0x00000010
#define SCRIPTTYPELIB_ISPERSISTENT      0x00000040
#define SCRIPTTYPELIB_ALL_FLAGS         (SCRIPTTEXT_ISCONTROL | SCRIPTTYPELIB_ISPERSISTENT)

/* IActiveScriptParse::AddScriptlet() and IActiveScriptParse::ParseScriptText() input flags */

#define SCRIPTTEXT_DELAYEXECUTION       0x00000001
#define SCRIPTTEXT_ISVISIBLE            0x00000002
#define SCRIPTTEXT_ISEXPRESSION         0x00000020
#define SCRIPTTEXT_ISPERSISTENT         0x00000040
#define SCRIPTTEXT_HOSTMANAGESSOURCE    0x00000080
#define SCRIPTTEXT_ALL_FLAGS            (SCRIPTTEXT_DELAYEXECUTION | \
                                         SCRIPTTEXT_ISVISIBLE | \
                                         SCRIPTTEXT_ISEXPRESSION | \
                                         SCRIPTTEXT_ISPERSISTENT | \
                                         SCRIPTTEXT_HOSTMANAGESSOURCE)

/* IActiveScriptSite::GetItemInfo() input flags */

#define SCRIPTINFO_IUNKNOWN             0x00000001
#define SCRIPTINFO_ITYPEINFO            0x00000002
#define SCRIPTINFO_ALL_FLAGS            (SCRIPTINFO_IUNKNOWN | \
                                         SCRIPTINFO_ITYPEINFO)

/* IActiveScript::Interrupt() Flags */

#define SCRIPTINTERRUPT_DEBUG           0x00000001
#define SCRIPTINTERRUPT_RAISEEXCEPTION  0x00000002
#define SCRIPTINTERRUPT_ALL_FLAGS       (SCRIPTINTERRUPT_DEBUG | \
                                         SCRIPTINTERRUPT_RAISEEXCEPTION)

/* script state values */

typedef enum tagSCRIPTSTATE {
  SCRIPTSTATE_UNINITIALIZED	= 0,
  SCRIPTSTATE_INITIALIZED	= 5,
  SCRIPTSTATE_STARTED	= 1,
  SCRIPTSTATE_CONNECTED	= 2,
  SCRIPTSTATE_DISCONNECTED	= 3,
  SCRIPTSTATE_CLOSED	= 4
} SCRIPTSTATE;

/* script thread state values */

			/* size is 2 */
typedef 
enum tagSCRIPTTHREADSTATE
    {	SCRIPTTHREADSTATE_NOTINSCRIPT	= 0,
	SCRIPTTHREADSTATE_RUNNING	= 1
    }	SCRIPTTHREADSTATE;

/* Thread IDs */

typedef DWORD SCRIPTTHREADID;

#define SCRIPTTHREADID_CURRENT  ((SCRIPTTHREADID)-1)
#define SCRIPTTHREADID_BASE     ((SCRIPTTHREADID)-2)
#define SCRIPTTHREADID_ALL      ((SCRIPTTHREADID)-3)

interface IActiveScriptError;

interface IActiveScriptSite : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetLCID( 
        /* [out] */ LCID __RPC_FAR *plcid) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetItemInfo( 
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti) = 0 ;
    
  virtual HRESULT STDMETHODCALLTYPE GetDocVersionString( 
        /* [out] */ BSTR __RPC_FAR *pbstrVersion)= 0 ;
    
  virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate( 
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo)= 0;
    
  virtual HRESULT STDMETHODCALLTYPE OnStateChange( 
        /* [in] */ SCRIPTSTATE ssScriptState) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE OnScriptError( 
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror)= 0 ;
    
  virtual HRESULT STDMETHODCALLTYPE OnEnterScript( void)= 0;
    
  virtual HRESULT STDMETHODCALLTYPE OnLeaveScript( void)= 0;
    
};

interface IActiveScriptError : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetExceptionInfo( 
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetSourcePosition( 
        /* [out] */ DWORD __RPC_FAR *pdwSourceContext,
        /* [out] */ ULONG __RPC_FAR *pulLineNumber,
        /* [out] */ LONG __RPC_FAR *plCharacterPosition) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetSourceLineText( 
        /* [out] */ BSTR __RPC_FAR *pbstrSourceLine) = 0;
    
};

interface IActiveScriptSiteWindow : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetWindow( 
        /* [out] */ HWND __RPC_FAR *phwnd) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE EnableModeless( 
        /* [in] */ BOOL fEnable) = 0;
    
};

interface IActiveScript : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE SetScriptSite( 
        /* [in] */ IActiveScriptSite __RPC_FAR *pass) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetScriptSite( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE SetScriptState( 
        /* [in] */ SCRIPTSTATE ss) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetScriptState( 
        /* [out] */ SCRIPTSTATE __RPC_FAR *pssState) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE AddNamedItem( 
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwFlags) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE AddTypeLib( 
        /* [in] */ REFGUID rguidTypeLib,
        /* [in] */ DWORD dwMajor,
        /* [in] */ DWORD dwMinor,
        /* [in] */ DWORD dwFlags) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetScriptDispatch( 
        /* [in] */ LPCOLESTR pstrItemName,
        /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetCurrentScriptThreadID( 
        /* [out] */ SCRIPTTHREADID __RPC_FAR *pstidThread) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetScriptThreadID( 
        /* [in] */ DWORD dwWin32ThreadId,
        /* [out] */ SCRIPTTHREADID __RPC_FAR *pstidThread) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE GetScriptThreadState( 
        /* [in] */ SCRIPTTHREADID stidThread,
        /* [out] */ SCRIPTTHREADSTATE __RPC_FAR *pstsState) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE InterruptScriptThread( 
        /* [in] */ SCRIPTTHREADID stidThread,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo,
        /* [in] */ DWORD dwFlags) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IActiveScript __RPC_FAR *__RPC_FAR *ppscript) = 0;
    
};


interface IActiveScriptParse : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE InitNew( void) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE AddScriptlet( 
        /* [in] */ LPCOLESTR pstrDefaultName,
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ LPCOLESTR pstrSubItemName,
        /* [in] */ LPCOLESTR pstrEventName,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ BSTR __RPC_FAR *pbstrName,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo) = 0;
    
  virtual HRESULT STDMETHODCALLTYPE ParseScriptText( 
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ IUnknown __RPC_FAR *punkContext,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ VARIANT __RPC_FAR *pvarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo) = 0;
    
};

extern "C" HRESULT CLSIDFromProgID (LPCOLESTR lpszProgID,
				    LPCLSID lpclsid);

/*---------------------------------------------------------------------*/
/*                            COM API                                  */
/*---------------------------------------------------------------------*/

extern "C" HRESULT CoCreateInstance(const CLSID & rclsid, 
				    LPUNKNOWN pUnkOuter,
				    DWORD dwClsContext, 
				    const IID & riid, 
				    LPVOID * ppv);


/*---------------------------------------------------------------------*/
/*                         Other used APIs                             */
/*---------------------------------------------------------------------*/

extern "C" DWORD GetModuleFileNameA(HMODULE hModule,
			 LPSTR lpFilename,
			 DWORD nSize);

extern "C" DWORD GetModuleFileNameW(HMODULE hModule,
			 LPWSTR lpFilename,
			 DWORD nSize);

#ifdef UNICODE
#define GetModuleFileName  GetModuleFileNameW
#else
#define GetModuleFileName  GetModuleFileNameA
#endif /* UNICODE */

/*---------------------------------------------------------------------*/
/*                   RPC stuff needed for MultiCAD                     */
/*---------------------------------------------------------------------*/

extern "C" void CoFreeUnusedLibraries(void);

#ifndef __RPC_USER
#define __RPC_USER
#endif

#ifndef __RPC_STUB
#define __RPC_STUB
#endif

#ifndef DECLSPEC_SELECTANY
#if (_MSC_VER >= 1100)
#define DECLSPEC_SELECTANY  __declspec(selectany)
#else
#define DECLSPEC_SELECTANY
#endif
#endif

#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)	__declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C    extern "C"
#else
#define EXTERN_C    extern
#endif
#endif

typedef void __RPC_FAR * RPC_IF_HANDLE;

#ifdef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#endif // INITGUID

#ifndef __IRpcStubBuffer_FWD_DEFINED__
#define __IRpcStubBuffer_FWD_DEFINED__
typedef struct    IRpcStubBuffer     IRpcStubBuffer;
#endif 	/* __IRpcStubBuffer_FWD_DEFINED__ */


#ifndef __IRpcChannelBuffer_FWD_DEFINED__
#define __IRpcChannelBuffer_FWD_DEFINED__
typedef struct    IRpcChannelBuffer  IRpcChannelBuffer;
#endif 	/* __IRpcChannelBuffer_FWD_DEFINED__ */

typedef void * I_RPC_HANDLE;
typedef I_RPC_HANDLE RPC_BINDING_HANDLE;
#define RPC_MGR_EPV void

typedef struct _RPC_VERSION {
    unsigned short MajorVersion;
    unsigned short MinorVersion;
} RPC_VERSION;

typedef struct _RPC_SYNTAX_IDENTIFIER {
    GUID SyntaxGUID;
    RPC_VERSION SyntaxVersion;
} RPC_SYNTAX_IDENTIFIER, __RPC_FAR * PRPC_SYNTAX_IDENTIFIER;

typedef struct _RPC_MESSAGE
{
    RPC_BINDING_HANDLE Handle;
    ULONG DataRepresentation;
    void __RPC_FAR * Buffer;
    unsigned int BufferLength;
    unsigned int ProcNum;
    PRPC_SYNTAX_IDENTIFIER TransferSyntax;
    void __RPC_FAR * RpcInterfaceInformation;
    void __RPC_FAR * ReservedForRuntime;
    RPC_MGR_EPV __RPC_FAR * ManagerEpv;
    void __RPC_FAR * ImportContext;
    ULONG RpcFlags;
} RPC_MESSAGE, __RPC_FAR * PRPC_MESSAGE;


#if (!defined (MAINWIN) && __STDC__) || defined(NONAMELESSUNION)
#define __VARIANT_NAME_1 n1
#define __VARIANT_NAME_3 n3
#else
#define __VARIANT_NAME_1
#define __VARIANT_NAME_3
#endif
#if (!defined (MAINWIN) && __STDC__) || defined(NONAMELESSDECLARATION)
#define __VARIANT_NAME_2 n2
#define __VARIANT_NAME_4 brecVal
#else
#define __VARIANT_NAME_2
#define __VARIANT_NAME_4
#endif

#define V_UNION(X, Y)    ((X)->Y)
#define V_ARRAY(X)       V_UNION(X, parray)

/*---------------------------------------------------------------------*/
/*                   System time type                                  */
/*---------------------------------------------------------------------*/

#ifndef _SYSTEMTIME_
#define _SYSTEMTIME_
typedef struct  _SYSTEMTIME
    {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
    }	SYSTEMTIME;

typedef struct _SYSTEMTIME __RPC_FAR *PSYSTEMTIME;

typedef struct _SYSTEMTIME __RPC_FAR *LPSYSTEMTIME;

#endif // !_SYSTEMTIME

/*---------------------------------------------------------------------*/
/*                       Error API                                     */
/*---------------------------------------------------------------------*/


interface ICreateErrorInfo : public IUnknown
{
 public:
  virtual HRESULT STDMETHODCALLTYPE SetGUID( 
	 /* [in] */ REFGUID rguid) = 0;
  
  virtual HRESULT STDMETHODCALLTYPE SetSource( 
	 /* [in] */ wchar_t * szSource) = 0;
  
  virtual HRESULT STDMETHODCALLTYPE SetDescription( 
	 /* [in] */ wchar_t * szDescription) = 0;
  
  virtual HRESULT STDMETHODCALLTYPE SetHelpFile( 
	/* [in] */ wchar_t * szHelpFile) = 0;
  
  virtual HRESULT STDMETHODCALLTYPE SetHelpContext( 
	/* [in] */ DWORD dwHelpContext) = 0;
        
};

#include "IErrorInfo.h"    

#endif // _WINDOWS_SOURCE

#endif // _CATMainwin_h






