
#ifndef CATCreateInstance_h
#define CATCreateInstance_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0CORBA.h"
#include "IUnknown.h"

#ifndef _WINDOWS_SOURCE
/** @nodoc */
#define COSERVERINFO void

/** @nodoc */
typedef enum tagCLSCTX {
  CLSCTX_INPROC_SERVER	   = 0x1,
  CLSCTX_INPROC_HANDLER	   = 0x2,
  CLSCTX_LOCAL_SERVER	   = 0x4,
  CLSCTX_INPROC_SERVER16   = 0x8,
  CLSCTX_REMOTE_SERVER	   = 0x10,
  CLSCTX_INPROC_HANDLER16  = 0x20,
  CLSCTX_INPROC_SERVERX86  = 0x40,
  CLSCTX_INPROC_HANDLERX86 = 0x80,
  CLSCTX_ESERVER_HANDLER   = 0x100,
  CLSCTX_RESERVED	   = 0x200,
  CLSCTX_NO_CODE_DOWNLOAD  = 0x400
}	CLSCTX;

#endif // _WINDOWS_SOURCE

/**
 * Creates an instance of the class associated with a specified CLSID.
 * @param iClsid
 *   The class identifier (CLSID) of the object
 * @param pUnkOuter
 *   Must be NULL
 * @param iContext
 *   Must be equal to 0
 * @param riid
 *   The reference to the identifier of the requested interface
 * @param ppv
 *   The address of output variable that receives the interface pointer requested in <tt>riid</tt>
 * @return
 *   <tt>S_OK</tt> if the creation succeeded, <tt>E_FAIL</tt> otherwise.
 */
HRESULT ExportedByJS0CORBA CATCreateInstance(const CLSID &iClsid, IUnknown* pUnkOuter, DWORD iContext, const IID &riid, void** ppv);

/** @nodoc */
HRESULT ExportedByJS0CORBA CATGetClassObject(const CLSID &rclsid, DWORD iContext, COSERVERINFO * pServerInfo, const IID riid, void** ppv);

/**
 * Declares a component CLSID.
 */
#define CATDeclareComponent(component) \
extern "C" const CLSID CLSID_##component


#endif // CATCreateInstance_h

