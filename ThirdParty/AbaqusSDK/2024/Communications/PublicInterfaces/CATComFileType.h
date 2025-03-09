/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 2003 					                */
/*=======================================================================*/
/*                                                                       */
/*  CATComFileType                                                   */
/*                                                                        */
/*  Usage Notes: Comm Protocol                        */
/*                                                                       */
/*=======================================================================*/
/*  Creation Oct	2000                                 jnm             */
/*=======================================================================*/
#ifndef _CATComFileType_H
#define _CATComFileType_H

// COPYRIGHT DASSAULT SYSTEMES 2003
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//------------------------------------------------------------------

/**
 * Enum
 * <b>Role</b>: Specifies the type of your file referenced by your XML parameters file.
 * Needed for uploads.
 
 *   @see CATBatchParameters.
 */
enum CATComFileType
{
	COMM_FILE_ASCII,
	COMM_FILE_BINARY
};


#endif

