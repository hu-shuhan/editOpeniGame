#ifndef CATIAV5Precompiled_H
#define CATIAV5Precompiled_H

#ifdef _WINDOWS_SOURCE

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/*
// COPYRIGHT DASSAULT SYSTEMES 2000

//=========================================================================
// HEADER PERMITTING TO 
//=========================================================================
*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#if (defined _MFC_VER) && (_MFC_VER < 0x0800 || (_MFC_VER >= 0x0900 && defined _MK_PRECOMPIL_WITH_AFXWIN))

#ifndef _WINDOWS_
#pragma warning(push)
#pragma warning(disable:4530) 
#include <afxwin.h>
#pragma warning(pop)
#endif // _WINDOWS_
#include <objbase.h>

#else

#pragma warning(push)
#pragma warning(disable:4530) 
#ifndef _WINDOWS_
#include <wtypes.h>
#include <wchar.h>
#include <tchar.h>
#include <shellapi.h>
#endif // _WINDOWS_
#include <objbase.h>
#pragma warning(pop)

#endif

#endif // _WINDOWS_SOURCE

#endif
