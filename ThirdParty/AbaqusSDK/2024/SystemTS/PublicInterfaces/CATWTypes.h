//*===========================================================================*
//* COPYRIGHT DASSAULT SYSTEMES 1996                                          *
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
//*===========================================================================*
//*                                                                           *
//*  CATFile                                                                  *
//*                                                                           *
//*  Usage Notes:                                                             *
//*===========================================================================*
//*  Creation mars 1997                                                       *
//*===========================================================================*
#ifndef __DEFTYPES
#define __DEFTYPES

#include "IUnknown.h"
#include "CATSTGMode.h"
#include "BigLittle.h"
#include "CATDataType.h"


#ifdef _WINDOWS_SOURCE

#define STATSTGDEL(a)	CoTaskMemFree (a)


#define HERITE :public CATBaseUnknown
#define FNULL  {0,0}
#define CATGUID_NULL  { 0,0,0,{0,0,0,0,0,0,0,0}}
#define  SNULL  {0, 0, {0,0}, FNULL,FNULL,FNULL, 0, 0, CATGUID_NULL, 0, 0}

#else


#define STATSTGDEL(a)	delete []a

#define FAR 
#define HERITE : public CATBaseUnknown

typedef  unsigned short WCHAR;	// unicode char
typedef  WCHAR OLECHAR;


typedef  WCHAR** SNB;
typedef unsigned long long ULONGLONG;
typedef long long LONGLONG;


// for little endian platforms
#ifdef _ENDIAN_LITTLE
//--------------------------------
typedef union _ULARGE_INTEGER
{
  struct 
  {
    CATULONG32 LowPart;
    CATULONG32 HighPart;
  }u;
  ULONGLONG  QuadPart;
}   ULARGE_INTEGER;

//-------------------------------
typedef union _LARGE_INTEGER
{
  struct 
  {
    CATLONG32 LowPart;
    CATLONG32 HighPart;
  }u;
  LONGLONG  QuadPart;
}   LARGE_INTEGER;

#else
// unix platform
//--------------------------------
typedef union _ULARGE_INTEGER
{
  struct 
  {
    CATULONG32 HighPart;
    CATULONG32 LowPart;
  }u;
  ULONGLONG  QuadPart;
}   ULARGE_INTEGER;

//-------------------------------
typedef union _LARGE_INTEGER
{
  struct 
  {
    CATLONG32 HighPart;
    CATLONG32 LowPart;
  }u;
  LONGLONG  QuadPart;
}   LARGE_INTEGER;
#endif // end of platforms dependent definitions

//------------------------------
typedef struct _tagFILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME; 




#define FNULL  { 0xd53e8000, 0x19db1de }
//#define GUID_NULL  { 0,0,0,{0,0,0,0,0,0,0,0}}
#define CATGUID_NULL  { 0,0,0,{0,0,0,0,0,0,0,0}}

//typedef GUID* REFCLSID;
#define REFCLSID            const CLSID &

//-----------------------------
typedef struct  tagSTATSTG
{
  WCHAR* 	pwcsName;
  DWORD 	type;
  ULARGE_INTEGER cbSize;
  FILETIME 	mtime;
  FILETIME 	ctime;
  FILETIME 	atime;
  DWORD 	grfMode;
  DWORD 	grfLocksSupported;
  CLSID 	clsid;
  DWORD 	grfStateBits;
  DWORD 	reserved;
}	STATSTG;

#define  SNULL  {0, 0, {0,0}, FNULL,FNULL,FNULL, 0, 0, CATGUID_NULL, 0, 0}



//-----------------------------
typedef enum tagSTGTY
{	
  STGTY_ROOT	= 0,
  STGTY_STORAGE	= 1,
  STGTY_STREAM	= 2,
  STGTY_LOCKBYTES	= 3,
  STGTY_PROPERTY	= 4
}	STGTY;

//-----------------------------
typedef enum tagSTATFLAG
{
  STATFLAG_NONAME =1
} STATFLAG;


 
typedef enum tagSTREAM_SEEK
{	
  STREAM_SEEK_SET	= 0,
  STREAM_SEEK_CUR	= 1,
  STREAM_SEEK_END	= 2
}	STREAM_SEEK;


typedef enum tagLOCKTYPE
{	
  LOCK_WRITES	= 1,
  LOCK_EXCLUSIVE	= 2,
  LOCK_ONLYONCE	= 4
}	LOCKTYPE;



#endif // Windows_source

#endif
