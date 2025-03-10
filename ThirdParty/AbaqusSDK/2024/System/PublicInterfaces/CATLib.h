#ifndef CATSYSTEMSERVICES_H
#define CATSYSTEMSERVICES_H

/*
// COPYRIGHT DASSAULT SYSTEMES 1999
*/

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include "CATDataType.h"
#include "CATSysErrorDef.h" // HRESULT
#include "JS0LIB.h"
#include "DSYSysTSLib.h"    // CATLibStatus

/**
 * Identifying value for Unix plateform used by CATGetOperatingSystem.
 */
#define      CATUnixPlatform      0
/**
 * Identifying value for MS Windows plateform used by CATGetOperatingSystem.
 */
#define      CATWindowsPlatform   1


/**
 * Identifying value for AIX returned by CATGetOperatingSystem.
 */
#define      CATOsAIX               0
/**
 * Identifying value for SOLARIS returned by CATGetOperatingSystem.
 */
#define      CATOsSOLARIS           1
/**
 * Identifying value for Irix returned  by CATGetOperatingSystem.
 */
#define      CATOsIRIX              2
/**
 * Identifying value for HPUX returned by CATGetOperatingSystem.
 */
#define      CATOsHPUX              3
/**
 * Identifying value for Windows NT returned by CATGetOperatingSystem.
 */
#define      CATOsWindowsNT         4
/**
 * Identifying value for Windows95 returned by CATGetOperatingSystem.
 */
#define      CATOsWindows95         5
/**
 * Identifying value for Windows98 returned by CATGetOperatingSystem.
 */
#define      CATOsWindows98         6
/**
 * Identifying value for LINUX returned by CATGetOperatingSystem.
 */
#define      CATOsLINUX             7
/**
 * Identifying value for Windows98 returned by CATGetOperatingSystem. 
 */ 
#define      CATOsWindows2000       8

/**
 * Identifying value for MacOSX returned by CATGetOperatingSystem. 
*/
#define      CATOsMacOSX            9

/**
 * Identifying value for iOS returned by CATGetOperatingSystem. 
*/
#define      CATOsiOS               10

/**
 * Identifying value for an unknown OS returned by CATGetOperatingSystem.
 */
#define      CATUnknownOS           0xffff

/**
 * any version of an operating System.
 */
#define	     CATOsVsANY		    -1


#ifdef _WINDOWS_SOURCE
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "CATUniStd.h"
#include <stdlib.h>


/**
 * CATMaxPathSize is the maximum length allowed for a path
 */
#define CATMaxPathSize		_MAX_PATH

/** @nodoc */
struct _CATDirectory 
{
	struct _finddata_t findData;
	CATLONGPTR findHandle;
	int findDataUsed;
};

#elif defined(unix)

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include "CATUniStd.h"
 
/**
 * CATMaxPathSize is the maximum length allowed for a path
 */
#define CATMaxPathSize		_POSIX_PATH_MAX

/* POSIX_PATH_MAX can be too short (256) */
#if defined(_LINUX_SOURCE)
#if _POSIX_PATH_MAX < 1025
#undef  CATMaxPathSize
#define CATMaxPathSize 1024
#endif
#endif


/** @nodoc */
struct _CATDirectory 
{
	DIR *dir;
};

#endif




/**
 * Information about the Computer.
 * @param HostName 
 * @param OSName 
 * Operating System.
 * @param OSVersion
 * Operating System Version.
 * @param MajorVersion
 * Major Os version.
 * @param MinorVersion
 * Minor Os version.
 * @param OSType
 * Os type .
*/
typedef struct 
{
    char HostName[CATMaxPathSize];
    char OSName[CATMaxPathSize];
    char OSVersion[CATMaxPathSize]; 
    unsigned int  MajorVersion;
    unsigned int MinorVersion;
    unsigned int OSType;
  
} CATSystemInfo;


/**
 * Handle on Directory.
 * <br><b>Role</b>: Structure used as argument by @href CATOpenDirectory 
 * @href CATReadDirectory and @href CATCloseDirectory.
*/
typedef struct _CATDirectory CATDirectory;

/**
 * Directory Entry.
*/
typedef struct {
    char name[CATMaxPathSize];
} CATDirectoryEntry;


/** @nodoc */
typedef CATULONG32 CATTimerId;
/** @nodoc */
#define CATNullTimerId (0xffffffffu)


/**
 * Times manipulation structure.
 * @param sec
 * Second since the first of january 1970.
 * @param usec 
 * Microseconds.
*/
typedef struct 
{
    CATLONG32 sec;		 
    CATLONG32 usec;		       
} CATTimeValue;



#if 0
/** @nodoc */
typedef DATE CATDate;
#else
/** @nodoc */
typedef double CATDate;
#endif
    

#ifdef __cplusplus
extern "C" {
#endif

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATStartLibServices (void);
/** @nodoc */
ExportedByJS0LIB CATLibStatus CATStopLibServices (void);



/**
 * Retrieves the caracteristics of a machine.
 * <br><b>Role</b>: Retrieves the name, operating system name and version of the 
 * computeur.
 * @param iSystemInfo
 *	the structure where the information will be written.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The login name was successfully retrieved.</dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATGetSystemInfo (CATSystemInfo *iSystemInfo); 



#ifdef __cplusplus
/**
 * Retrieves the type of platform.
 * <br><b>Role</b>: Retrieves the type of platform on where the code is running, and the 
 * type of operating system. 
 * <br>This function is used in C++ code. 
 * @param oPlatform
 *	The type of platform. The argument is by default NULL.
 *	<br><b>Legal values</b>: @href CATUnixPlatform or   @href CATWindowsPlatform
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATOsAIX </dt>
 *     <dd> Operating System is AIX.</dd>
 *     <dt>CATOsSOLARIS </dt>
 *     <dd> Operating System is SOLARIS.</dd>
 *     <dt>CATOsIRIX  </dt>
 *     <dd> Operating System is IRIX.</dd>
 *     <dt>CATOsHPUX </dt>
 *     <dd> Operating System is HPUX.</dd>
 *     <dt>CATOsWindowsNT </dt>
 *     <dd> Operating System is WindowsNT.</dd>
 *     <dt>CATOsWindows95  </dt>
 *     <dd> Operating System is Windows95 .</dd>
 *     <dt>CATOsWindows98 </dt>
 *     <dd> Operating System is Windows98 .</dd>
 *     <dt>CATOsLINUX </dt>
 *     <dd> Operating System is LINUX.</dd>
 *     <dt>CATUnknownOS  </dt>
 *     <dd> Operating System is Unknown or an error as occured.</dd>
 * </dl>
 */
ExportedByJS0LIB unsigned short  CATGetOperatingSystem(unsigned short *oPlatform=NULL);
#else
/**
 * Retrieves the type of platform.
 * <br><b>Role</b>: Retrieves the type of platform on where the code is running, and the 
 * type of operating system.
 * <br>This function is used in C code. (no default parameters in C)
 * @param oPlatform
 *	the type of platform.
 *	<br><b>Legal values</b>: @href CATUnixPlatform or   @href CATWindowsPlatform
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATOsAIX </dt>
 *     <dd> Operating System is AIX.</dd>
 *     <dt>CATOsSOLARIS </dt>
 *     <dd> Operating System is SOLARIS.</dd>
 *     <dt>CATOsIRIX  </dt>
 *     <dd> Operating System is IRIX.</dd>
 *     <dt>CATOsHPUX </dt>
 *     <dd> Operating System is HPUX.</dd>
 *     <dt>CATOsWindowsNT </dt>
 *     <dd> Operating System is WindowsNT.</dd>
 *     <dt>CATOsWindows95  </dt>
 *     <dd> Operating System is Windows95 .</dd>
 *     <dt>CATOsWindows98 </dt>
 *     <dd> Operating System is Windows98 .</dd>
 *     <dt>CATOsLINUX </dt>
 *     <dd> Operating System is LINUX.</dd>
 *     <dt>CATUnknownOS  </dt>
 *     <dd> Operating System is Unknown or an error as occured.</dd>
 * </dl>
 */
ExportedByJS0LIB unsigned short  CATGetOperatingSystem(unsigned short *oPlatform);
#endif



#ifdef __cplusplus
/**
 * Ask for the type of operating system.
 * <br><b>Role</b>: Asks if the program runs on a given operating system.
 * <br>This function is used in C++ code. 
 * @param iOSType 
 *<dl>
 *     <dt>CATOsAIX </dt>
 *     <dd> Operating System is AIX.</dd>
 *     <dt>CATOsSOLARIS </dt>
 *     <dd> Operating System is SOLARIS.</dd>
 *     <dt>CATOsIRIX  </dt>
 *     <dd> Operating System is IRIX.</dd>
 *     <dt>CATOsHPUX </dt>
 *     <dd> Operating System is HPUX.</dd>
 *     <dt>CATOsWindowsNT </dt>
 *     <dd> Operating System is WindowsNT.</dd>
 *     <dt>CATOsWindows95  </dt>
 *     <dd> Operating System is Windows95 .</dd>
 *     <dt>CATOsWindows98 </dt>
 *     <dd> Operating System is Windows98 .</dd>
 *     <dt>CATOsWindows2000 </dt>
 *     <dd> Operating System is Windows2000 .</dd>
 *     <dt>CATOsLINUX </dt>
 *     <dd> Operating System is LINUX.</dd>
 * @param iMajorVersion
 *	the major Version of the OS. If iMajorVersion=CATOsVsANY,the default value, then 
 *      Major version is not taken into account for the response.
 * @param iMinorVersion
 *	the minor Version of the OS. If iMinorVersion=CATOsVsANY, the default value, then 
 *      minor version is not taken into account for the response.
 * @return
 * <b>Legal values</b>: CATLibSuccess or CATLibError
 * 
 */
ExportedByJS0LIB CATLibStatus  CATAmIOnOS( int iOSType, 
					   int iMajorVersion = CATOsVsANY ,
					   int iMinorVersion=CATOsVsANY);
#else
/**
 * Ask for the type of operating system.
 * <br><b>Role</b>: Asks if the program runs on a given operating system.
 * <br>This function is used in C code. (no default parameters in C)
 * @param iOSType 
 *<dl>
 *     <dt>CATOsAIX </dt>
 *     <dd> Operating System is AIX.</dd>
 *     <dt>CATOsSOLARIS </dt>
 *     <dd> Operating System is SOLARIS.</dd>
 *     <dt>CATOsIRIX  </dt>
 *     <dd> Operating System is IRIX.</dd>
 *     <dt>CATOsHPUX </dt>
 *     <dd> Operating System is HPUX.</dd>
 *     <dt>CATOsWindowsNT </dt>
 *     <dd> Operating System is WindowsNT.</dd>
 *     <dt>CATOsWindows95  </dt>
 *     <dd> Operating System is Windows95 .</dd>
 *     <dt>CATOsWindows98 </dt>
 *     <dd> Operating System is Windows98 .</dd>
 *     <dt>CATOsWindows2000 </dt>
 *     <dd> Operating System is Windows2000 .</dd>
 *     <dt>CATOsLINUX </dt>
 *     <dd> Operating System is LINUX.</dd>
 * @param iMajorVersion
 *	the major Version of the OS. If iMajorVersion=CATOsVsANY, then 
 *      Major version is not taken into account for the response.
 * @param iMinorVersion
 *	the minor Version of the OS. If iMinorVersion=CATOsVsANY, then 
 *      minor version is not taken into account for the response.
 * @return
 * <b>Legal values</b>: CATLibSuccess or CATLibError
 * 
 */
ExportedByJS0LIB CATLibStatus  CATAmIOnOS( int iOSType, int iMajorVersion,
					   int iMinorVersion);
#endif

/**
 * Retrieves the full path of a file from an official variable name.
 * <br><b>Role</b>: Retrieves the full path of a file located in a file tree referred to by an official environment variable name.
 * @param Filename
 *	the name of the file to locate.
 * @param OffVar
 *	the name of the official environment variable.
 * @param Path
 *	the returned full path of the file.
 * @return
 * <b>Legal values</b>: CATLibSuccess or CATLibError
 * 
 */
ExportedByJS0LIB CATLibStatus CATGetFilePathFromOfficialVariable ( const char * Filename,
                                                                  const char * OffVar,
                                                                  char * Path );
/** @nodoc */
ExportedByJS0LIB const char *CATGetOfficialVariable ( const char *varname );
/** @nodoc */
ExportedByJS0LIB int CATIsActiveLevel ( const char *levname );
/** @nodoc */
ExportedByJS0LIB const char *CATGetEnv (const char *varname);
/** @nodoc */
ExportedByJS0LIB const char *CATGetEnvName ();
/** @nodoc */
ExportedByJS0LIB const char *CATGetEnvDir ();
/** @nodoc */
ExportedByJS0LIB int CATGetEnvStatus ( int  * env_mode,    int *is_default_env,
                                       char * env_name, int env_name_size,
                                       char * env_path, int env_path_size);
/** @nodoc */
ExportedByJS0LIB int MakeEnvManager ();
/**
 * Exports an environment variable.
 * <br><b>Role</b>:Exports an environment variable.
 * @param iAssignexpr
 *	<tt> iAssignexpr</tt>points to a string of the form Name=Value, where Name is 
 *	the environment variable and Value is the new value for it.
 *	this String must be global to the process, or the environnement tables will
 *	be corrupted.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>the environment is correctly set</dd>
 *     <dt>CATLibError</dt>
 *	<dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATPutEnv (const char *iAssignexpr);

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATPutEnvReg (const char *assignexpr);

ExportedByJS0LIB const char *CATGetEnv (const char *varname);
ExportedByJS0LIB CATLibStatus CATPutEnv (const char *iAssignexpr);

/**
 * Opens a directory in order to browse it.
 * <br><b>Role</b>: Opens the directory designated by <tt>iPath</tt> and gives
 * an handle on it, which would be used to identify the directory in subsequent 
 * operations.
 * @param iPath
 *	Names the directory
 * @param oDir
 *	<br><b>Lifecycle rules deviation</b>
 *	the handle on the directory that must be allocated by the caller.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>the directory is opened</dd>
 *     <dt>CATLibError</dt>
 *	<dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATOpenDirectory (const char *iPath, CATDirectory *oDir);
/**
 * Returns a pointer to the next directory entry.
 * <br><b>Role</b>:Returns a pointer to the next directory entry of a directory
 * opened by @href CATOpenDirectory.
 * @param iDir
 * The directory opened by @href CATOpenDirectory.
 * @param oEntry
 *	<br><b>Lifecycle rules deviation</b>
 *	the handle on the next directory Entrythat must have been allocated by 
 *	the caller.
 * @param oEndOfDir
 *	pointer on an integer. When the end of the directory is reached, the pointed
 *	integer is set to 1, otherwise its value remains 0.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>the directory is closed</dd>
 *     <dt>CATLibError</dt>
 *	<dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATReadDirectory (CATDirectory *iDir,
						CATDirectoryEntry *oEntry,
						int *oEndOfDir);
/**
 * Closes a directory.
 * <br><b>Role</b>: Closes the directory designated by the <tt>iDir</tt> handle
 * This handle has been obtained by a previous call to @href CATOpenDirectory
 * @param iDir
 *	the CATDirectory Handle on the directory to be closed
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>the directory is closed</dd>
 *     <dt>CATLibError</dt>
 *	<dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATCloseDirectory (CATDirectory *iDir);


/**
 * Retrieves the system temporary directory.
 * <br><b>Role</b>: Retrieves the path of the directory used by the system to
 * create temporary files.
 * @return
 *	The path of the temporary directory or a Null String if an error occured
*/
ExportedByJS0LIB const char *CATGetTempDirectory ();


/**
 * Creates a directory.
 * <br><b>Role</b>:Creates a directory with the read,write execute rights.
 * @param iPath
 *	Specifies the name of the new directory.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>A new directory was successfully created or a directory
 *     <tt>iPath</tt> already exists</dd>
 *     <dt>CATLibError</dt>
 *	<dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATCreateDirectory (const char *iPath); 

/**
 * Deletes a directory.
 * <br><b>Role</b>: Deletes a directory, that must exist and be already empty.
 * @param iPath
 *	Specifies the name of the directory to be deleted.
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The directory was successfully destroyed</dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATDeleteDirectory (const char *iPath);



/**
 * @nodoc
 * Retrieves informations about a file.
 * <br><b>Role</b>: Obtains information about the file or directory named by the
 * <tt>iPath</tt> parameter  and store it in a CATFileInfo structure.
 * @param iPath
 *	Specifies the path name identifying the file.
 * @param oBuf
 *	Specifies a pointer to the @href CATFileInfo structure in which 
 *	information is returned.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The information about the specified file system object has been 
 *	   successfully retrieved and writen in <tt>oBuff</tt></dd>
 *     <dt>CATLibError</dt>
 *	<dd> An error has occured or the object doesn't exist.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATGetFileInfo (const char *iPath, CATFileInfo *oBuf);


/**
 * @nodoc
 * Determines the accessibility of a file system object.
 * <br><b>Role</b>: Determines the accessibility of a file system object (file or
 * directory.
 * @param iPath
 *	Specifies the path name identifying the file.
 * @param iAccessMode
 *	Specifies the access modes to be checked. This parameter is a bit mask
 *	containing 0 or more of the following values.
 *	<br><b>Legal values</b> 
 *	<br><tt>F_OK</tt> Check the existence of a file.
 *	<br><tt>R_OK</tt> Check read permission.
 *	<br><tt>W_OK</tt> Check write permission.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The requested access is permitted </dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured or the requested access is not granted.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATFileAccess(const char *iPath, int iAccessMode);


/**
 * Renames a file.
 * <br><b>Role</b>:Renames a file.
 * @param iOldpath
 *	the original name of the file.
 * @param iNewpath
 *	the new name of the file.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The requested access is permitted </dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured or the requested access is not granted.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATRenameFile (const char *iOldpath, const char *iNewpath);

/**
 * Deletes a file.
 * <br><b>Role</b>: Deletes a file.
 * @param iPath
 *	Specifies the path of the  file.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>The file has been successfully renamed</dd>
 *     <dt>CATLibError</dt>
 *     <dd> The file could not be renamed </dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus CATDeleteFile (const char *iPath);



/**
 * Constructs the absolute Path of a file.
 * <br><b>Role</b>: Constructs the absolute Path of a file, given its name and 
 * the path of its directory.
 * @param iDir
 *	Specifies the path of the directory of the file.
 * @param iFileName
 *	Specifies the name of the file.
 * @param oFullpath
 *	A pointer to the buffer where the absolute full path of the file will 
 *	be written. The buffer must be allocated by the caller.
*/
ExportedByJS0LIB void CATMakePath (const char *iDir, const char *iFileName,
				   char *oFullpath); 

/**
 * Slipts the absolute path of a file.
 * <br><b>Role</b>: Retrieves the file name and directory Name of a file, given 
 * its full path.
 * @param iFullPath
 *	Absolute path of the file.
 * @param oDir
 *	A pointer to the buffer where the path of the directory of the file
 *	will be written. This buffer must be allocated by the caller.
 * @param oFileName
 * 	A pointer to the buffer where the name of the file
 *	will be written. This buffer must be allocated by the caller.
*/
ExportedByJS0LIB void CATSplitPath (const char *iFullPath, char *oDir,
				    char *oFileName); 

/**
 * Searches for a file inside a given path.
 * <br><b>Role</b>: Searches for a file inside a given path and returns the absolute path
 * of the file if it was found.
 * @param iFilename
 *	Name of  the file to  be searched 
 * @param iPaths
 *	Path where the search will occur
 * @return
 *	If a file named <tt> iFilename</tt> was found, its absolute path is returned,
 *	otherwise NULL.
 */
ExportedByJS0LIB const char *CATFindPath (const char *iFilename, 
					  const char *iPaths);







    /* Time-related Functions */
    /* UTCtime is expressed as seconds and microseconds since 1970/1/1 */
    /* UTCbias is expressed in hours and defined as localTime - UTCTime */


/** @nodoc */
ExportedByJS0LIB void CATGetCurrentTime (CATTimeValue *UTCtime, CATLONG32 *UTCbias);
      
/** @nodoc */
ExportedByJS0LIB CATDate CATGetCurrentDate ();
/** @nodoc */
ExportedByJS0LIB CATTimerId CATResetTimes (CATTimerId id);
/* both times are in milliseconds */
/** @nodoc */
ExportedByJS0LIB CATLibStatus CATGetTimes (CATTimerId id,
					   CATULONG32 *cpu,
					   CATULONG32 *elapsed);
/** @nodoc */
ExportedByJS0LIB CATLibStatus CATReleaseTimes (CATTimerId id);

/**
 * @nodoc
 * Suspends the execution.
 * <br><b>Role</b>: Suspends the execution of the process.
 * @param iMillisecs
 *	the length of the waiting expressed in milliseconds.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>Success</dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured.</dd>
 * </dl>

*/
ExportedByJS0LIB CATLibStatus CATSleep (CATULONG32 iMillisecs); 


/** @nodoc */
#define CATLibMuteProcess 0x0001
/** @nodoc */
#define CATLibWaitProcess 0x0002
/** @nodoc */
#define CATLibConsoleProcess 0x0004
/*
//-----------------------------------------------------------------------------
//
//	CATStartProcess (const char *comPath, char *const argv[], int wait, int *pid, int *exitStatus);
//	const char comPath[] = "PROGRAM_NAME";
//	char  const argv[...][MAX_STRING_SIZE] = { "PROGRAM_NAME", "First_Argument", ...,  "Last_Argument", NULL};
//	int wait = 1; Mode synchrone : Father process waits for the end of child process;
//	int wait = 0; Mode asynchrone; Father process is executed until a waitpid( SonProcessPID, rc) is met.
//	int *exitStatus = 
//		if wait = 1
//			WEXITSTATUS (childStat) on UNIX 
//			return code on _spawnv  on Windows_NT
//		if wait = 0
//			no used
//
//       int iFlags 
//              CATLibMuteProcess:redirect traces of forked process to /dev/null
//	
//-----------------------------------------------------------------------------
*/

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATStartProcessEx (const char *comPath,
					       char *const argv[],
					       int iFlags , int *pid,
					       int *exitStatus,
					       void **oReserved) ;

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATStartProcess (const char *comPath,
					       char *const argv[],
					       int wait, int *pid,
					       int *exitStatus);
/**
 * @nodoc
*/
ExportedByJS0LIB CATLibStatus CATStopProcess (int pid);
/**
 * @nodoc
*/
ExportedByJS0LIB CATLibStatus CATWaitProcess (int pid, int *exitStatus);

/** 
 * Gives the process Id.
 * <br><b>Role</b>: Returns the process Id.
 * @return
 *	the process Id.	
*/
ExportedByJS0LIB CATULONG32 CATGetProcessId ();


/**
 * @nodoc
*/
ExportedByJS0LIB void CATDisplayMsg (const char *message);
/**
 * @nodoc
*/
ExportedByJS0LIB const char *CATGetAppArg (const char *var);
/**
 * @nodoc
*/
ExportedByJS0LIB const char *CATGetAppName ();



#ifdef __cplusplus
/**
 * Gives the list of used disks drivers.
 * <br><b>Role</b>: Gives the list of the letters currently used to mapp a disk. This
 * call is only usefull on Windows platforms, and thus will generate systematically 
 * a CATLibSuccess return code and will return an empty array on Unix.
 * <br>This function is used in C++ code. 
 * @param iBufferSize 
 *	Size of the buffer .
 * @param  iBuffer 
 *	The buffer will be filled  with a character array containing contiguous 
 *	strings separated by NULL characters. The end of the array is marked by two 
 *	NULL characters.
 * @param oLen 
 *  Pointer to contain the length of the array filled. If this parameter 
 *  is NULL, the default, this information is not given.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>Success.</dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus  CATGetDrivesList(int iBufferSize, char *iBuffer,
 int *oLen=NULL);
#else
/**
 * Gives the list of used disks drivers.
 * <br><b>Role</b>: Gives the list of the letters currently used to mapp a disk. This
 * call is only usefull on Windows platforms, and thus will generate systematically 
 * a CATLibSuccess return code and will return an empty array on Unix.
 * <br>This function is used in C code. ( no default parameters in C )
 * @param iBufferSize 
 *	size of the buffer .
 * @param  iBuffer 
 *	the buffer will be filled  with a character array containing contiguous 
 *	strings separated by NULL characters. The end of the array is marked by two 
 *	NULL characters.
 * @param oLen 
 *  Pointer to contain the length of the array filled. If this parameter 
 *  is NULL, this information is not given.
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>Success.</dd>
 *     <dt>CATLibError</dt>
 *     <dd> An error has occured.</dd>
 * </dl>
*/
ExportedByJS0LIB CATLibStatus  CATGetDrivesList(int iBufferSize, char *iBuffer,
 int *oLen);
#endif




/** @nodoc */
ExportedByJS0LIB CATLibStatus CATGetLongName(const char *iShortName, char *oLongName);

/**
 * Retrieves the system temporary directory.
 * <br><b>Role</b>: Retrieves the path of the directory used by the system to
 * create temporary files.
 * @return
 *	The path of the temporary directory or a Null String if an error occured
*/
/** @nodoc */
ExportedByJS0LIB const char *CATGetTempDirectory ();

/** @nodoc */
struct CATIALevelInfo {
	int Version;
	int Release;
	int ServicePack;
	int HotFix;
	const char* BuildDate;
};

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATGetCATIALevelInfo(struct CATIALevelInfo* oCATIALevelInfo);

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus

class CATUnicodeString;

/** @nodoc */
ExportedByJS0LIB HRESULT _CATFileAccess(const CATUnicodeString &iPath, int iMode, int iFlag);
/** @nodoc */
ExportedByJS0LIB HRESULT _CATDeleteFile(const CATUnicodeString &iPath);
/** @nodoc */
ExportedByJS0LIB HRESULT _CATCreateDirectory(const CATUnicodeString &iPath);
/** @nodoc */
ExportedByJS0LIB HRESULT _CATRenameFile(const CATUnicodeString &iOldpath, const CATUnicodeString &iNewpath);
/** @nodoc Compatibility only */
ExportedByJS0LIB CATLibStatus CATRenameFile(const char *iOldpath, const CATUnicodeString &iNewpath);
/** @nodoc Compatibility only */
ExportedByJS0LIB CATLibStatus CATRenameFile(const CATUnicodeString &iOldpath, const char *iNewpath);


/** @nodoc */
ExportedByJS0LIB CATLibStatus CATPutEnv (const CATUnicodeString &iAssignexpr);

/** @nodoc */
inline CATLibStatus CATCreateDirectory(const CATUnicodeString &iPath)
{
    if(FAILED(_CATCreateDirectory(iPath)))
        return CATLibError;
    return CATLibSuccess;
}

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATOpenDirectory (const CATUnicodeString &iPath, CATDirectory *oDir);

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATDeleteDirectory (const CATUnicodeString &iPath);


/** @nodoc */
ExportedByJS0LIB CATLibStatus CATGetFileInfo (const CATUnicodeString &iPath, CATFileInfo *oBuf);

/** @nodoc */
inline CATLibStatus CATFileAccess(const CATUnicodeString &iPath, int iMode, int iFlag = 0)
{
    if(FAILED(_CATFileAccess(iPath, iMode, iFlag)))
        return CATLibError;
    return CATLibSuccess;
}

/** @nodoc */
inline CATLibStatus CATRenameFile(const CATUnicodeString &iOldpath, const CATUnicodeString &iNewpath)
{
    if(FAILED(_CATRenameFile(iOldpath, iNewpath)))
        return CATLibError;
    return CATLibSuccess;
}

/** @nodoc */
inline CATLibStatus CATDeleteFile(const CATUnicodeString &iPath)
{
    if(FAILED(_CATDeleteFile(iPath)))
        return CATLibError;
    return CATLibSuccess;
}

#endif  // __cplusplus

#endif  // CATSYSTEMSERVICES_H
