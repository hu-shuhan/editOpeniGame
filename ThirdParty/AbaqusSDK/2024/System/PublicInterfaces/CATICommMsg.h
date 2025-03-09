#ifndef CATICommMsg_H
#define CATICommMsg_H

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

#include "CATBaseUnknown.h"
#include <IUnknown.h>
#include "CATMessageClass.h"
#include "CATSysCommunication.h"

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByCATSysCommunication IID IID_CATICommMsg;
#else
extern "C" const IID IID_CATICommMsg;
#endif
//class CATICommMsg_var; // - 03.02.2010

/**
 * Interface to access the backbone messages services.
 * <b>Role</b>: This interface provides methods to handle backbone messages.
 * It is implemented by the @href CATBBMessage component.
 * Since your message component derives from this component, it inherits
 * the implementation of CATICommMsg, and you don't need to implement it.
 * In most cases, the @href #SetMessageClass and @href #SetMessageSpecifiers methods
 * should be called in the @href CATIStreamMsg#SetMessageSpecifications method.
 */
class ExportedByCATSysCommunication CATICommMsg : public IUnknown

{
  CATDeclareInterface;
  public:
 /**
  *
  * Returns the backbone message class name.
  * <br><b>Role</b>:The message sender has specified the message class name.
  * 
  * @param oClassName
  *    The returned message class name 
  *    Deletion of the returned  oClassName is under the responsibility
  *    of the caller of GetMessageClass
  *    The returned oClassName must be deleted by delete []
  *    when it does not 
  *
  */
  virtual HRESULT  GetMessageClass( CATMessageClass *oClassName)=0;

 /**
  * @nodoc 
  * Returns the backbone message class name.
  * <br><b>Role</b>:The message sender has specified the message class name.
  *  @return 
  *    The message class name 
  */
  virtual CATMessageClass GetMessageClass()=0;

 /**
  * Sets the backbone message class name.
  * <br><b>Role</b>: The message class name is used to create the message using its name,
  * thanks to the @href CATInstantiateComponent global function that uses
  * the @href CATICreateInstance interface implemented by the message component.
  * To enable for this, the message class name must be declared by the message sender.
  * Use this method in the @href CATIStreamMsg#SetMessageSpecifications method.
  * @param iMsgClass
  *   The message class name 
  */ 
  virtual HRESULT SetMessageClass( CATMessageClass iMsgClass )=0;

 /** 
  * @nodoc 
  * Returns the destinator application identificator.
  * <br><b>Role</b>:The application which has sent the message has specified the
  * identificator of the destinator application. 
  * @return 
  *   The destinator application  identificator
  */
  virtual CATApplicationClass GetDestinatorAppClass()=0;

  /** 
   * @nodoc 
   * Sets the destinator application identificator.
   * <br><b>Role</b>: To send a message you must precise the identificator of the destinator
   * Application. Use this method in the @href CATIStreamMsg#SetMessageSpecification method.
   *  @param iApplicationClass
   *    the destinator application identificator
   */
  virtual HRESULT SetDestinatorAppClass(CATApplicationClass iApplicationClass)=0;


  /** 
   * @nodoc 
   */
  virtual HRESULT SetDestinatorAppType(CATApplicationType  iApplicationType)=0;

 /** 
  * @nodoc 
  */
  virtual CATApplicationType  GetDestinatorAppType()=0;



  /** 
   * @nodoc
   */
  virtual CATBusDescriptor  GetDestinatorBus()=0;

  /** 
   * @nodoc
   */
  virtual HRESULT SetDestinatorBus(CATBusDescriptor iDestBus)=0;


  /** 
   * @nodoc
   */
  virtual HRESULT SetDestinatorDomain(CATBusDescriptor iDestBus)=0;
  
  /** 
   * @nodoc
   */
  virtual HRESULT GetDestinatorDomain(CATBusDescriptor *iDestBus)=0;


 /**
  * 
  * Returns the message specifiers.
  *  @param oMsgSpecifiers
  *   The specifiers of the message
  * @see CATMessageSpecifiers
  */
  virtual HRESULT GetMessageSpecifiers(CATMessageSpecifiers *oMsgSpecifiers)=0;

 /**
  * @nodoc
  * 
  * Returns the message specifiers.
  * @return 
  *   The specifiers of the message
  * @see CATMessageSpecifiers
  */
  virtual CATMessageSpecifiers GetMessageSpecifiers( )=0;

 /**
  * Sets the message specifiers.
  * <br><b>Role</b>: The application that sends the message can set options or specifiers
  * to the message, such as a request for an answer.
  * Use this method in the @href CATIStreamMsg#SetMessageSpecifications method.
  * @param iMsgSpecifiers
  *   A mask giving the specifiers of the message.
  *   <br> Several values may be given combined by "binary OR" logical operator
  * @see CATMessageSpecifiers
  */
  virtual HRESULT SetMessageSpecifiers( CATMessageSpecifiers iMsgSpecifiers )=0;


 /**
  * Allows the message receiver to answer a message.
  * <br><b>Role</b>: A message sender may request that a message needs an answer.
  * Such messages' specifiers include <tt>CATMsgSpec_AnswerNeeded</tt>.
  * To answer, the message receiver should just create a new message that contains
  * the answer, and call <tt>Answsers</tt>.
  * @param iAnswer
  *   The backbone message object representing the answer
  * <dt><b>Example</b>:
  * <dd>Suppose that you have received the message <tt>MyReceivedMessage</tt> that requests
  * an answer.
  * <pre>
  * ...
  * if (MyReceivedMessage->GetSpecifiers() == CATMsgSpec_AnswerNeeded)
  * {
  *   ... // Create a new message that contains the answer and ask for
  *   ... // a pointer to CATICommsg: piCommMsgOnAnswer
  *   MyReceivedMessage->Answers(piCommMsgOnAnswer)
  *   ... 
  * </pre>
  */
  virtual HRESULT Answers(CATICommMsg *iAnswer)=0;


  /** 
   * @nodoc
   * Get the username of the owner of the sender process
   *
  */
  virtual CATUsername GetSenderUsername()=0;

  /** 
   * @nodoc
   * Get the hostname of the sender process
   * This method permits to specify a Destinator Application Type 
  */
  virtual CATHostname GetSenderHostname()=0;

   /** 
   * Get DestinatorHost.                                    
   * This method permits to get the destinator bus.
   */
  virtual HRESULT  GetDestinatorHost(char **oHost)=0;

  /** 
   * Set DestinatorHost.                                               
   * This method permits to specify the destinator bus.
   */
  virtual HRESULT SetDestinatorHost(const char *iHost)=0;

  /** 
   * Get DestinatorUser.                                               
   * This method permits to get the destinator user.
   */
  virtual HRESULT  GetDestinatorUser(char **oUser)=0;

  /** 
   * Set DestinatorUser.                                               
   * This method permits to specify the destinator user.
   */
  virtual HRESULT SetDestinatorUser(const char *iUser)=0;


   /** 
   * GetRequestID.                                            
   * This method returns the RequestID.
   */
  virtual int GetRequestID()=0;




};

/**
 * Declaration of the handler class  for CATICommMsg : CATICommMsg_var
 */
CATDeclareHandler( CATICommMsg, IUnknown );

#endif
