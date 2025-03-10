/* COPYRIGHT DASSAULT SYSTEMES 2000 */
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
#ifndef CATTypesForFileExit_H 
#define CATTypesForFileExit_H
#ifndef _WINDOWS_SOURCE
#include <errno.h>
#endif


 /**
  * Definition of the acces rigth on a file.
  * <b>Role</b>: This type is used to defined the requested and or granted rights 
  * on a file. It is construted by ORING the following values.
  * @param   CATReadRight    Read right for the user.
  * @param   CATWriteRight Write right for the user.
  * @param   CATRWRight Read/write right for the user.
  * @param   CATExecuteRight Execute right for the user.
  *  @see CATIUExitIO
  */
typedef unsigned int CATAccessRight;

/** @nodoc */
#define CATReadRight		0x0000000
/** @nodoc */
#define CATWriteRight		0x0000001
/** @nodoc */
#define CATRWRight		0x0000002
/** @nodoc */
#define CATExecuteRight		0x0000040




 /**
  * Definition of the error type for the IO user Exit.
  * <b>Role</b>: If any error occurs in the code of an IO User Exit, the method
  * must return CATERRNO, in order that the CATIA soft can interpret correctly
  * the error and show the correct error messages.
  *  @param  0  On success
  *  @param  CATERRNO if any system error has occurred.
  *  @see CATIUExitIO
  */
typedef int CATFileSystemError;

#ifdef _WINDOWS_SOURCE
/** @nodoc */
#define CATERRNO GetLastError()
#else
/** @nodoc */
#define CATERRNO errno
#endif

#endif
