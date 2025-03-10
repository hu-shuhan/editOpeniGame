#ifndef CATEventSubscriber_H 
#define CATEventSubscriber_H 

#include "CATDataType.h"

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

class CATEventSubscriber;
class CATNotification;

//----------------------------------------------------------------
// Definition of CATCallback Type : identifier of Callback
/**
 * Callback identifier.
 */
typedef CATLONG32 CATCallback;

//----------------------------------------------------------------
// Definition of CATCallbackEvent Type : identifier of Event
/**
 * Callback event identifier.
 */
typedef CATClassId CATCallbackEvent ;

//----------------------------------------------------------------
// Definition of CATSubscriberData  :  client data 
/**
 * Pointer to callback data.
 * <b>Role</b>:
 * Pointer to the data to pass as argument of the event subscriber's method called. 
 * @see CATEventSubscriber#AddCallback
 */
typedef void * CATSubscriberData;

//----------------------------------------------------------------
// Definition of CATSubscriberMethod   
/**
 * Prototype of callback methods.
 * <b>Role</b>: This callback method is declared by means of
 * the @href CATEventSubscriber#AddCallback method.
 * @param iPublishedEvent
 *   The event in question published by <tt>iPublishingObject</tt>
 * @param iPublishingObject
 *   A pointer to the object that is expected to publish the event
 * @param iNotification
 *   The notification published with the event
 * @param iClientData
 *   Useful data to pass as argument of <tt>iMethodToCall</tt>
 * @param iCallback
 *   The callback identifier
 * @see CATEventSubscriber#AddCallback
 */
typedef void (CATBaseUnknown::*CATSubscriberMethod)(
	 	 CATCallbackEvent   iPublishedEvent,
		 void             * iPublishingObject,
		 CATNotification  * iNotification,
		 CATSubscriberData  iClientData,
		 CATCallback        iCallback);


#endif
