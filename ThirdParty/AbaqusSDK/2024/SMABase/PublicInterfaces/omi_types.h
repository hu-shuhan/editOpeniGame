//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef omi_TYPES_h
#define omi_TYPES_h

#if defined(_WINDOWS_SOURCE) || defined (_WIN64)
#include <stddef.h> /* intptr_t */
#include <BaseTsd.h> /* SSIZE_T */
#else
#include <inttypes.h> /* intptr_t */
#endif

/* Begin local includes */

/* Convenience typedef */
#if defined(_WINDOWS_SOURCE) || defined (_WIN64)
typedef SSIZE_T ssize_t;
#endif

/* The int64 and uint64 data types should be used only for special
   purposes, such as addressing large files. Discuss any proposed use
   of this type with Product Architecture. */

/* uint64: */
#if defined _WINDOWS_SOURCE
typedef unsigned __int64 uint64;
#elif defined _LINUX_SOURCE
typedef unsigned long long uint64;
#endif

/* int64: */
#if defined _WINDOWS_SOURCE
typedef __int64 int64;
#elif defined _LINUX_SOURCE
typedef long long int64;
#endif

/* Convenience typedef to save typing
   Only use our typedef if it has not already been set by the system */
#if (defined(_LINUX_SOURCE) && defined(__USE_MISC) && defined(_SYS_TYPES_H)) 
#else
/* uint */
typedef unsigned int uint;
#endif

/* Convenience typedef to save typing
   Only use our typedef if it has not already been set by the system */
#if (defined(_LINUX_SOURCE) && defined(__USE_MISC) && defined(_SYS_TYPES_H))
#else
/* ushort */
typedef unsigned short int ushort;
#endif

/* Convenience typedef to explicitely indicate that char is used as int */
typedef signed char schar;
typedef unsigned char uchar;

/* Special typedef to allow us to deal with MS special types where we need a 
   32 bit type that is disguised as a long. This type should not show up in 
   many places. If you want to use it get the blessing from architecture. */
typedef int omiInt32_t;
typedef unsigned int omiUint32_t;

/* Special typedef to deal with long in External interfaces that we do not 
   control or in interfaces we do not want to change. This type should not 
   show up in many places. If you want to use it get the blessing from 
   architecture. 
   Note that these types are not portable:
   4 byte types on 32 bit systems;
   8 byte types on 64 bit UNIX and Linux;
   4 byte types on 64 bit windows; 
   Never ever use these for a pointer in non platform specific code. */
#if defined _WINDOWS_SOURCE
typedef int omi_interfLong_t;
typedef unsigned int omi_interfULong_t;
#elif defined _LINUX_SOURCE
typedef  long long omi_interfLong_t;
typedef unsigned long long omi_interfULong_t;
#endif

#endif
