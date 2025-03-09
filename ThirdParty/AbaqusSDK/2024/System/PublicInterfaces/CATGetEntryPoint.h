#ifndef __CATGetEntryPoint
#define __CATGetEntryPoint

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#ifdef _WINDOWS_SOURCE
#include <wtypes.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#endif
#if defined(_IRIX_SOURCE) || defined(_SUNOS_SOURCE) || defined(_AIX) || defined(_LINUX_SOURCE) || defined(_DARWIN_SOURCE) || defined (_MACOSX_SOURCE) || defined (_ANDROID_SOURCE)
#include <dlfcn.h>
#endif
#ifdef _HPUX_SOURCE
#include <dl.h>
#endif

#include "CATSysDataType.h"
#include "JS0CORBA.h"

/**
 * Structure returned by global fonctions.
 * @see CATGetEntryPoint, CATGetFunctionAddress
 */
struct LibraryHandler
{
/**
 * Pointer to the requested function.
 */
   void *EntryPoint;
#if defined(_IRIX_SOURCE) || defined(_SUNOS_SOURCE) || defined(_AIX) || defined(_LINUX_SOURCE) || defined(_DARWIN_SOURCE) || defined (_MACOSX_SOURCE) || defined (_ANDROID_SOURCE)
/**
 * @nodoc
 */
   void *Handler;
#endif
#ifdef _HPUX_SOURCE
/**
 * @nodoc
 */
   shl_t Handler;
#endif
#if defined (_WINDOWS_SOURCE)
/**
 * @nodoc
 */
   HINSTANCE Handler;
#endif
};


/**
 * Loads a library and retrieves the address of a fonction.
 * @param iLibraryName The name of the library to load. This name is operating system dependent.
 * @param iEntryName The name of the requested function. Optional.
 * @param iLibPath The path of the library. Optional.
 * @param iErreurMsg controls display of messages, optional.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>No display.</dd>
 *     <dt>1 </dt>
 *     <dd>Messages are displayed. This is the default.</dd>
 *   </dl>
 * @paran iHpSearch On HP-UX, consider the <tt>SHLIB_PATH</tt> environment variable.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>Do not consider the <tt>SHLIB_PATH</tt> environment variable.</dd>
 *     <dt>1 </dt>
 *     <dd>Consider the <tt>SHLIB_PATH</tt> environment variable. This is the default.</dd>
 *   </dl>
 * @param iIgnoreLastError Optional.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>Do not try to load the library if a previous load failed for this library name. This is the default.</dd>
 *     <dt>1 </dt>
 *     <dd>Try to load the library even if a previous load failed for this library name.</dd>
 *   </dl>
 * @return a @href LibraryHandler
 */ 
ExportedByJS0CORBA extern LibraryHandler CATGetEntryPoint (
			const char *iLibraryName,
			const char *iEntryName = NULL,
			const char *iLibPath = NULL,
			int iErreurMsg = 1,
			int iHpSearch = 1,
   int iIgnoreLastError = 0);


/**
 * Loads a library and retrieves the address of a fonction.
 * @param iLibraryName The name of the library to load. This name is not operating system dependent, prefix and suffix are automaticaly added.
 * @param iEntryName The name of the requested function. Optional.
 * @param iLibPath The path of the library. Optional.
 * @param iErreurMsg controls display of messages, optional.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>No display. . This is the default.</dd>
 *     <dt>1 </dt>
 *     <dd>Messages are displayed</dd>
 *   </dl>
 * @paran iHpSearch On HP-UX, consider the <tt>SHLIB_PATH</tt> environment variable.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>Do not consider the <tt>SHLIB_PATH</tt> environment variable.</dd>
 *     <dt>1 </dt>
 *     <dd>Consider the <tt>SHLIB_PATH</tt> environment variable. This is the default.</dd>
 *   </dl>
 * @param iIgnoreLastError Optional.
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>Do not try to load the library if a previous load failed for this library name. This is the default.</dd>
 *     <dt>1 </dt>
 *     <dd>Try to load the library even if a previous load failed for this library name.</dd>
 *   </dl>
 * @return a @href LibraryHandler
 */ 
ExportedByJS0CORBA extern LibraryHandler CATGetFunctionAddress (
			const char *iLibraryName,
			const char *iEntryName = NULL,
			const char *iLibPath = NULL,
			int iErreurMsg = -1,
			int iHpSearch = 1,
   int iIgnoreLastError = 0);

/**
 * Closes a library.
 * <b>Role</b>: Closes a library loaded with @href CATGetEntryPoint or @href CATGetFunctionAddress.
 * @param iHandler Handler returned by @href CATGetEntryPoint or @href CATGetFunctionAddress.
 * @return 
 *   <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *     <dd>Library has been closed.</dd>
 *     <dt>1 </dt>
 *     <dd>A system error occured.</dd>
 *     <dt>-1 </dt>
 *     <dd>Library has not been closed because one handler remains on it.</dd>
 *   </dl>
 */
ExportedByJS0CORBA extern int CATCloseLibrary (LibraryHandler iHandler);

#endif
