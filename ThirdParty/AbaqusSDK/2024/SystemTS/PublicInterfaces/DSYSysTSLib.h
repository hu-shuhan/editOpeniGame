#ifndef DSYSYSTSLIB_H
#define DSYSYSTSLIB_H
/*
// COPYRIGHT DASSAULT SYSTEMES 2011
*/
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATDataType.h"
#include "CATSysTS.h"

#if !defined(unix) && (defined(_AIX) || defined(_AIX_SOURCE) || defined(_HPUX_SOURCE) || defined(_IRIX_SOURCE) || defined(_SUNOS_SOURCE) || defined(_LINUX_SOURCE) || defined (_MACOSX_SOURCE) || defined(_DARWIN_SOURCE) ||defined (_IOS_SOURCE) || defined(_ANDROID_SOURCE)) 
    #define unix
#endif

#ifdef _WINDOWS_SOURCE
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdlib.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#endif



/**
 * Return values.
 * @param CATLibSuccess
 *	The called function has succeeded.
 * @param CATLibError
 *	The called function has failed
*/
typedef enum {
    CATLibSuccess = 0,  // S_OK
	CATLibError = -1    // FAILED
} CATLibStatus;


#ifdef _IOS_SOURCE
#define stat64		stat
#define fstat64		fstat
#endif

    

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Retrieves the logon.
 * <br><b>Role</b>: Retrieves the login name and its size.
 * @param oName
 *	the login name.
 *	<br><b>Lifecycle rules deviation</b>: The caller must allocate the 
 *	buffer oName.
 * @param iNamesize
 *	the maximum size of the login name that would be retrieved.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The login name was successfully retrieved.</dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured.</dd>
 * </dl>
*/
ExportedByCATSysTS CATLibStatus CATGetLoginName (char *oName,
						 unsigned int iNamesize);



#ifdef __cplusplus
};
#endif

#endif
