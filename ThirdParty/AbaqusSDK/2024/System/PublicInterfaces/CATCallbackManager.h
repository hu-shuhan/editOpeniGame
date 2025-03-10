#ifndef CATCALLBACKMANAGER_H
#define CATCALLBACKMANAGER_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "JS0SCBAK.h"
#include "CATEventSubscriber.h"
#include "stdlib.h"


/**
 * @nodoc
 * Defines a callback event, given its name.
 * Insert this macro in the header file of the class that defines the callback event.
 */
#define CATDefineCBEvent( name  ) static CATCallbackEvent name();
/**
 * Declares an event, given its name.
 * <b>Role</b>: The event created is assigned to the class that defines it.
 * This class is afterwards able to publish this event, and is therefore called
 * an event publisher.
 * Insert this macro in the header file of the class that should define the event.
 * <br><b>Postcondition</b>: Use the @href CATImplementCBEvent macro
 * in the class source (.cpp) file to create the event.
 * @param name
 *   The event name
 */
#define CATDeclareCBEvent( name  ) static CATCallbackEvent name()
/**
 * Creates a callback event, given its name.
 * <b>Role</b>: The event created is assigned to the class that defines it.
 * This class is afterwards able to publish this event, and is therefore called
 * an event publisher.
 * Insert this macro in the source (.cpp) file of the class that should define the event.
 * <br><b>Precondition</b>: Use the @href CATDeclareCBEvent macro
 * in the class header file to create the event.
 * @param classname
 *   The name of the class that creates the event
 * @param name
 *   The event name
 * @param debug_chain
 *   A character string you can use for debug
 */
#define CATImplementCBEvent( classname, name, debug_chain  ) \
class classname##name##Evt : public CATNotification  \
{                                                      \
  CATDeclareClass ;                                    \
};                                                     \
CATImplementClass(classname##name##Evt,Implementation,CATNotification,CATNull);\
CATCallbackEvent classname::name() { return classname##name##Evt::ClassName() ; }
// do not use
/** @nodoc */
#define CATDefineCBEventInHeader( name  ) static CATCallbackEvent name();
/** @nodoc */
#define CATDefineCBEventInSource( classname, name, debug_chain  ) \
CATCallbackEvent classname::name() { return debug_chain ; }

/** @nodoc */
class CATCallbacksData
{
};

/**
 * Class that manages the callbacks of a given event publisher.
 * <b>Role</b>: A callback manager attached to a given event publisher dispatches
 * the events that this event publisher publishes to all the subscribers.
 * This consists in calling the methods declared by the event subscribers while subscribing.
 */
class ExportedByJS0SCBAK CATCallbackManager : public CATEventSubscriber {

	public:
		 CATDeclareClass;
		/**
		 * @nodoc
		 * Constructs a callback manager.
		 */
		 CATCallbackManager() ;
		 ~CATCallbackManager() ;
		
		 // --- Calls authorized for all ------------------------------------------
		
		/**
		 * Removes all the callbacks added by all the subscribers for a given event.
		 * @param iEvent
		 *   The event for which the callbacks must be removed
		 */
		 void RemoveCallbacksOnEvent(CATCallbackEvent iEvent);
		
		/**
		 * Checks whether the callback manager manages a subscriber list for a given event.
		 * @param iEvent
		 *   The event in question
		 * @return
		 *   <b>Legal values</b>: -1 if a subscriber list is found and 0 otherwise.
		 */
		 int HasClients(CATCallbackEvent iEvent);
		
		/**
		 * Dispatches the event published to all the event subscribers.
		 * <br><b>Role</b>: This method calls all the callback methods declared
		 * by all the subscribers that
		 * subscribe to the event publisher for the given notification.
		 * @param iEvent
		 *   The event which occurs
		 * @param iNotification
		 *   The associated notification
		 * @param iEventPublisher
		 *   The event publisher
		 */
		 void DispatchCallbacks(
		              CATCallbackEvent  iPublishedEvent,
		              CATNotification  *iPublishedNotification=NULL,
		              void             *iEventPublisher=NULL);
		
		/**
		 * Dispatches the event published to all the subscribers.
		 * @param iNotification
		 *   The notification that stands for the event
		 * @param iPublishingObject
		 *   The event publisher
		 */
		 void DispatchCallbacks(
		              CATNotification  *iPublishedNotification=NULL,
		              void             *iEventPublisher = NULL);
		
		/**
		 * Removes all the callbacks.
		 * <br><b>Role</b>: Removes all the callbacks managed by the callback manager
		 * and added to its event publisher.
		 */
		 void RemoveAllCallbacks();
		
		/**
		 * @nodoc
		 * Returns the object's callback manager.
		 */
		 virtual CATCallbackManager * GetCallbackManager();
		
		/**
		 * @nodoc
		 * Reserved for the CATEventSubscriber class instances. Do not use.
		 */
		 CATCallback RegisterCallback(CATCallbackEvent      iEventWhichInterestsMe,
		                              CATBaseUnknown      * iSubscriberWithCBMgr, 
		                              CATSubscriberMethod   iMethodToBeCalled,
		                              CATSubscriberData     idata=NULL);
		
		/**
		 * @nodoc
		 * Reserved for the CATEventSubscriber class instances. Do not use.
		 */
		 void UnregisterCallback(CATCallback iCallbackIWantToRemove);
		
		/**
		 * @nodoc
		 * Reserved for the CATEventSubscriber class instances. Do not use.
		 */
		 void UnregisterSubscriberCallbacks(
		                CATBaseUnknown      * iSubscriberWhichAddedTheCB,
		                CATCallbackEvent      iEventOnWhichTheCBWasAdded,
		                CATSubscriberData     SubscriberDataOfTheCB);
		
		/**
		 * @nodoc
		 * Retrieves the list of events subscribed to this event publisher.
		 * @param oEventsArray
		 *   The retrieved events array 
		 * 	 (this array is allocated by this method : it is up to the caller to delete it by a delete[] call)
		 * @param oArraySize
		 *   The size of the retrieved events array 
		 */
		HRESULT GetSubscribedEvents(
				CATCallbackEvent*& oEventsArray, 
				int& oArraySize);
		  
		/**
		 * @nodoc
		 * Retrieves the list of subscribers on this event publisher for the specified event.
		 * @param iEvent
		 *   The specified event
		 * @param oSubscribers
		 *   The retrieved list of subscribers
		 */
		HRESULT GetSubscribers(
				CATCallbackEvent iEvent, 
				CATListValCATBaseUnknown_var& oSubscribers);

	private:
		CATCallbacksData * _CBdata;
		
		void ToBeRemoved(CATCallback);
		void UpdateRemove();
};

#endif
