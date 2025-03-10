//===========================================================================
// COPYRIGHT DASSAULT SYSTEMES 1999                                          
//===========================================================================
//                                                                           
//  Standard Files                                                     
//                                                                           
//  Usage Notes: As stdio
//===========================================================================
//  Creation octobre 1999                                 adt              
//===========================================================================
/* COPYRIGHT DASSAULT SYSTEMES 1999 */
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
#ifndef __CATSTDIO
#define __CATSTDIO
#include "JS0LIB.h"
#include "CATDataType.h"
#include "CATErrorDef.h"
#include <time.h>
#if defined ( _LINUX_SOURCE )|| defined (_MACOSX_SOURCE) || defined(_DARWIN_SOURCE)
#include <unistd.h> // off_t is define there on linux, added by spx 6 of October 2006
#endif
class CATUnicodeString;

/**
 * Opens a file.
 * <br><b>Role</b>:Open a file and returns a descriptor.
 * @param iFileName
 *      The path of the file to open.
 * @param iMode 
 *      Opening mode as in fopen
 * @param oFileDesc
 *      the returned file descriptor that will be used in the other functions
 *      for manipulating the opened file.
 * @return
 *   <b>Legal values</b>:
 *   <br><tt> S_OK :</tt>on Success.
 *   <br><tt> negative :</tt> on failure
*/
ExportedByJS0LIB HRESULT CATFOpen(const CATUnicodeString & iFileName, const char *iMode, unsigned int* oFileDesc);

/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFOpen(CATUnicodeString *iFileName, const char *iMode, unsigned int* oFileDesc)
{
    return (iFileName) ? CATFOpen(*iFileName, iMode, oFileDesc) : E_INVALIDARG;
}

/**
 * Copies a file.
 * <br><b>Role</b>:Copies a given file into a new one.
 * @param iFileName
 *      The path of the file to copy.
 * @param iCopyName
 *      the path of the copy file
 * @param iCopyTime
 *      if set the time of the original file times will be preserved.
 * @return
 *   <b>Legal values</b>:
 *   <br><tt> S_OK :</tt>on Success.
 *   <br><tt> negative :</tt> on failure
*/
ExportedByJS0LIB HRESULT CATFCopy(const CATUnicodeString & iFileName, const CATUnicodeString & iCopyName, int iCopyTime);

/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFCopy(CATUnicodeString *iFileName, CATUnicodeString *iCopyName, int iCopyTime)
{
    return (iFileName && iCopyName) ? CATFCopy(*iFileName, *iCopyName, iCopyTime) : E_INVALIDARG;
}

/**
 * Changes the times of a file. 
 * <br><b>Role</b>: Changes the last access and last modification times of
 * a file named by iName
 * @param iName 
 *      the path of the file. 
 * @param iNewAccessTime
 *      the new last access time. 
 * @param iNewModTime
 *      the new last modification time
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/ 
ExportedByJS0LIB HRESULT CATFSetTimes (const CATUnicodeString & iName, time_t iNewAccessTime, time_t iNewModTime);

/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFSetTimes (CATUnicodeString  *iName, time_t iNewAccessTime, time_t iNewModTime)
{
    return (iName) ? CATFSetTimes(*iName, iNewAccessTime, iNewModTime) : E_INVALIDARG;
}

/**
 * Changes the status of a file.
 * <br><b>Role</b>: Applies the rights a the file named by iReferenceFile to 
 * the file named by iFileToUpdate.
 * @param iReferenceFile
 *      the path of the reference file.
 * @param iFileToUpdate
 *      the path of the file which rights have to be modified.
 * @return
 *   <b>Legal values</b>:
 *   <br><tt> S_OK :</tt>on Success.
 *   <br><tt> negative :</tt> on failure
*/
ExportedByJS0LIB HRESULT CATFChmod (const CATUnicodeString  & iReferenceFile, const CATUnicodeString & iFileToUpdate);

/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFChmod (CATUnicodeString *iReferenceFile, CATUnicodeString  *iFileToUpdate)
{
    return (iReferenceFile && iFileToUpdate) ? CATFChmod(*iReferenceFile, *iFileToUpdate) : E_INVALIDARG;
}

#ifdef __cplusplus
extern "C" {
#endif
  
/**
 * Opens a file. 
 * <br><b>Role</b>:Open a file and returns a descriptor.
 * @param iFileName 
 *      The path of the file to open. 
 * @param iMode  
 *      Opening mode as in fopen 
 * @param oFileDesc 
 *      the returned file descriptor that will be used in the other functions
 *      for manipulating the opened file. 
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/ 
  ExportedByJS0LIB HRESULT CATFOpen( const wchar_t *iFileName, 
				     const char *iMode,
				     unsigned int* oFileDesc);

/**
 * Closes a file.
 * <br><b>Role</b>:Writes any buffered data and closes a file previously 
 * opened with CATFOpen.
 * @param iFileDesc
 *      the file descriptor that has been obtained through @href #CATFOpen
 * @return
 *   <b>Legal values</b>:
 *   <br><tt> S_OK :</tt>on Success.
 *   <br><tt> negative :</tt> on failure
*/
  ExportedByJS0LIB HRESULT CATFClose(unsigned int iFileDesc);

/**
 * Flushes a file.
 * <br><b>Role</b>:Writes any buffered data for the previously opened with 
 * CATFOpen file and leaves it open
 * @param iFileDesc
 *      the file descriptor that has been obtained through @href #CATFOpen
 * @return
 *   <b>Legal values</b>:
 *   <br><tt> S_OK :</tt>on Success.
 *   <br><tt> negative :</tt> on failure
*/
  ExportedByJS0LIB HRESULT CATFFlush(unsigned int iFileDesc);


/**
 * Reads from a file. 
 * <br><b>Role</b>:Reads from  a file previously opened with CATFOpen. 
 * @param iFileDesc
 *      the file descriptor that has been obtained through @href #CATFOpen
 * @param iBuff 
 *      The buffer where the data will be stored. 
 * @param iSize
 *      The size of iBuff buffer. 
 * @param oRead 
 *      the length of data actually read 
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/ 
  ExportedByJS0LIB HRESULT CATFRead(unsigned int iFileDesc, char *iBuff, 
				    unsigned int iSize, unsigned int *oRead);
  
/**
 * Writes in a file. 
 * <br><b>Role</b>:Writes into a file previously opened with CATFOpen. 
 * @param iFileDesc
 *      the file descriptor that has been obtained through @href #CATFOpen
 * @param iBuff 
 *      The buffer of data to write. 
 * @param iSize
 *      The size of iBuff buffer. 
 * @param oRead 
 *      the length of data actually written. 
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/ 
  ExportedByJS0LIB HRESULT CATFWrite(unsigned int iFileDesc, const char *iBuff,
				     unsigned int iSize, 
				     unsigned int *oWritten);

/**
 * Moves the file pointer. 
 * <br><b>Role</b>: Moves the file pointer of a file previously opened with 
 * CATFOpen. 
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param iOffset
 *    The position of the next I/O operation, which can be either positive 
 *    or negative.
 * @param iOrigine
 *    the origine used to determined the new file pointer. 
 *   <b>Legal values</b>: 
 *   <br><tt> SEEK_SET:</tt> the pointer is set to iOffset.
 *   <br><tt> SEEK_CUR:</tt> the pointer is set to the current position plus 
 *                           iOffset.
 *   <br><tt> SEEK_END:</tt> the pointer is set to the size of the file plus 
 *                           the value of iOffset.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/   
  ExportedByJS0LIB HRESULT CATFSeek ( unsigned int iFileDesc, int iOffset, 
				       int iOrigine);

/**
 * Moves the file pointer. 
 * <br><b>Role</b>: Moves the file pointer of a file previously opened with 
 * CATFOpen. This function is for large files.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param iOffset
 *    The position of the next I/O operation, which can be either positive 
 *    or negative.
 * @param iOrigine
 *    the origine used to determined the new file pointer. 
 *   <b>Legal values</b>: 
 *   <br><tt> SEEK_SET:</tt> the pointer is set to iOffset.
 *   <br><tt> SEEK_CUR:</tt> the pointer is set to the current position plus 
 *                           iOffset.
 *   <br><tt> SEEK_END:</tt> the pointer is set to the size of the file plus 
 *                           the value of iOffset.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/   
  ExportedByJS0LIB HRESULT CATFSeek64( unsigned int iFileDesc, 
				       CATLONG64 iOffset, 
				       int iOrigine);
 
/**
 * Returns the current offset. 
 * <br><b>Role</b>: Returns the current offset in a file previously opened 
 * with CATFOpen. 
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param oOffset
 *    The position of the next I/O operation.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/    
  ExportedByJS0LIB HRESULT CATFTell (unsigned int iFileDesc,
				     unsigned int *oOffset);
 
/**
 * Returns the current offset. 
 * <br><b>Role</b>: Returns the current offset in a file previously opened 
 * with CATFOpen. This function is for large files.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param oOffset
 *    The position of the next I/O operation.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFTell64 (unsigned int iFileDesc,
				       CATLONG64 *oOffset);

/**
 * Determines if the end f the file has been reached.
 * <br><b>Role</b>: Determines if EOF  has previously been detected reading 
 * the file identified by iFileDesc.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen 
  * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt> if EOF has been reached. 
 *   <br><tt> S_FALSE :</tt> if the EOF has not been reached.
 *   <br><tt> negative:</tt> if an error has been encountered.
*/
  ExportedByJS0LIB HRESULT CATFEof (unsigned int iFileDesc);

 
/**
 * Returns the size of a file. 
 * <br><b>Role</b>: Returns the current size of a file previously opened 
 * with CATFOpen. 
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param oSize
 *    The size of the file.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFSize (unsigned int iFileDesc, 
				     unsigned int* oSize);

/**
 * Returns the size of a file. 
 * <br><b>Role</b>: Returns the current size of a file previously opened 
 * with CATFOpen. This function is for large files.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param oSize
 *    The size of the file.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFSize64 (unsigned int iFileDesc, 
				       CATLONG64 * oSize);
  
/**
 * Reads a line from a file.
 * <br><b>Role</b>: Reads characters from the file  into the array pointed to
 * by ioBuff, until iNb-1 characters are read, or a newline character is read 
 * and transferred to ioBuff, or an end-of-file condition is encountered.
 * The string is then terminated with a null character.
 * @param ioBuff
 *    the buffer where the read characters will be stored.
 * @param iNb
 *    The size of the buffer.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFGets ( char * ioBuff, int iNb, 
				      unsigned int iFileDesc);

/**
 * Writes a line into a file.
 * <br><b>Role</b>: Writes the null-terminated string pointed to by iBuff to 
 * the file pointed by iFileDesc. The terminating null character is not written
 * @param iBuff
 *    the string to write.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param oWritten
 *    On success the number of characters actually written.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFPuts ( const char * iBuff, 
				      unsigned int iFileDesc,
				      int *oWritten);
/**
 * Reads a character.
 * <br><b>Role</b>: Obtains the next byte (if present) as an unsigned char 
 * converted to an int, from the input file pointed to by iFileDesc, and 
 * advances the associated file position indicator for the file.
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param oC
 *    On success the read character.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFGetc ( unsigned int iFileDesc, int *oC);


/**
 * Writes a character.
 * <br><b>Role</b>: Writes the byte specified by iChar (converted to an 
 * unsigned char) to the output file pointed to by iFileDesc, at the position
 * indicated by the associated file-position indicator and advances the 
 * indicator appropriately. 
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param iChar
 *    the character to write.
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/
  ExportedByJS0LIB HRESULT CATFPutc ( unsigned int iFileDesc, int iChar);

  
/**
 * Copies a file.
 * <br><b>Role</b>:Copies a given file into a new one.
 * @param iFileName
 *      The path of the file to copy.
 * @param iCopyName
 *      the path of the copy file
 * @param iCopyTime
 *      if set the times of the original file will be preserved.
 * @return
 *   <b>Legal values</b>:
 *   <br><tt> S_OK :</tt>on Success.
 *   <br><tt> negative :</tt> on failure
*/
  ExportedByJS0LIB HRESULT CATFCopy ( const wchar_t *iName, 
				      const wchar_t *iCopyName,
				      int iCopyTime =0);

/**
 * Changes the status of a file. 
 * <br><b>Role</b>: Applies the rights a the file named by iReferenceFile to  
 * the file named by iFileToUpdate. 
 * @param iReferenceFile 
 *      the path of the reference file. 
 * @param iFileToUpdate 
 *      the path of the file which rights have to be modified. 
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/ 
  ExportedByJS0LIB HRESULT CATFChmod (const wchar_t *iReferenceFile, 
				      const wchar_t *iFileToUpdate);

/**
 * Changes the times of a file. 
 * <br><b>Role</b>: Changes the last access and last modification times of
 * a file named by iName
 * @param iName 
 *      the path of the file. 
 * @param iNewAccessTime
 *      the new last access time. 
 * @param iNewModTime
 *      the new last modification time
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/ 
  ExportedByJS0LIB  HRESULT CATFSetTimes ( const wchar_t *iName, 
					   time_t iNewAccessTime,
					   time_t iNewModTime);


/**
 * Truncates the file. 
 * <br><b>Role</b>: Truncates a file previously opened with CATFOpen. 
 * @param iFileDesc
 *    the file descriptor that has been obtained through @href #CATFOpen
 * @param iOffset
 *    The position of the truncature
 * @return 
 *   <b>Legal values</b>: 
 *   <br><tt> S_OK :</tt>on Success. 
 *   <br><tt> negative :</tt> on failure 
*/   
  ExportedByJS0LIB HRESULT CATFTruncate ( unsigned int iFileDesc, off_t iOffset);

  
#ifdef __cplusplus
};
#endif

#if defined(_WINDOWS_SOURCE) && defined(_NATIVE_WCHAR_T_DEFINED)
// Signatures provided for compatibility only
// On Windows, sizeof(wchar_t) == sizeof(unsigned short), so the cast it safe
/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFOpen(const unsigned short *iFileName, const char *iMode, unsigned int* oFileDesc)
{
  return CATFOpen((const wchar_t *)iFileName, iMode, oFileDesc);
}
/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFCopy(const unsigned short *iName, const unsigned short *iCopyName, int iCopyTime =0)
{
  return CATFCopy((const wchar_t *)iName, (const wchar_t *)iCopyName, iCopyTime);
}
/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFChmod(const unsigned short *iReferenceFile, const unsigned short *iFileToUpdate)
{
  return CATFChmod((const wchar_t *)iReferenceFile, (const wchar_t *)iFileToUpdate);
}
/** 
 * @nodoc
 * @deprecated R424
 */
inline HRESULT CATFSetTimes(const unsigned short *iName, time_t iNewAccessTime, time_t iNewModTime)
{
  return CATFSetTimes((const wchar_t *)iName, iNewAccessTime, iNewModTime);
}
#endif

#endif
