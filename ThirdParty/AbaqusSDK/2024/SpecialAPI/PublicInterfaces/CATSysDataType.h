#ifndef CATSysDataType_H
#define CATSysDataType_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/** 
 * Definition of Internationalization Framework types.
 * <b>Role</b>:
 * Defines Internationalization Framework types.
 * <p>
 * You will find below a reference to the Unicode 
 * standard, and to UTF8. Unicode is a standard designed and
 * promoted by the unicode consortium, it encodes characters
 * on two bytes. UTF-8 is a transformation format used as a file
 * code set, in particular for persistent data. The ISO10646 
 * standard is built on the top of unicode, it includes several 
 * code sets: Unicode, known here as UCS-2 (for Universal 
 * Multiple-Octet Coded Character Set 2-byte form), UTF-8 and
 * so on ...
 * <p>
 * <b>Note</b>: You will find also below some references to the 
 * STEP Standard. It is descibed in ISO 10133 .
 */

/**
 * CATUC2Bytes type.
 * <b>Role<b>: 16 bits width UCS-2/UTF-16 character type. Note that the C++ char16_t type may be defined larger.
 */
typedef unsigned short int CATUC2Bytes;

/**
 * @nodoc
 * CATUC4Bytes type.
 * 32 bits width UTF-32 character type. Note that the C++ char32_t type may be defined larger.
 */
typedef unsigned int CATUC4Bytes;

#define DS_CXX11_SUPPORT_UTF_CHAR_TYPES
#if __cplusplus > 199711L
#elif defined(_WINDOWS_SOURCE) && (_MSC_VER >= 1900)
#else
#undef DS_CXX11_SUPPORT_UTF_CHAR_TYPES
/** nodoc */
typedef CATUC4Bytes char32_t;
/** nodoc */
typedef CATUC2Bytes char16_t;
#endif

#include "CATDataType.h"

#ifndef NULL
#define NULL 0
#endif


#ifndef _WINDOWS_SOURCE

/**
 * A type for unsigned long to be Microsoft(R) compatible.
 */
#ifdef PLATEFORME_DS64
typedef unsigned int ULONG;
#else
typedef unsigned long ULONG;
#endif

/**
 * A type for long to be Microsoft(R) compatible.
 */
#ifdef PLATEFORME_DS64
typedef int LONG;
#else
typedef long LONG;
#endif

/**
 * A type for char to be Microsoft(R) compatible.
 */
typedef unsigned char BYTE;
/**
 * A type for unsigned long to be Microsoft(R) compatible.
 */
#ifdef PLATEFORME_DS64
typedef unsigned int DWORD;
#else
typedef unsigned long DWORD;
#endif
/**
 * A type for short to be Microsoft(R) compatible.
 */
typedef unsigned short WORD;

#else

#pragma once

#if defined(_MFC_VER) && _MFC_VER>=0x0800

#pragma warning(push)
#pragma warning(disable:4530) 
#ifndef _WINDOWS_
#include <wtypes.h>
#include <wchar.h>
#include <tchar.h>
#include <shellapi.h>
#endif
#pragma warning(pop)

#else

#ifndef _WINDOWS_
#include <afxwin.h>
#endif

#endif //defined(_MFC_VER) && _MFC_VER>=0x0800

#endif //_WINDOWS_SOURCE

/**
 * A type for long in Automation interfaces to be Microsoft(R) compatible.
 * For non Automation interfaces, use CATLONG32.
 */
typedef LONG CATLONG;

#endif //CATSysDataType_H

