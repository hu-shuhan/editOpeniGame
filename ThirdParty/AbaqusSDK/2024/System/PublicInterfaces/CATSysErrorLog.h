#ifndef CATSysErrorLog_H
#define CATSysErrorLog_H
// COPYRIGHT DASSAULT SYSTEMES 2003

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0ERROR.h"
#include "IUnknown.h"
class CATUnicodeString;

/** 
 * Severity of a logged message.
 * <b>Role</b>: Defines three levels of severity for messages logged 
 * via the function @href CATSysErrorLog.
 * @param CATSysErrorLogSeverityError
 *   Level for Error messages, generally signaling that an unhandled problem occured and
 *   may cause immediate ou future damage to the application
 * @param CATSysErrorLogSeverityWarning
 *   Level for Warning messages, generally signaling that a problem occured
 *   but was successfully handled or was not harmful to the application
 * @param CATSysErrorLogSeverityComment
 *   Level for Comment messages, generally not signaling a problem, but rather 
 *   an information. Using Comment messages is not recommended.
 * @see CATSysErrorLog
 */
enum CATSysErrorLogSeverity 
{
  CATSysErrorLogSeverityError=1, 
  CATSysErrorLogSeverityWarning=2,
  CATSysErrorLogSeverityComment=3
};

/**
 * Output a message to a dedicated log.
 * <br><b>Role</b>: Allows to log various messages in a dedicated location.
 * This error logging may be activated via Tools/Option.
 * If the logging output is active, any message declared with CATSysErrorLog will appear in the corresponding log.
 * Otherwise, no message will appear.
 * @param iDefectClassOrSoftware
 *   Defines the class of the logged message.
 *   This parameter allows to create families of defects. 
 *   Its value may be the name of a framework or any meaningful error class. 
 *   <br><b>Examples</b>: 
 *   <ul>
 *	   <li>Geometric Error</li>
 *	   <li>Visualization Error</li>
 *	   <li>Update Error</li>
 *	   <li>I/O Error</li>
 *   </ul>
 * @param iSeverity
 *   Defines the severity level of the logged message.
 *   <br><b>Legal values</b>:
 *   <ul>
 *     <li>CATSysErrorLogSeverityError</li>
 *     <li>CATSysErrorLogSeverityWarning</li>
 *     <li>CATSysErrorLogSeverityComment</li>
 *   </ul>
 * @param iParameteredNLSMessage
 *   The logged message to be built with an NLS message catalog.
 *   This NLS message may have parameters.
 *   Characters ":" and ";" are forbidden. Carriage returns are not recommended.
 *   This allows the administrator to get messages in native language.
 * @param iNonNLSErrorMsg
 *   The logged message in English, built in the code itself.
 *	 Characters ":" and ";" are forbidden. Carriage returns are not recommended.
 *	 This allows to get messages understandable by most developpers (useful for debugging purpose).
 * @see CATSysErrorLogSeverity
 */
ExportedByJS0ERROR HRESULT CATSysErrorLog( const char *iDefectClassOrSoftware, CATSysErrorLogSeverity iSeverity, CATUnicodeString *iParameteredNLSMessage, CATUnicodeString *iNonNLSErrorMsg );

#endif
