#ifndef __CATGETENVVALUE
#define __CATGETENVVALUE
/*
// COPYRIGHT DASSAULT SYSTEMES 1999
*/

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0LIB.h"
#include "CATLib.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves the value of an environment variable.
 * <br><b>Role</b>:Retrieves the value of an environment variable
 * The memory is allocated by the function CATGetEnvValue. The caller must then
 * free the allocated memory by calling the standard libc function free 
 * @param iVarName
 *   The Name of the requested environment variable
 * @param oVarValue
 *   The address where the returned pointer to the string is located.
 *   This argument could be null, if the caller just wants to know if 
 *   the variable is set but is not interested in its value
 * @return
 * <b>Legal values</b> 
 * <dl>
 *     <dt>CATLibSuccess</dt>
 *     <dd>the query succeeds.</dd>
 *     <dt>CATLibError</dt>
 *     <dd> the variable doesn't exist.</dd>
 * </dl>
 */
  ExportedByJS0LIB CATLibStatus CATGetEnvValue (const char *iVarName,
						char ** oVarValue);




#ifdef __cplusplus
};
#endif

#endif
