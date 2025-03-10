#ifndef CATIMessageReceiver_H
#define CATIMessageReceiver_H

// COPYRIGHT DASSAULT SYSTEMES 2000
 
#include "CATICommMsg.h"

/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByCATSysCommunication IID IID_CATIMessageReceiver;
#else
extern "C" const IID IID_CATIMessageReceiver;
#endif

/**
 * Interface to handle backbone messages. 
 * <b>Role</b>: This interface gives the possibility to handle backbone messages
 * when they arrive to the receiver application. This interface must be implemented by
 * an object declared to the receiver by the @href CATICommunicator#AssociateHandler method.
 * @see CATIStreamMsg, CATIBBStreamer, CATICommMsg, CATICommunicator, CATBBMessage, CATICommunicatorLoop
 */

class  ExportedByCATSysCommunication CATIMessageReceiver : public IUnknown
{
  CATDeclareInterface;
  public:
/**
 * Process the incoming backbone message.
 * <br><b>Role</b>: when a backbone message arrives on a process, 
 *  the handler object associated to this message with the 
 *  @href CATICommunicator#AssociateHandler, will be called on the method <tt>HandleMessage</tt>
 * @param iMessage
 *    Pointer to the incomming message
 */
  virtual void HandleMessage(CATICommMsg *iMessage)=0;
};

/**
 * @nodoc
 * Declare the class handler for CATIMessageReceiver : CATIMessageReceiver_var
 */
CATDeclareHandler (CATIMessageReceiver,IUnknown);


#endif
