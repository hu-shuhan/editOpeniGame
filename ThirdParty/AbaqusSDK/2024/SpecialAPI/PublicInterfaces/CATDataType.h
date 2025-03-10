#ifndef __CATDataType
#define __CATDataType

/* COPYRIGHT DASSAULT SYSTEMES 1999                       */

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#if defined(_HPUX_SOURCE)
#ifndef  _LARGEFILE64_SOURCE
#define  _LARGEFILE64_SOURCE
#endif
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 
#ifdef _WINDOWS_SOURCE
#include <io.h>
#include <stdlib.h>
#else
#include <dirent.h>
#include <limits.h>
#endif


#if defined(_DARWIN_SOURCE) || defined(_MACOSX_SOURCE)
#include <stdint.h>
#endif
#if defined( _IRIX_SOURCE ) || defined( _AIX ) 
 #include <inttypes.h>
#elif _SUNOS_SOURCE 
 #include <sys/int_types.h>
#elif _HPUX_SOURCE  
 #include <inttypes.h>
#elif _WINDOWS_SOURCE
/**
 * @nodoc
 */
	typedef int LONG32, INT32 ;
/**
 * @nodoc
 */
	typedef unsigned int ULONG32, UINT32 ;
#elif _LINUX_SOURCE
 #include <stdint.h>
#endif


/**
  *========================================================
  * Fixed Length Data types
  *
  * 			CATINT32  - CATUINT32
  * 			CATLONG32 - CATULONG32
  * 			CATLONG64 - CATULONG64
  *
  * The Length of these Data Types remains the same, 
  * regardless of the OS and the machine.
  *========================================================
  */

    /**
        * 32-bit signed integer (32 and 64-Unix, Windows).
	*/
  typedef int CATINT32 ;

    /**
        * 32-bit unsigned integer (32 and 64-Unix, Windows).
	*/
  typedef unsigned int CATUINT32 ;


#if !defined(PLATEFORME_DS64)

    /**
        * 32-bit signed long integer.
        * <br>Typedef valid on Windows and 32-bit Unix
	*/
  typedef long CATLONG32 ;

    /**
        * 32-bit unsigned long integer.
        * <br>Typedef valid on Windows and 32-bit Unix
	*/
  typedef unsigned long CATULONG32 ;

#else

    /**
        * 32-bit signed integer.
        * <br>Typedef valid on 64-bit Unix
	*/
    typedef int CATLONG32 ;

    /**
        * 32-bit unsigned integer.
        * <br>Typedef valid on 64-bit Unix
	*/
    typedef unsigned int CATULONG32 ;

#endif


#if defined (_WINDOWS_SOURCE)

/**
* 64-bit signed long.
* <br>Typedef valid on Windows platforms
*/
typedef __int64 CATLONG64 ; 

/**
* 64-bit unsigned long.
* <br>Typedef valid on Windows platforms
*/
typedef unsigned __int64 CATULONG64 ; 

#else

/**
* 64-bit signed long.
* <br>Typedef valid on Unix platforms
*/
typedef long long CATLONG64 ; 

/**
* 64-bit unsigned long.
* <br>Typedef valid on Unix platforms
*/
typedef unsigned long long CATULONG64 ; 

#endif

 
/**
  *========================================================
  * Non Fixed Length Data types
  *
  * CATLONGINT - CATULONGINT
  * Integer type whose length is the same as the length
  * of a long, whatever the platform
  *
  * CATLONGPTR - CATULONGPTR - CATINTPTR - CATUINTPTR
  * Used to store data that need to span the range of a
  * pointer
  *========================================================
  */

  /**
      * Signed long type having the size of a long, i.e.
      * <br> 64 bits on 64-bit Unix and 32 bits on any
      * <br> other 32- and 64-bit Unix/Windows.
      */

  typedef long CATLONGINT ;

  /**
      * Unsigned long type having the size of a long, i.e.
      * <br> 64 bits on 64-bit Unix and 32 bits on any
      * <br> other 32- and 64-bit Unix/Windows.
      */

  typedef unsigned long CATULONGINT ;


  #if defined(_MSC_VER) && _MSC_VER >=1300
    /**
        * @nodoc
        * Flag used for warning generation under VC7
        */
      #define __W64 __w64
  #else
    /**
        * @nodoc
        */
      #define __W64
  #endif

  #if defined (_WINDOWS_SOURCE) && defined (PLATEFORME_DS64)

    /**
	* Signed long type having pointer precision.
        * <br>Typedef valid on 64-bit Windows
	*/
      typedef __int64 CATLONGPTR;

    /**
	* Unsigned long type having pointer precision.
        * <br>Typedef valid on 64-bit Windows
	*/
      typedef unsigned __int64 CATULONGPTR;

    /**
	* signed int type having pointer precision.
        * <br>Typedef valid on 64-bit Windows
	*/
      typedef __int64 CATINTPTR;  

    /**
	* unsigned int type having pointer precision.
        * <br>Typedef valid on 64-bit Windows
	*/
      typedef unsigned __int64 CATUINTPTR; 

  #elif defined (PLATEFORME_DS64)

    /**
        * signed long type having pointer precision.
        * <br>Typedef valid on 64-bit Unix
	*/
      typedef intptr_t CATLONGPTR;

    /**
        * unsigned long type having pointer precision.
        * <br>Typedef valid on 64-bit Unix
	*/
      typedef uintptr_t CATULONGPTR;

    /**
        * signed int type having pointer precision.
        * <br>Typedef valid on 64-bit Unix
	*/
      typedef intptr_t CATINTPTR;  

    /**
        * unsigned int type having pointer precision.
        * <br>Typedef valid on 64-bit Unix
	*/
      typedef uintptr_t CATUINTPTR; 

  #else

    /**
	* signed long type having pointer precision.
        * <br>Typedef valid on 32-bit platforms
	*/
      typedef __W64 long CATLONGPTR ;

    /**
	* unsigned long type having pointer precision.
        * <br>Typedef valid on 32-bit platforms
	*/
      typedef __W64 unsigned long CATULONGPTR ;

    /**
	* signed int type having pointer precision.
        * <br>Typedef valid on 32-bit platforms
	*/
      typedef __W64 int CATINTPTR ;

    /**
	* unsigned int type having pointer precision.
        * <br>Typedef valid on 32-bit platforms
	*/
      typedef __W64 unsigned int CATUINTPTR ;

  #endif

  #if defined (_WINDOWS_SOURCE) && defined(_MSC_VER) && (_MSC_VER < 1300)
    /**
        * @nodoc
        * DWORD_PTR : Microsoft type not provided with older VC++ 
        * versions
        */
      typedef unsigned long DWORD_PTR, *PDWORD_PTR; 
  #endif
 
/** 
 *========================================================
 * Data Manipulation Macros
 *
 *    1. Pointers to Int/Long Data Types
 *           CATPtrToLONG32(p) - CATPtrToULONG32(p)
 *           CATPtrToINT32(p) - CATPtrToUINT32(p)
 *    2. Data 
 *======================================================== 
 * Usage   : If you absolutely must truncate a pointer to a 
 *           32 bit value.
 * Caution : Use these macro with extreme caution. Once a 
 *           pointer has passed through these macros, you 
 *           should never use it a pointer again. 
 */

    /**
        * Use this macro to convert a pointer to a 32-bit 
        * signed long integer for arithmetic manipulation.
	*/
    #define CATPtrToLONG32(p) ((CATLONG32)(CATLONGPTR)(p))

    /**
        * Use this macro to convert a pointer to a 32-bit 
        * unsigned long integer for arithmetic manipulation.
	*/
    #define CATPtrToULONG32(p) ((CATULONG32)(CATLONGPTR)(p))

    /**
        * Use this macro to convert a pointer to a 32-bit 
        * unsigned integer for arithmetic manipulation.
	*/
    #define CATPtrToINT32(p) ((CATINT32)(CATLONGPTR)(p))
 
   /**
        * Use this macro to convert a pointer to a 32-bit 
        * unsigned integer for arithmetic manipulation.
	*/
    #define CATPtrToUINT32(p) ((CATUINT32)(CATULONGPTR)(p))
 
   /**
        * Use this macro to cast a 32-bits signed long type 
        * to a pointer.
	*/
    #define CATLONG32ToPtr(p) ((void*)(CATLONGPTR)(p))
 
   /**
        * Use this macro to cast a 32-bits unsigned long 
        * type to a pointer.
	*/
    #define CATULONG32ToPtr(p) ((void*)(CATULONGPTR)(p))
 
   /**
        * Use this macro to cast a 32-bits integer type
        * to a pointer.
	*/
    #define CATINT32ToPtr(p) ((void*)(CATLONGPTR)(p))
 
   /**
        * Use this macro to cast a 32-bits unsigned integer 
        * type to a pointer.
	*/
    #define CATUINT32ToPtr(p) ((void*)(CATULONGPTR)(p))
 
 
/**
 *========================================================
 * printf() format strings
 *
 *   FMTLONGI64 : Flag for platform independent way of 
 *     formatting strings in the printf family of 
 *     statements
 *
 *   Usage : CATINTPTR a64bitint;
 *	     printf("The value is " FMTLONGI64 "d\n", 
 *                  a64bitint);
 *========================================================
*/	

/**
  * @nodoc
*/	
#if defined(_WINDOWS_SOURCE)
	#define FMTLONGI64 "%I64"
#else
	#define FMTLONGI64 "%ll"
#endif




/**
  * @nodoc
*/
typedef struct stat		CATFileInfo32;
/**
  * @nodoc
*/
typedef struct stat		 CATFileInfo;

/**
  * @nodoc
*/
#ifdef _WINDOWS_SOURCE
typedef struct _stati64		CATFileInfo64;
#else
#if defined(_IOS_SOURCE) || defined(_MACOSX_SOURCE)
/* strange stat64 not defined but obviously stat is a stat64 */
typedef struct stat		CATFileInfo64; 
#else
typedef struct stat64		CATFileInfo64;
#endif
#endif




#endif
