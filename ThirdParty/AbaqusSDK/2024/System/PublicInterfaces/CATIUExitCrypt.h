/* COPYRIGHT DASSAULT SYSTEMES 2002 */
/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */


#ifndef CATIUExitCrypt_H 
#define CATIUExitCrypt_H
#include <stdlib.h>

#include "IUnknown.h"
#include "JS0CRYPTEXIT.h"
 
extern "C" const IID IID_CATIUExitCrypt;


     
/**
 *  Interface to enable some user specific encryption mechanism.
 *  <b>Role</b>: This interface is a user exit, that allows an user to perform
 *  encryption/decryption of data. It is called when transmitting passwords. 
 *  <br>You should implement it on the <tt>CATUExitCrypt</tt> component by 
 *  creating a data extension.
 **/


class ExportedByJS0CRYPTEXIT CATIUExitCrypt : public IUnknown
{
public:

/**
  * Code a given buffer.
  * @param iBuffer
  *  Input Buffer to be coded 
  * @param iLen
  *  Length of the input buffer 
  * @param oCodedBuffer
  *  Output Buffer. It must be allocated by the method Code
  * @param oCodedLen
  *  Length of the output buffer
  * @return
  *  S_OK 
  *  E_FAIL if the coding is not succesfull
 */

  virtual HRESULT Code ( const void *iBuffer, size_t iLen, 
			 void **CodedBuffer, size_t *oCodedLen)=0;
/**
 * Decode a given Buffer.
 * @param iCodedBuffer
 *  Input Buffer to be decoded 
 * @param iCodedLen
 *  Length of the input buffer 
 * @param oDecodedBuffer
 *  Output Buffer. It must be allocated by the method Decode
 * @param oDecodedLen
 *  Length of the output buffer
 * @return
 *  S_OK 
 *  E_FAIL if the decoding is not succesfull
 */
  virtual HRESULT Decode (const void *iCodedBuffer, size_t iCodedLen, 
			  void **DecodedBuffer, size_t *oDecodedLen)=0;
};

#endif
