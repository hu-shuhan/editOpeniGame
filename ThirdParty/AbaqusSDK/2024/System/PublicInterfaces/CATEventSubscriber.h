#ifndef CATEVENTSUBSCRIBER_H
#define CATEVENTSUBSCRIBER_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include <stdlib.h>
#include "JS0SCBAK.h"
#include "CATNotification.h"
#include "CATLISTV_CATBaseUnknown.h"

class CATCallbackManager ;
class TIECATIEventSubscriberCATEventSubscriber;



//----------------------------------------------------------------
// Class CATEventSubscriber             
//----------------------------------------------------------------
/**
 * Base class for event publishers and subscribers.
 * <b>Role</b>: Objects that publish events and objects
 * that subscribe to published events can derive from CATEventSubscriber.
 * Preferably, use the global methods to add and remove callbacks to event subscribers.
 * @see AddCallback, RemoveCallback, RemoveSubscriberCallbacks, RemoveCallbacksOn, DispatchCallbacks, GetDefaultCallbackManager
 */
class ExportedByJS0SCBAK CATEventSubscriber : public CATBaseUnknown
{
  friend class TIECATIEventSubscriberCATEventSubscriber;

/**
 * @nodoc
 */
  CATDeclareClass;

  public:

    // Constructor ----------------------------------------------
/**
 * Constructs an event subscriber.
 */
    CATEventSubscriber() ;

    // Destructor -----------------------------------------------
    ~CATEventSubscriber() ;

/**
 * Adds a callback for a given event published by a given event publisher.
 * <br><b>Role</b>:
 * The current event subscriber subscribes to a given event that is published
 * by a given event publisher.
 * Whenever this event is published by this event publisher,
 * the method declared when subscribing is called, along with possible useful data.
 * @param iEventPublisher
 *   A pointer to the object that is expected to publish the event
 * @param iPublishedEvent
 *   The event in question published by <tt>iEventPublisher</tt>
 * @param iMethodToCall
 *   The subscriber's method to call whenever <tt>iEventPublisher</tt>
 *   publishes <tt>iPublishedEvent</tt>
 * @param iClientData
 *   Useful data to pass as an argument of <tt>iMethodToCall</tt>.
 * @return The identifier of the created callback.
 */
    virtual CATCallback AddCallback(CATBaseUnknown * iEventPublisher, 
				    CATCallbackEvent     iPublishedEvent,
				    CATSubscriberMethod  iMethodToCall, 
				    CATSubscriberData    iClientData=NULL);
    // Remove a callback ----------------------------------------
    // Remove a callback of this Client on a CATCallbackManager -
/**
 * Removes a callback with a given identifier from a given event publisher.
 * <br><b>Role</b>:
 * The current event subscriber resigns its subscription to a given event publisher
 * made thanks to a given callback.
 * @param iEventPublisher
 *   A pointer to the object from which the callback <tt>iCallback</tt>
 *   is to be removed
 * @param iCallback
 *   The identifier of the callback to remove.
 *   <br><b>Legal values</b>: This identifier was returned by
 *   the @href #AddCallback method.
 */
    virtual void RemoveCallback(CATBaseUnknown * iEventPublisher,
                                CATCallback          iCallback) ;

    // Remove all callbacks which concern :
    //        the same CATCallbackManager
    //        the same CATCallbackEvent
    //        the same CATSubscriberData
/**
 * Removes all the callbacks from a given event published by a given event publisher.
 * <br><b>Role</b>:
 * The current event subscriber resigns its subscription to all the callbacks
 * set for a given event published by a given event publisher, and
 * with a given client data pointer.
 * @param iEventPublisher
 *   A pointer to the object from which the callback <tt>iCallback</tt>
 *   is to be removed
 * @param iPublishedEvent
 *   The event in question published by <tt>iEventPublisher</tt>
 * @param iClientData
 *   Useful data to pass as an argument of the method called
 */
    virtual void RemoveCallbacksOn(CATBaseUnknown * iEventPublisher,
                                   CATCallbackEvent     iPublishedEvent,
                                   CATSubscriberData    iClientData);

    // Remove all callbacks of this Client on a CATCallbackManager --
/**
 * Removes all the callbacks for all events published by a given event publisher.
 * <br><b>Role</b>:
 * The current event subscriber resigns its subscription to all the callbacks
 * set for all the events published by a given event publisher.
 * @param iEventPublisher
 *   A pointer to the object from which all the callbacks are to 
 *   be removed.
 */
    virtual void RemoveSubscriberCallbacks(CATBaseUnknown * iEventPublisher);

    public:
    // Give the associated CATCallbackManager if it exists ----------
/**
 * Returns an event publisher's callback manager.
 * <br><b>Warning</b>: Do not redefine this method.
 */
    virtual CATCallbackManager *GetCallbackManager() ;

    //---------------------------------------------------------------
    // to support callbacks on interfaces   
    //---------------------------------------------------------------
    // Add a callback on an interface ---------------------------
/**
 * @nodoc
 * Adds a callback on a publishing object for a subscriber whose method
 * declared to be called is part of an interface this subscriber implements.
 * The interface and the requested method to call along with possible useful
 * data are specified.
 * @param iPublishingObject
 *   A pointer to the object that is expected to publish the event.
 * @param iPublishedEvent
 *   The event in question published by <tt>iPublishingObject</tt>.
 * @param iMethodToCall
 *   The subscriber's method to call whenever <tt>iPublishingObject</tt>
 *   publishes <tt>iPublishedEvent</tt>.
 * @param iInterfaceName
 *   Name of the interface implemented by the subscriber and which contains
 *   <tt>iMethodToCall</tt>.
 * @param iClientData
 *   Useful data to pass as arguments of <tt>iMethodToCall</tt>.
 * @return The identifier of the created callback.
 */
    virtual CATCallback AddSubscription(CATBaseUnknown     * iPublishingObject, 
					CATCallbackEvent     iPublishedEvent,
					CATSubscriberMethod  iMethodToCall, 
					char               * iInterfaceName,
					CATSubscriberData    iClientData=NULL);
/** @nodoc */
    virtual CATCallback AddSubscription(CATBaseUnknown     * iPublishingObject, 
					CATCallbackEvent     iPublishedEvent,
					CATSubscriberMethod  iMethodToCall, 
					const IID &          iInterfaceId,
					CATSubscriberData    iClientData=NULL);


    // Remove all callbacks which concern :
    //        the same CATCallbackManager
    //        the same CATCallbackEvent
    //        the same CATSubscriberData
/**
 * @nodoc
 * Removes all the callbacks managed for the subscriber in question 
 * on a publishing object when the method called is part of an interface
 * this subscriber implements, for a given event, a given interface and
 * a given client data pointer.
 * @param iPublishingObject
 *   A pointer to the object from which all the callbacks are to 
 *   be removed.
 * @param iPublishedEvent
 *   The event in question published by <tt>iPublishingObject</tt>.
 * @param iInterfaceName
 *   Name of the interface implemented by the subscriber.
 * @param iClientData
 *   Useful data to pass as arguments of the method called.
 */
    virtual void RemoveSubscriptionsOn(CATBaseUnknown    * iPublishingObject,
                                       CATCallbackEvent    iPublishedEvent,
                                       char              * iInterfaceName,
                                       CATSubscriberData   iClientData);
/** @nodoc */
    virtual void RemoveSubscriptionsOn(CATBaseUnknown    * iPublishingObject,
                                       CATCallbackEvent    iPublishedEvent,
				       const IID &         iInterfaceId,
                                       CATSubscriberData   iClientData);

    // Remove all callbacks of this Client on a CATCallbackManager --
/**
 * @nodoc
 * Removes all the callbacks managed for the subscriber in question
 * on a publishing object when the method called is part of an interface
 * this subscriber implements, for a given interface.
 * @param iPublishingObject
 *   A pointer to the object from which all the callbacks are to
 *   be removed.
 * @param iInterfaceName
 *   Name of the interface implemented by the subscriber.
 */
    virtual void RemoveInterfaceSubscriptions(CATBaseUnknown * iPublishingObject,
                                              char          * iInterfaceName);
/** @nodoc */
    virtual void RemoveInterfaceSubscriptions(CATBaseUnknown * iPublishingObject,
				              const IID &         iInterfaceId);

};

/**
 * Adds a callback for a given event subscriber to a given event publisher and a given event.
 * <b>Role</b>:
 * This function makes the event subscriber subscribe to a given event that is published
 * by a given event publisher.
 * Whenever this event is published by this event publisher,
 * the method declared when subscribing is called, along with possible useful data.
 * @param iEventSubscriber
 *   The event subscriber, that is the object that features the 
 *   <tt>iMethodToCall</tt> method called whenever <tt>iEventPublisher</tt> publishes
 *   <tt>iPublishedEvent</tt>
 * @param iEventPublisher
 *   The event publisher that is expected to publish the event.
 * @param iPublishedEvent
 *   The event in question published by <tt>iEventPublisher</tt>.
 * @param iMethodToCall
 *   The subscriber's method to call whenever <tt>iEventPublisher</tt>
 *   publishes <tt>iPublishedEvent</tt>.
 * @param iClientData
 *   Useful data to pass as an argument of <tt>iMethodToCall</tt>.
 * @return The identifier of the created callback.
 */
ExportedByJS0SCBAK CATCallback AddCallback( CATBaseUnknown    *iEventSubscriber,
                         CATBaseUnknown    *iEventPublisher,
                         CATCallbackEvent  iPublishedEvent,
                         CATSubscriberMethod iMethodToCall,
                         CATSubscriberData  iClientData=NULL );

/**
 * Removes a callback with a given identifier added for a given event subscriber to a given event publisher.
 * <br><b>Role</b>:
 * This function makes the event subscriber resign its subscription to a given event publisher
 * made thanks to a given callback.
 * @param iEventSubscriber
 *   The event subscriber
 * @param iEventPublisher
 *   The event publisher from which the callback <tt>iCallback</tt> is to be removed
 * @param iCallback
 *   The identifier of the callback to remove.
 *   <br><b>Legal values</b>: This identifier was returned by
 *   the @href AddCallback method.
 */
ExportedByJS0SCBAK void RemoveCallback( CATBaseUnknown    *iEventSubscriber,
                            CATBaseUnknown    *iEventPublisher,
                            CATCallback       iCallback);

/**
 * Removes all the callbacks added for a given event subscriber to a given event publisher.
 * <br><b>Role</b>:
 * This function makes the event subscriber resigns its subscription to all the callbacks
 * added for all the events published by a given event publisher.
 * @param iEventSubscriber
 *   The event subscriber
 * @param iEventPublisher
 *   The event publisher from which all the callbacks are to be removed.
 */
ExportedByJS0SCBAK void RemoveSubscriberCallbacks( CATBaseUnknown    *iEventSubscriber,
                                       CATBaseUnknown    *iEventPublisher);

/**
 * Removes a callback added for a given event subscriber to a given event publisher and a given event.
 * <br><b>Role</b>:
 * This function makes the event subscriber resign its subscription to a given event publisher
 * for a given event and associated data.
 * @param iEventSubscriber
 *   The event subscriber
 * @param iEventPublisher
 *   The event publisher from which the callback <tt>iCallback</tt>
 *   is to be removed
 * @param iPublishedEvent
 *   The published event
 * @param iCallback
 *   The identifier of the callback to remove.
 *   <br><b>Legal values</b>: This identifier was returned by
 *   the @href AddCallback method.
 */
ExportedByJS0SCBAK void RemoveCallbacksOn( CATBaseUnknown    *iEventSubscriber,
                            CATBaseUnknown    *iEventPublisher,
                            CATCallbackEvent  iPublishedEvent,
                            CATSubscriberData  iClientData=NULL );

/**
 * Dispatches the callbacks set to a given event publisher for a given notification.
 * <br><b>Role</b>:
 * This function calls all the callback methods declared by all the subscribers that
 * subscribe to the event publisher for the notification.
 * @param iEventPublisher
 *   The event publisher
 * @param iPublishedNotification
 *   The published notification
 */
ExportedByJS0SCBAK void DispatchCallbacks( CATBaseUnknown    *iEventPublisher,
                        CATNotification  *iPublishedNotification);

/**
 * Returns an event publisher's default callback manager.
 * @param iEventPublisher
 *   The event publisher
 * @return
 *   The default callback manager
 */
ExportedByJS0SCBAK CATCallbackManager  *GetDefaultCallbackManager(CATBaseUnknown *iEventPublisher);


/** @nodoc */
ExportedByJS0SCBAK HRESULT CATSysRemoveAllRemainingCallbacks(CATBaseUnknown *iClient);

/** @nodoc **/
/* remove a unique callback set by a client without having to give the 
   event  publish 
*/
ExportedByJS0SCBAK void CATSysRemoveSubscriberCallback(
                                   CATBaseUnknown *iClient, CATCallback iId);

/**
 * @nodoc
 * Retrieves the list of events subscribed to the given event publisher.
 * @param iEventPublisher
 *   The event publisher
 * @param oEventsArray
 *   The retrieved events array 
 * 	 (this array is allocated by this method : it is up to the caller to delete it by a delete[] call)
 * @param oArraySize
 *   The size of the retrieved events array 
 */
ExportedByJS0SCBAK HRESULT CATSysGetSubscribedEvents(
			CATBaseUnknown * iEventPublisher,
			CATCallbackEvent*& oEventsArray, 
			int& oArraySize);

/**
 * @nodoc
 * Retrieves the list of subscribers on the given event publisher for the specified event.
 * @param iEventPublisher
 *   The event publisher
 * @param iEvent
 *   The specified event
 * @param oSubscribers
 *   The retrieved list of subscribers
 */
ExportedByJS0SCBAK HRESULT CATSysGetSubscribers(
			CATBaseUnknown * iEventPublisher,
			CATCallbackEvent iEvent, 
			CATListValCATBaseUnknown_var& oSubscribers);



#endif
