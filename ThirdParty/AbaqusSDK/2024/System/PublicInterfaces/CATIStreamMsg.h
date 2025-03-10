#ifndef CATIStreamMsg_H
#define CATIStreamMsg_H

// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATBaseUnknown.h"
#include <IUnknown.h>
#include "CATMessageClass.h"
#include "CATSysCommunication.h"

/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */

#ifndef LOCAL_DEFINITION_FOR_IID
extern   ExportedByCATSysCommunication   IID IID_CATIStreamMsg;
#else
extern "C" const IID IID_CATIStreamMsg;
#endif

/**
 * Interface to stream and unstream backbone messages.
 * <b>Role</b>: This interface must be implemented by backbone messages
 * to enable their streaming and unstreaming.
 * <p>
 * If your message contains simple data types, use the methods of 
 * the @href CATIBBStreamer interface implemented by the @href CATBBMessage 
 * component.
 */
class ExportedByCATSysCommunication CATIStreamMsg : public CATBaseUnknown

{
  CATDeclareInterface;
  public:
  /**
   * Unstreams a backbone message.
   * <br><b>Role</b>: A message sent to the backbone bus is streamed.
   * This method allows you to unstream it.
   * If the message contains simple data types,
   * you can use the @href CATIBBStreamer interface to unstream them.
   * <dl>
   * <dt><b>Example</b>:
   * <dd>
   * This example implements <tt>UnstreamData</tt> to unstream a received backbone message.
   * <pre>
   * HRESULT UnstreamData(void *iBuffer, uint32 iLength)
   * {
   *   ...
   *   CATIBBStreamer * pICATIBBStreamer = NULL;
   *   HRESULT rc = QueryInterface(IID_CATIBBStreamer,(void**)&pICATIBBStreamer);
   *   // unstream simple data types
   *   if ( SUCCEEDED(rc) )
   *   {
   *     // Begin by this instruction 
   *     pICATIBBStreamer->BeginUnstream(iBuffer, iLength);
   *
   *	 // Unstream message data (in the same order as when streaming)
   *     // according to its type 
   *	 // -----------------------------------------------------------
   *     pICATIBBStreamer->UnstreamFloat(..); 
   *     pICATIBBStreamer->UnstreamInt(..); 
   *	 ...
   *     // --------------------------
   *     // End by this instruction 
   *     pICATIBBStreamer->EndUnstream();
   *
   *     pICATIBBStreamer->Release();
   *     pICATIBBStreamer = NULL;
   *   }
   *   ...  // unstream other data
   * }
   * </pre>
   * </dd>
   * </dl>
   * @param iBuffer
   *   The buffer containing the raw data to unstream
   * @param iLength
   *   The length of <tt>iBuffer</tt> expressed in bytes
   */
  virtual HRESULT UnstreamData( void *iBuffer, uint32 iLength)=0;


  /**
   * Streams a backbone message.
   * <br><b>Role</b>: To send a message to the backbone bus, the message must be
   * first streamed. 
   * If the message contains simple data types,
   * you can use the @href CATIBBStreamer interface to stream them.
   * Once the message is sent, use @href #FreeStreamData to free the buffer containing
   * the streamed message.
   * <dl>
   * <dt><b>Example</b>:
   * <dd>
   * This example implements <tt>StreamData</tt> to stream a backbone message.
   * <pre>
   * HRESULT StreamData( void **oBuffer, uint32 *oLength)
   * {
   *   ...
   *   CATIBBStreamer * pICATIBBStreamer = NULL ;
   *   HRESULT rc = QueryInterface(IID_CATIBBStreamer,(void**)&pICATIBBStreamer);
   *   // stream simple data types
   *   if ( SUCCEEDED(rc) )
   *   {
   *     // Begin by this instruction 
   *     pICATIBBStreamer->BeginStream();
   *
   *     // Stream each message data according to its type
   *     // ----------------------------------------------
   *     pICATIBBStreamer->StreamFloat(..); 
   *     pICATIBBStreamer->StreamInt(..); 
   *
   *     // End by these 3 instructions 
   *     int Length; 
   *     *oBuffer = pICATIBBStreamer->EndStream(&Length);
   *     *oLength = Length ;
   *
   *     pICATIBBStreamer->Release();
   *     pICATIBBStreamer = NULL;
   *     }
   *	 ... // stream other data and update oLength
   *  }
   *  </pre>
   *  </dd>
   *  </dl>
   *  @param oBuffer
   *     This buffer, to be allocated, contains all the streamed data 
   *  @param oLength
   *     The length of <tt>oBuffer</tt> expressed in bytes
   */
  virtual HRESULT StreamData( void **oBuffer, uint32 *oLength)=0;


  /**
   * Frees the backbone message streaming buffer.
   * <br><b>Role</b>: Any backbone message sent uses a buffer allocated by the @href #StreamData method.
   * This buffer must be freed as soon as the message is sent.
   * If your message contains simple data types,
   * you can use the @href CATIBBStreamer interface to free them.
   * <dl>
   * <dt><b>Example</b>:
   * <dd>
   * This example implements <tt>FreeStreamData</tt> to free the buffer containing a streamed backbone message.
   * <pre>
   * HRESULT FreeStreamData(void *Buffer, uint32 iLength)
   * {
   *   ...
   *   CATIBBStreamer * pICATIBBStreamer = NULL ;
   *   HRESULT rc = QueryInterface(IID_CATIBBStreamer,(void**)&pICATIBBStreamer);
   *   if ( SUCCEEDED(rc) )
   *   {
   *     pICATIBBStreamer->ResetStreamData();  // free simple data types at once
   *
   *     pICATIBBStreamer->Release();
   *     pICATIBBStreamer = NULL ;
   *   }
   *   ... // free other data 
   * }
   * </pre>
   * </dd>
   * </dl>
   * @param iBuffer
   *   The buffer to be freed.
   *   It was created by the @href #StreamData method 
   * @param iLength
   *   The <tt>iBuffer</tt> length expressed in bytes
   */
  virtual HRESULT FreeStreamData( void *Buffer, uint32 iLength)=0;


  /**
   * Sets backbone message class name and options.
   * <br><b>Role</b>: Use the @href CATICommMsg interface to set 
   * options and class message name. The class name of the message is mandatory, but for
   * specifiers, if you precise anything your message is without answer 
   * and received by all destinator application ( @href CATMessageSpecifiers )
   * <dl>
   * <dt><b>Example</b>:
   * <dd>
   * This example implements <tt>SetMessageSpecifications</tt>
   * <pre>
   * HRESULT SetMessageSpecifications()
   * {
   *   ...
   *   CATICommMsg * pICATICommMsg = NULL;
   *   HRESULT rc = QueryInterface(IID_CATICommMsg,(void**)&pICATICommMsg);
   *  
   *   if ( SUCCEEDED(rc) )
   *   {
   *     // To set the message class name (mandatory)
   *     // MessageClassName is the name of the component. 
   *     pICATICommMsg->SetMessageClass(MessageClassName);
   *
   *     // To set options (if necessary)
   *     pICATICommMsg->SetMessageSpecifiers(..|..|..);
   *
   *     pICATICommMsg->Release();
   *     pICATICommMsg = NULL;
   *   }
   * }
   * </pre>
   * </dd>
   * </dl>
   */
  virtual HRESULT SetMessageSpecifications()=0;
};

/**
 * @nodoc
 * Declaration of the handler class  for CATIStreamMsg : CATIStreamMsg_var
 */
CATDeclareHandler( CATIStreamMsg, CATBaseUnknown );

#endif
