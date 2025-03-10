#ifndef CATSysGetBatchMode_H
#define CATSysGetBatchMode_H
// COPYRIGHT DASSAULT-SYSTEMES 2003
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0ERROR.h"

/**
 * Type of application.
 * @param  CATSysInteractive
 *   The application is interactive.
 * @param CATSysInteractiveAsBatch
 *   The application designed as interactive is run as batch
 * @param CATSysBatch
 *   The application is fully designed as  a batch
 */
enum CATSysBatchMode 
{
  CATSysInteractive =0,  
  CATSysInteractiveAsBatch=1,
  CATSysBatch=2              
};



/**
 * Retrieves the type of application.
 * <br><b>Role</b>: Retrieves the type of the currently running application, 
 * that can be one of the following:
 *   <li><tt>CATSysInteractive</tt> The application is interactive.</li>
 *   <li><tt>CATSysInteractiveAsBatch</tt>  The application designed as 
 *                                          interactive is run as batch</li>
 *   <li><tt>CATSysBatch</tt> The application is fully designed as a batch</li>
*/
ExportedByJS0ERROR CATSysBatchMode  CATSysGetBatchMode();

#endif
