#ifndef CATICommunicator_H
#define CATICommunicator_H
#include "CATICommMsg.h"

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

class CATIMessageReceiver ;

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByCATSysCommunication IID IID_CATICommunicator;
#else
extern "C" const IID IID_CATICommunicator;
#endif

/**
 * Interface to represent the logical connexion to the backbone bus.
 * <b>Role</b>: This interface provides all the main services to exchange
 * messages with the  backbone bus:
 * <ul>
 *  <li> Gets a connexion to the bus </li>
 *  <li> Declares itself on the bus </li>
 *  <li> Sends a message on the bus </li>
 *  <li> Receives a message from other applications connected to the bus (via the bus)</li>
 *  <li> Declares the messages the process may handle </li>
 *  <li> Associates handlers to these messages </li>
 *  <li> Removes these handlers  </li>
 *  <li> Disconnects from the bus </li>
 * </ul>
 * <br>
 * <b>Example of code</b>:
 * <ul>
 * <li> Get a connexion to the backbone bus with the @href CATGetBackboneConnection global method </li>
 * <li> Declare to be a backbone application with the @href #Declare method</li>
 * <br>
 * To receive message an application should:
 * <ul>
 * <li> Declare to be interested in receiving messages thanks the 
 *      @href #DeclareEventManaged method </li>
 * <li> Declare an handler for message class it want to receive thanks 
 *       to the  @href #AssociateHandler </li> method
 * </ul>
 * <br>
 * To send message an application should:
 * <ul>
 * <li> used the @href #SendRequest method </li>
 * </ul>
 * <li> Disconnect to the backbone bus thanks the @href #Disconnect method </li>
 * </ul>
 * <br> 
 *   @see CATIStreamMsg, CATIBBStreamer, CATICommMsg, CATIMessageReceiver, CATBBMessage, CATICommunicatorLoop
 */
class ExportedByCATSysCommunication CATICommunicator : public IUnknown
{
  CATDeclareInterface;
  public:
  

/** 
 * Declares an application as a backbone application.
 * <br><b>Role</b>: This service should be called only once for a given application during
 * a connexion.
 * @param iAppClass
 *   The identificator under which the application is known on 
 *   the backbone bus.
 * @param iAppType
 *   <b>Legal Values</b>:<tt>CATStandardApp</tt>
 */
 
  virtual HRESULT Declare( CATApplicationClass iAppClass,
                   CATApplicationType iAppType=CATStandardApp)=0;

/** 
 * Declares a backbone message class the application is interested with to receive.
 * <br><b>Role</b>: Permits an application to declare that it is interested with receiving
 * message of class iMsgClass 
 * @param iMsgClass
 *   iMsgClass is the class name of the message class the application wishes to receive
 */
  virtual HRESULT DeclareEventManaged(CATMessageClass iMsgClass)=0;


/** 
 * Declares an handler for a backbone message class.
 * <br><b>Role</b>: Permits to associate a handler object to any arriving message 
 * of class iMsgClass sent for the iAppClassReceiver application.
 * If a message of class iMsgClass incomes to the application iAppClassReceiver,
 * iReceiver will be called  on the @href CATIMessageReceiver#HandleMessage method 
 * to process the message.
 * <br>
 * @param iAppClassReceiver
 *   The  identificator under which the receiver application is known on 
 *   the backbone bus.
 * @param iMsgClass
 *   The message class of the future incoming messages to which we want to 
 *   associate a handler
 * @param iReceiver
 *   This object will be called in the future when a message of type 
 *   iMsgClass sent to the application iAppClassReceiver will arrive on the receiver application
 *
 */
  virtual HRESULT AssociateHandler(CATApplicationClass iAppClassReceiver,
                           CATMessageClass iMsgClass,
                           CATIMessageReceiver *iReceiver)=0;


/** 
 * Removes a backbone message handler.
 * <br><b>Role</b>: This method permits to remove the handler iReceiver of the message
 * identified by its target application iAppClass and its class iMsgClass.
 * <br> This method cancels  effects of the corresponding @href #AssociateHandler method.
 * <br>
 * @param iAppClass
 *   The identificator under which the application is known on the backbone bus
 * @param iMsgClass
 *   The message class to which iReceiver is associated
 * @param iReceiver
 *   The object which handles the message 
 *
 */

  virtual HRESULT RemoveHandler(CATApplicationClass iAppClass,
                           CATMessageClass iMsgClass,
                           CATIMessageReceiver *iReceiver)=0;


/** 
 * @nodoc
 * Removes a backbone message handler.
 * <br><b>Role</b>: This method permits to remove all the associations
 * created with @href #AssociateHandler and where the handler object is identified
 * by iReceiver.
 * <br>
 * @param iReceiver
 *   The object which handles the message 
 *
 */
  virtual HRESULT RemoveHandler(CATIMessageReceiver *iReceiver)=0;

/** 
 * Emits a backbone message on the backbone bus.
 * <br><b>Role</b>: This method permits to send messages to other backbone 
 * applications via the backbone bus.
 * @param iAppClass
 *   Target application of the message
 * @param iMsg
 *   Backbone message to emit
 *
 */
  virtual HRESULT SendRequest( CATApplicationClass iAppClass,
                       CATICommMsg *iMsg)=0;

/** 
 * Permits to wait for the answer of a synchronous backbone message.
 * 
 * <br><b>Role</b>: This method permits to wait for the answer of a given  
 *  backbone message.
 * We advise user to avoid as much as possible synchronous messages and the use
 * of GetRequestAnswer.  
 * <br>Using  synchronous messages and waiting to the answers has the following 
 * disadvantages :
 * <br> - costs more resources
 * <br> - may generate interblocking conditions ( two applications waiting for each other)
 * <p>
 * @param iMsg
 *   synchronous backbone request needing a immediate answer
 * @param oAnswer
 *   Answer to the iMsg request 
 *
 */

  virtual HRESULT GetRequestAnswer( CATICommMsg *iMsg, CATICommMsg **oAnswer, int timeout=-1)=0;


/** 
 * Logical disconnection from the backbone bus.
 * <br><b>Role</b>: This method permits the disconnection from the bus. It's the end of 
 * the connexion done by the @href CATGetBackboneConnection  global method.
 */
  virtual HRESULT Disconnect()=0;


/** 
 * Gets the last error.
 * <br><b>Role</b>: This method permits to get the last error.
 * @param oLastError
 * <b>Legal Values</b>:
 *  <ul>
 *    <li>CATBBErr_PortNotFound  : Unable to contact given port</li>
 *    <li>CATBBErr_UnableToGetPort  : Unable to declare on given port</li>
 *    <li>CATBBErr_AnswerPortAcceptFailed  : failed because invalid answer port</li>
 *    <li>CATBBErr_ErrorWaitingForPort  : error during WaitForPort operation</li>
 *    <li>CATBBErr_InitSendSocketInvalid  : Try to send a message on a close or invalid socket
 *                                          during the declaration phase,
 *                                          contact to bus probably lost </li>
 *    <li>CATBBErr_SendSocketInvalid  : Try to send a message on a close or invalid socket,
 *                                      contact to bus probably lost</li>
 *    <li>CATBBErr_RcvSocketInvalid  : Try to receive a message from a close or invalid socket,
 *                                     contact to bus probably lost</li>
 *    <li>CATBBErr_MessageClassNotFound  : message class not found</li>
 *    <li>CATBBErr_DataStreamingFailure  : error during stream operation</li>
 *    <li>CATBBErr_DataUntreamingFailure  : error during unstream operation</li>
 *    <li>CATBBErr_MustImplementCATIStreamMsg  : message object doesn't implements CATIStreamMsg</li>
 *    <li>CATBBErr_MustNeedAnAnswer  : message object doesn't need answers</li>
 *    <li>CATBBErr_UnableToInstanciateMsg  : unable to instanciate message object</li>
 *    <li>CATBBErr_ReentranceConditionFound  : </li>
 *    <li>CATBBErr_TimeoutOccurred  : </li>
 *    <li>CATBBErr_TargetApplicationNotPresent  : </li>
 *  </ul>
 *   
 */
  virtual HRESULT GetLastErrorId( int *oLastError)=0;

  
};

/**
 * @nodoc
 */
ExportedByCATSysCommunication CATBusDescriptor GetBusDescriptor( const char *iKey=NULL,const char *iHost=NULL);


/**
 * @nodoc
 * Declare the handler class for CATICommunicator :  CATICommunicator_var
 */
CATDeclareHandler (CATICommunicator,IUnknown);


#include "CATGetBackboneConnection.h"
#endif
