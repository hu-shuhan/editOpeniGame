#ifndef CATSysBoolean_H
#define CATSysBoolean_H

/*
// COPYRIGHT DASSAULT SYSTEMES 1999
*/

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**  
 * Definition of a boolean type.
 */
typedef unsigned char CATBoolean;

#ifndef FALSE
static const CATBoolean FALSE = 0;
/**  
 * boolean false value. 
 */
#define FALSE FALSE
#endif

#ifndef TRUE
static const CATBoolean TRUE = 1;
/**  
 * boolean true value. 
 */
#define TRUE TRUE
#endif

#endif
