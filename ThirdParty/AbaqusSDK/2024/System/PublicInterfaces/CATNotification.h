#ifdef _WINDOWS_SOURCE
#pragma once
#endif
#ifndef CATNOTIFICATION_H
#define CATNOTIFICATION_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

 /*  Usage Notes: Event Container */


#include "string.h"
#include "CATBaseUnknown.h"
#include "CATEventSubscriberTypedefs.h"


#include "JS0SCBAK.h"

/**
 * Notification deletion request. 
 * <b>Role</b>: Parameter used in the @href CATNotification constructor to specify that
 * the notification must be deleted at the end of the next transaction.
 */
#define CATNotificationDeleteOn -1
/**
 * Notification keeping request. 
 * <b>Role</b>: Parameter used in the @href CATNotification constructor to specify that
 * the notification must not be deleted at the end of the next transaction.
 */
#define CATNotificationDeleteOff 0

/**
 * Base class for notifications.
 * <b>Role</b>: Notifications are objects that make other objects collaborate
 * by conveying information from an object to another.
 * They are intensively used by the Send/Receive communication protocol
 * that make @href CATCommand instance collaborate.
 */
class ExportedByJS0SCBAK CATNotification : public CATBaseUnknown
{

 public:

/**
 * Constructs a notification.
 * @param iAutomaticDelete
 *   Specifies whether the notification should be deleted at the end of the next transaction.
 *   <br><b>Legal values</b>: can be set to <tt>CATNotificationDeleteOn</tt> to delete
 *   the notification, or to <tt>CATNotificationDeleteOff</tt> to keep it.
 *   The notification is kept by default.
 */
    CATNotification(int iAutomaticDelete=CATNotificationDeleteOff);
    virtual ~CATNotification();
/**
 * returns the class name of the CATNotification instance.
 */
   const char *GetNotificationName();
/**
 * @nodoc
 * Requests the ClassName, IsA, and IsAKindOf methods to be created.
 */
    CATDeclareClass ;
/**
 * @nodoc
 * Reserved for the CATCleaner class instances. Do not use.
 */
    static void AutomaticDelete();

 private:

    static CATNotification* _Head;
    CATNotification* _Next;
};

#endif
