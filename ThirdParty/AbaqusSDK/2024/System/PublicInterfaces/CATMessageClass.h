#ifndef CATMessageClass_H
#define CATMessageClass_H   42500

/*
// COPYRIGHT DASSAULT SYSTEMES 1999
*/

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATSysCommunication.h"
#include <string.h>
#include <stdint.h> // for uint32_t

/** 
* Type used in backbone. 
* @see CATIStreamMsg 
*/
typedef unsigned int  uint32;

/** @nodoc */
typedef const char    *CATUsername;

/** @nodoc */
typedef const char    *CATHostname;

/**
* Class name of a backbone message.
* @see CATICommunicator, CATICommMsg 
*/
typedef const char *  CATMessageClass;

/** 
* Descriptor of the backbone bus.
* <br>Use the default value: <tt>CATDefaultBus </tt>
*/
enum CATBusDescriptorAddrType { NOT_INITIALISED, OLD, IPv4, IPv6, IPv4_AND_IPv6 };

struct CATBusDescriptor 
{
/** @nodoc */
  int key1;
/** @nodoc */
  int key2;
/** @nodoc */
  int key3;
/** @nodoc */
  int key4;

  CATBusDescriptorAddrType addrType;

  //struct in_addr 
  uint32_t v4address;
  //struct in6_addr 
  char v6address[16];
  
  CATBusDescriptor(void) : 
	key1(0), key2(0), key3(0), key4(0), 
		addrType(NOT_INITIALISED), v4address(0)
  {
  }

  CATBusDescriptor(const CATBusDescriptor & desc) : 
	key1(desc.key1), key2(desc.key2), key3(desc.key3), key4(desc.key4), 
		addrType(desc.addrType), v4address(desc.v4address)
  {	  
	  if (addrType == IPv6 || addrType == IPv4_AND_IPv6)
		memcpy(v6address, desc.v6address, sizeof(v6address));
  }
};

/**
 * Specifier for a backbone message class.
 * <br><b>Role</b>: This specifier is used in the @href CATICommMsg#GetMessageSpecifiers
 * and in the @href CATICommMsg#SetMessageSpecifiers.
 * @param CATMsgSpec_AnswerNeeded
 *   The sender requests an answer
 * @param CATMsgSpec_FirstReceiverOnly
 *   The message will be sent to the first valid destination application only
 * @param CATMsgSpec_Reflexive
 *   The sender requests to receive its own message
 *   (this is rarely needed)
 */
typedef unsigned int CATMessageSpecifiers;
//typedef unsigned int CATBusDescriptor;

/**
* Identificator of a backbone application.
* @see CATICommunicator, CATICommMsg 
*/
typedef const char * CATApplicationClass;

/**
* Type of a backbone application.
* <b>Legal value</b>: <tt>CATStandardApp</tt>
* @see CATICommunicator
*/
typedef unsigned int CATApplicationType ;

/** @nodoc */
enum CATBBServiceType {
	CATBB_WakeMeOnClientDisconnection = 6
};

/** 
* Returns the default backbone bus.
* <br><b>Role</b>:You can use this function in the function @href CATGetBackboneConnection, but the best is to use the type <tt>CATDefaultBus</tt>.
*/
ExportedByCATSysCommunication CATBusDescriptor CATGetDefaultBus();

/** @nodoc */
#define CATStandardApp      0
/** @nodoc */
#define CATAppBatch			0x80000000
/** @nodoc */
#define CATV1App			0x00000008
/** @nodoc */
#define CATAppReserved1DoNotUse 0x00000001
/** @nodoc */
#define CATAppReserved2DoNotUse 0x00000002
/** @nodoc */
#define CATAppReserved3DoNotUse 0x00000004
/** @nodoc */
#define CATDefaultBus       CATGetDefaultBus()
/** @nodoc */
#define CATBBBusClass  "CATV5_Backbone_Bus"
/** @nodoc */
#define CATAllAppClass "CATV5_AllApplications"

/** @nodoc */
#define CATBBMsgReply "CATV5_ReplyToRequest"

/**
 *
 * Messages specifiers 
 *
 */

/** @nodoc */
#define CATMsgSpec_Reflexive           0x00000001
/** @nodoc */
#define CATMsgSpec_DomainWide          0x00000002
/** @nodoc */
#define CATMsgSpec_Acknowledge         0x00000004
/** @nodoc */
#define CATMsgSpec_AnswerNeeded        0x00000008
/** @nodoc */
#define CATMsgSpec_MultiUser           0x00000010
/** @nodoc */
#define CATMsgSpec_FirstReceiverOnly   0x00000040
/** @nodoc */
#define CATMsgSpec_Answer              0x00000080
/** @nodoc */
#define CATMsgSpec_MultipleFirstPart   0x00000100
/** @nodoc */
#define CATMsgSpec_MultipleMiddlePart  0x00000200
/** @nodoc */
#define CATMsgSpec_MultipleEndingPart  0x00000400
/** @nodoc */
#define CATMsgSpec_SpecificUser        0x00001000
/** @nodoc */
#define CATMsgSpec_SpecificBus         0x00002000
/** @nodoc */
#define CATMsgSpec_SpecificDomain      0x00004000
/** @nodoc */
#define CATMsgSpec_SpecificHost        0x00008000
/** @nodoc */
#define CATMsgSpec_SpecificURL         0x00010000
/** @nodoc */
#define CATMsgSpec_SpecificFilter      0x00020000
/** @nodoc */
#define CATMsgSpec_ClientSearchQuery   0x00040000

//les tailles
/** @nodoc */
#define BB_SIZE_SPECIFIC_USER  50
 /** @nodoc */
#define BB_SIZE_SPECIFIC_HOST 256
 /** @nodoc */
#define BB_SIZE_SPECIFIC_DOMAIN 256  
 /** @nodoc */
//#define BB_SIZE_SPECIFIC_URL 1024
 /** @nodoc */
#define BB_SIZE_SPECIFIC_FILTER 256





/** @nodoc */
// Unable to contact given port
#define CATBBErr_PortNotFound                        0x0001 

/** @nodoc */        
// Unable to declare on given port
#define CATBBErr_UnableToGetPort                     0x0002  

/** @nodoc */        
// failed because invalid answer port
#define CATBBErr_AnswerPortAcceptFailed              0x0003

/** @nodoc */
// error during WaitForPort operation
#define CATBBErr_ErrorWaitingForPort                 0x0004

/** @nodoc */
// Try to send a message on a close or invalid socket
// during the declaration phase
// contact to bus probably lost
#define CATBBErr_InitSendSocketInvalid               0x0102         

/** @nodoc */
// Try to send a message on a close or invalid socket
// contact to bus probably lost
#define CATBBErr_SendSocketInvalid                   0x0103         

/** @nodoc */
// Try to receive a message from a close or invalid socket
// contact to bus probably lost
#define CATBBErr_RcvSocketInvalid                    0x0104         

/** @nodoc */
// message class not found
#define CATBBErr_MessageClassNotFound                0x1001  

/** @nodoc */
// error during stream operation
#define CATBBErr_DataStreamingFailure                0x1002  

/** @nodoc */
// error during unstream operation
#define CATBBErr_DataUntreamingFailure               0x1003  

/** @nodoc */
// message object doesn't implements CATIStreamMsg
#define CATBBErr_MustImplementCATIStreamMsg          0x2001

/** @nodoc */
// message object doesn't implements CATICommMsg
#define CATBBErr_MustImplementCATICommMsg            0x2002

/** @nodoc */
// message object doesn't need answers
#define CATBBErr_MustNeedAnAnswer                    0x2003

/** @nodoc */
// unable to instanciate message object
#define CATBBErr_UnableToInstanciateMsg              0x2004

/** @nodoc */
#define CATBBErr_ReentranceConditionFound            0x2005         

/** @nodoc */
#define CATBBErr_TimeoutOccurred                     0x2006

/** @nodoc */
#define CATBBErr_TargetApplicationNotPresent         0x2007

/** @nodoc */
#define CATBBErr_NoAnswerFromDest					 0x2008


//-----------------------------------------
// the same for CAA
//-----------------------------------------
/*
 * message specifiers
 */
/** @nodoc */
#define CATMsgSpecReflexive           0x00000001
/** @nodoc */
#define CATMsgSpecDomainWide          0x00000002
/** @nodoc */
#define CATMsgSpecAcknowledge         0x00000004
/** @nodoc */
#define CATMsgSpecAnswerNeeded        0x00000008
/** @nodoc */
#define CATMsgSpecMultiUser           0x00000010
/** @nodoc */
#define CATMsgSpecFirstReceiverOnly   0x00000040
/** @nodoc */
#define CATMsgSpecAnswer              0x00000080
/** @nodoc */
#define CATMsgSpecMultipleFirstPart   0x00000100
/** @nodoc */
#define CATMsgSpecMultipleMiddlePart  0x00000200
/** @nodoc */
#define CATMsgSpecMultipleEndingPart  0x00000400
/** @nodoc */
#define CATMsgSpecSpecificUser        0x00001000
/** @nodoc */
#define CATMsgSpecSpecificBus         0x00002000
/** @nodoc */
#define CATMsgSpecSpecificDomain      0x00004000
/** @nodoc */
#define CATMsgSpecSpecificHost        0x00008000

/*
 * errors generated by backbone API
 */
/** @nodoc */
// Unable to contact given port
#define CATBBErrPortNotFound                        0x0001 

/** @nodoc */        
// Unable to declare on given port
#define CATBBErrUnableToGetPort                     0x0002  

/** @nodoc */        
// failed because invalid answer port
#define CATBBErrAnswerPortAcceptFailed              0x0003

/** @nodoc */
// error during WaitForPort operation
#define CATBBErrErrorWaitingForPort                 0x0004

/** @nodoc */
// Try to send a message on a close or invalid socket
// during the declaration phase
// contact to bus probably lost
#define CATBBErrInitSendSocketInvalid               0x0102         

/** @nodoc */
// Try to send a message on a close or invalid socket
// contact to bus probably lost
#define CATBBErrSendSocketInvalid                   0x0103         

/** @nodoc */
// Try to receive a message from a close or invalid socket
// contact to bus probably lost
#define CATBBErrRcvSocketInvalid                    0x0104         

/** @nodoc */
// message class not found
#define CATBBErrMessageClassNotFound                0x1001  

/** @nodoc */
// error during stream operation
#define CATBBErrDataStreamingFailure                0x1002  

/** @nodoc */
// error during unstream operation
#define CATBBErrDataUntreamingFailure               0x1003  

/** @nodoc */
// message object doesn't implements CATIStreamMsg
#define CATBBErrMsgDoesNotImplementCATIStreamMsg          0x2001

/** @nodoc */
// message object doesn't implements CATICommMsg
#define CATBBErrMsgDoesNotImplementCATICommMsg            0x2002

/** @nodoc */
// message object doesn't need answers
#define CATBBErrMsgDoesNotNeedAnAnswer                 0x2003

/** @nodoc */
// unable to instantiate message object
#define CATBBErrUnableToInstantiateMsg              0x2004

/** @nodoc */
#define CATBBErrReentranceConditionFound            0x2005         

/** @nodoc */
#define CATBBErrTimeoutOccurred                     0x2006

/** @nodoc */
#define CATBBErrTargetApplicationNotPresent         0x2007
#endif
