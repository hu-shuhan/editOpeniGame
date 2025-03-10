#ifndef CATCOMMAND_H
#define CATCOMMAND_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "JS0FM.h"
#include "CATEventSubscriber.h"
#include "CATString.h"
#include "CATNotification.h"
#include "CATCallbackManager.h"
#include "CATMacForIUnknown.h"

class CATCommand;
class CATMarshal;
class  CATCommandGlobalUndo;

// Arguments of CATCommand Constructor when you start the CATCommand
// -----------------------------------------------------------------

/**
 * Command start mode.
 * @param CATCommandModeShared
 *   The shared mode. The command is known by the command selector.
 *   As soon as the command is selected, the command selector
 *   deactivates the previous active command, that is, withdraws
 *   the focus and puts it into the command stack from where it can be
 *   reactivated later on,
 *   and gives the focus to the selected command to make
 *   it the active one.
 * @param CATCommandModeExclusive
 *   The exclusive mode. The command is known by the command selector.
 *   As soon as the command is selected, the command selector
 *   deletes the previous active command
 *   and gives the focus to the selected command to make
 *   it the active one.
 * @param CATCommandModeUndefined
 *   The undefined mode. The command is ignored by the command selector.
 *   It cannot be deleted or deactivated if another command is selected
 *   while it runs, and it runs until it completes.
 */
enum CATCommandMode{CATCommandModeShared    =1,
		    CATCommandModeExclusive =0,
		    CATCommandModeUndefined =3};

/**
 * Message type sent from a command to the command selector.
 * @param CATCommandMsgRequestSharedMode
 *   The command requests to start in shared mode.
 * @param CATCommandMsgRequestExclusiveMode
 *   The command requests to start in exclusive mode.
 * @param CATCommandMsgCanceled
 *   The command requests to be canceled.
 * @param CATCommandMsgDesactivated
 *   The command requests to be deactivated.
 * @param CATCommandUnknow
 *   The command requests the unknown mode.
 * @see CATCommandMode
 */
enum CATCommandMsg{CATCommandMsgRequestSharedMode    =0,
		   CATCommandMsgRequestExclusiveMode =1,
		   CATCommandMsgCanceled             =2,
		   CATCommandMsgDesactivated         =3,
		   CATCommandUnknow                  =100};

/**
 * Message type received by a command from the command selector.
 * @param CATCommandSelectorMsgActivate
 *   The command selector activates the command
 * @param CATCommandSelectorMsgDesactivate
 *   The command selector deactivates the command.
 * @param CATCommandSelectorMsgCancel
 *   The command selector cancels the command
 * @param CATCommandSelectorUnknow
 *   The command selector sets the command mode to unknown
 */
enum CATCommandSelectorMsg{CATCommandSelectorMsgActivate     =4,
			   CATCommandSelectorMsgDesactivate  =5,
			   CATCommandSelectorMsgCancel       =6,
			   CATCommandSelectorUnknow          =100};

/**
 * Return code to accept a notification and stop its propagation.
 * <b>Role</b>: This return code is used by the @href CATCommand#AnalyseNotification method.
 */
#define  CATNotifAccepted CATNotifDontTransmitToFather

/**
 * Return code to refuse a notification and continues its propagation.
 * <b>Role</b>: This return code is used by the @href CATCommand#AnalyseNotification method.
 */
#define  CATNotifRefused CATNotifTransmitToFather

/**
 * Notification propagation mode.
 * <b>Role</b>: It is returned by the @href CATCommand#AnalyseNotification method.
 * @param CATNotifTransmitToFather
 *   The notification cannot be processed by the command that has analyzed it.
 *   This command requests the sending command to resend the notification to its parent
 *   command
 * @param CATNotifDontTransmitToFather
 *   The notification can be processed by the command that has analyzed it.
 *   The sending command should not resend the notification
 */
enum CATNotifPropagationMode{CATNotifTransmitToFather     =0,
			     CATNotifDontTransmitToFather =1};

/**
 * Command return status.
 * <b>Role</b>: It is returned by @href CATCommand#Activate, @href CATCommand#Desactivate,
 * and @href CATCommand#Cancel methods.
 * @param CATStatusChangeRCCompleted
 *   The command completes normally
 * @param CATStatusChangeRCAborted
 *   The command aborts
 */
enum CATStatusChangeRC{CATStatusChangeRCCompleted =0,
		       CATStatusChangeRCAborted   =1};

/**
 * Type for data passed as an argument of a callback method. 
 */
typedef void * CATCommandClientData;

/**
 * Prototype of the callback method.
 * @param iCommand 
 *   The calling command.
 * @param iNotification
 *   The notification declared to set the callback.
 * @param iUsefulData
 *   Data to pass as parameter to the method that can be useful.
 */
typedef void (CATBaseUnknown::*CATCommandMethod)(CATCommand * iCommand,
                                           CATNotification * iNotification,
                                        CATCommandClientData iUsefulData);

/**
 * Base class for all objects that need to collaborate through notifications.
 * <b>Role</b>: To provide the basic command behavior, that is, to
 * send notifications, that usually depicts a user interaction, to other commands
 * and to receive notifications sent by other commands.
 * This behavior is known as the send/receive communication protocol.
 * <p>
 * All the existing commands are organized as a tree structure.
 * Each command has a parent command and possibly children.
 * The parent is passed as a parameter of the command constructor.
 * <p>
 * When the end-user interacts with the application, system events are produced
 * that CNext turns into notifications, and sends them to either the command
 * that is involved, such as the push button pushed by the end user,
 * or to the command that is the nearest to the 
 * user interaction, such as the manipulator set onto a document object's representation
 * selected by the end user in a graphics viewer.
 * <p>
 * The notification is sent from command to command along the tree structure
 * until it can be processed.
 * If no command able to process the notification is
 * found, it is sent to the command selector that resends it to
 * the active command.
 * This can decide to process the notification,
 * or otherwise to do something else, possibly nothing.
 * @see CATEventSubscriber, CATCallbackManager, CATNotification
 */
class ExportedByJS0FM CATCommand : public CATEventSubscriber
{
   public :

//Constructors

/**
 * Constructs a command with a given parent and identifier.
 * <br><b>Role</b>: The command is constructed using its parent command
 * in the command tree structure, that is the command to which the notifications
 * that the constructed command cannot process are sent, and using its identifier.
 * @param iParent
 *   The command's parent
 * @param iIdentifier
 *   The command's identifier, as a pointer to a constant pointer to CATString
 */
      CATCommand(CATCommand * iParent=NULL,
		 CATString * const iIdentifier=NULL);

/**
 * @nodoc
 * Constructs a command with a given parent and start mode.
 * <br><b>Role</b>: The command is constructed using its parent command
 * in the command tree structure, that is the command to which the notifications
 * that the constructed command cannot process are sent, and using its start mode.
 * @param iParent
 *   The command's parent
 * @param iStartMode
 *   The command start mode
 */
      CATCommand(CATCommand * iParent,
                 CATCommandMode iStartMode);

/**
 * Constructs a command with a given parent and identifier.
 * <br><b>Role</b>: The command is constructed using its parent command
 * in the command tree structure, that is the command to which the notifications
 * that the constructed command cannot process are sent, and using its identifier.
 * @param iParent
 *   The command's parent
 * @param iIdentifier
 *   The command's identifier, as a constant CATString
 */
      CATCommand(CATCommand * iParent,
		 const CATString & iIdentifier);

/**
 * @nodoc
 * Constructs a command with a given parent.
 * @param iParent
 *   The command's parent
 */
      CATCommand(CATCommand & iParent);

/**
 * @nodoc
 * Constructs a command with a given identifier and start mode.
 * @param iIdentifier
 *   The command's identifier, as a constant CATString
 * @param iStartMode
 *   The command start mode
 */
      CATCommand(const CATString & iIdentifier,
                 CATCommandMode iStartMode);

//Destructor
      virtual ~CATCommand () ;

      // Send a notification to an other CATCommand
      // -----------------------------------------------------

/**
 * Sends a notification to another command.
 * <br><b>Role</b>: This is a key method in the Send/receive communication
 * protocol which can be used to send to the parent command either
 * a notification received as is, or another notification that replaces and
 * enriches the received notification. A command different than the
 * parent command can be chosen if required. 
 * @param iToClient
 *   The command to which the notification is to be sent
 * @param iNotification
 *   The notification to send
 */
      void SendNotification(CATCommand * iToClient,
			    CATNotification * iNotification);

      // Post a notification to an other CATCommand
      // ( Internal used only )
      // -----------------------------------------------------
/**
 * @nodoc
 * Posts a notification to another CATCommand instance.
 * @param iToClient
 *   The CATCommand instance to which the notification is to be sent.
 * @param iEvtDat
 *   The notification to send.
 */
      void PostNotification(CATCommand * iToClient,
			    CATNotification * iEvtDat);

      // Reception of a notification (not virtual)
      // ---------------------------
/**
 * @nodoc
 * Receives a notification from another CATCommand instance.
 * @param iFromClient
 *   The CATCommand instance from which the 
 *   notification is to be  received.
 * @param iEvtDat
 *   The notification to receive.
 */
      void ReceiveNotification(CATCommand* FromClient,
			       CATNotification* EvtDat);

      // Event interpretation (not virtual)
      // --------------------
/**
 * Requests an object to be retrieved.
 * @param iObjectClassNeeded
 *   The class of which the requested object should be an instance.
 * @param iNotification
 *   The notification received before the request.
 * @return
 *   A pointer to the retrieved object
 */
      void * SendObject(const char *      iObjectClassNeeded,
                        CATNotification * iNotification);

      // Change The Status of The CATCommand
      // -----------------------------------
/**
 * Requests a command status change.
 * @param iMessageType
 *   The message type to send to the command selector
 * @param iCommandSelector
 *   The command selector
 * @return
 *   0 if the command status changes successfully
 */
      int RequestStatusChange(CATCommandMsg iMessageType, 
			      CATCommand * iCommandSelector = NULL);

      // Delayed Destruction 
      // -------------------
/**
 * Requests the command delayed destruction.
 * <br><b>Role</b>: The command will ve destroyed as soon as this will be possible.
 */
      virtual void RequestDelayedDestruction();

/**
 * @nodoc
 */
      virtual HRESULT LogicalDeath();

      // Manage the default receiver of my Notification
      // -----------------------------------
/**
 * Returns the command parent.
 * <br><b>Role</b>: The command parent is usually set by the constructor.
 * The notifications sent to the command are resent to its parent if the 
 * command cannot process them.
 */
      CATCommand * GetFather();
/**
 * Sets the command parent.
 * <br><b>Role</b>: The command parent is usually set by the constructor.
 * The notifications sent to the command are resent to its parent if the 
 * command cannot process them.
 * @param iParent
 *   The command to set as the parent of the current command
 *   in the command tree structure
 */
      void SetFather(CATCommand * iParent);

      // Set/Get the CATCommand name
      // ---------------------------
/**
 * Returns the command identifier.
 */
      virtual CATString & GetName();
/**
 * @nodoc
 * Sets the command identifier.
 * @param iIdentifier
 *   The identifier to set to the command
 */
      virtual void  SetName(CATString & iIdentifier);

      // Get the CallbackManager
      // ----------------------
/**
 * @nodoc
 * Returns the command callback manager.
 * <br><b>Role</b>: The callback manager is the object known by the command (referenced or
 * aggregated) that dispatches the events declared by the command whenever
 * they happen to the subscribers to these events by means of the method
 * @href CATCallbackManager#DispatchCallbacks
 * <p>
 * Overrides @href CATCallbackManager#GetCallbackManager in class
 * CATCallbackManager
 */
      virtual CATCallbackManager * GetCallbackManager();

      // Event dispath by CallbackManager
      // --------------------------------
/**
 * @nodoc
 * Defines a Delete event.
 */
      CATDeclareCBEvent(Delete);
/**
 * @nodoc
 * Defines an Activate event.
 */
      CATDeclareCBEvent(Activate);
/**
 * @nodoc
 * Defines a Deactivate event.
 */
      CATDeclareCBEvent(Desactivate);
/**
 * @nodoc
 * Defines a Cancel event.
 */
      CATDeclareCBEvent(Cancel);

      // CATCommand start mode
      // ----------------------
/**
 * Returns the command start mode.
 */
      CATCommandMode GetStartMode();

      // Reception of a notification
      // ---------------------------
/**
 * Analyzes a notification sent by another command.
 * @param FromClient
 *   The command that sends the notification to be analyzed
 * @param iNotification
 *   The notification to analyze
 * @return
 *   The notification propagation mode
 */
      virtual CATNotifPropagationMode AnalyseNotification(
				       CATCommand * iFromClient,
				       CATNotification * iNotification);

      // Reception of Focus In from CATCommandSelector protocol
      // ----------------------------------------------------
/**
 * Activates a command.
 * <br><b>Role</b>: Called by the command selector to give the focus
 * to the command.
 * @param iFromClient
 *   The command that requests to activate the current one
 * @param iNotification
 *   The notification sent
 * @return The command status.
 */
      virtual CATStatusChangeRC Activate(
			    CATCommand * iFromClient,
			    CATNotification * iNotification);

      // Reception of Focus Out from CATCommandSelector protocol
      // ----------------------------------------------------
/**
 * Deactivates a command.
 * <br><b>Role</b>: Called by the command selector to temporarily
 * withdraw the focus from the command.
 * @param iFromClient
 *   The command that requests to deactivate the current one
 * @param iNotification
 *   The notification sent
 * @return The command status.
 */
      virtual CATStatusChangeRC Desactivate(
			    CATCommand * iFromClient,
			    CATNotification * iNotification);

      // Reception of Cancel from CATCommandSelector protocol
      // ----------------------------------------------------
/**
 * Cancels a command.
 * <br><b>Role</b>: Called by the command selector to definitely
 * withdraw the focus from the command.
 * The command should then request its destruction.
 * @param iFromClient
 *   The command that requests to cancel the current one
 * @param iNotification
 *   The notification sent
 * @return The command status.
 */
      virtual CATStatusChangeRC Cancel(
			    CATCommand * iFromClient,
			    CATNotification * iNotification);

      // For surcharge purposes
      // ----------------------------------------------------
/**
 * Requests an object to be retrieved.
 * <br><b>Role</b>: This method should be redefined by derived classes.
 * @param iObjectClassNeeded
 *   The class of which the requested object should be an instance
 * @param iNotification
 *   The notification received before the request
 * @return
 *   A pointer to the retrieved object
 */
      virtual void * SendCommandSpecificObject(
		       const char      * iObjectClassNeeded,
                       CATNotification * iNotification);


   protected:

      // Add a Callback for notification
      //--------------------------------------------------
/**
 * Adds a callback for a given notification published by a given command.
 * @param iPublishingCommand
 *   The command that publishes the notification.
 * @param iPublishedNotification
 *   The notification published by <tt>iPublishingCATCommand</tt>
 * @param iMethodToExecute
 *   The method to execute whenever <tt>iPublishingCATCommand</tt>
 *   publishes <tt>iPublishedNotification</tt>
 * @param iUsefulData
 *   Data to pass to <tt>iMethodToExecute</tt> and that can be useful
 *   to this method
 * @return
 *   The added callback
 */
      CATCallback AddAnalyseNotificationCB(
                            CATCommand * iPublishingCommand,
	                    CATNotification * iPublishedNotification,
                       	    CATCommandMethod iMethodToExecute,
	                    CATCommandClientData iUsefulData);
/**
 * Adds a callback for a given notification published by a given command.
 * @param iPublishingCommand
 *   The command that publishes the notification
 * @param iNotificationClassName
 *   The class name of the notification sent by <tt>iPublishingCATCommand</tt>.
 *   <br><b>Legal values</b>: Use the <tt>ClassName</tt> static method of
 *   the notification class.
 *   <br>Example: <tt>MyNotification::ClassName()</tt>, where MyNotification
 *   is the notification class from which an instance is expected 
 * @param iMethodToExecute
 *   The method to execute whenever <tt>iPublishingCATCommand</tt> publishes
 *   <tt>iNotificationClassName</tt>
 * @param iUsefulData
 *   Data to pass to <tt>iMethodToExecute</tt> and that can be useful
 *   to this method
 * @return
 *   The added callback
 */
      CATCallback AddAnalyseNotificationCB(
                            CATCommand * iPublishingCommand,
	                    const char * iNotificationClassName,
                       	    CATCommandMethod iMethodToExecute,
	                    CATCommandClientData iUsefulData);
      // Remove a Callback
      //------------------
/**
 * Removes a callback for a given notification published by a given command.
 * @param iPublishingCommand
 *   The command that publishes the notification
 * @param iPublishedNotification
 *   The notification published by <tt>iPublishingCATCommand</tt>
 * @param iUsefulData
 *   Data to pass to <tt>iMethodToExecute</tt> and that can be useful
 * to this method
 */
      void RemoveAnalyseNotificationCB(
                        CATCommand * iPublishingCommand,
	                CATNotification * iPublishedNotification,
	                CATCommandClientData iUsefulData);

/**
 * Removes a callback for a given notification published by a given command.
 * @param iPublishingCommand
 *   The command that publishes the notification
 * @param iNotificationClassName
 *   The class name of the notification sent by <tt>iPublishingCATCommand</tt>.
 *   <br><b>Legal values</b>: Use the <tt>ClassName</tt> static method of
 *   the notification class.
 *   <br>Example: <tt>MyNotification::ClassName()</tt>, where MyNotification
 *   is the notification class from which an instance is expected 
 * @param iUsefulData
 *   Data to pass to <tt>iMethodToExecute</tt> and that can be useful
 *   to this method
 */
      void RemoveAnalyseNotificationCB(
                        CATCommand * iPublishingCommand,
	                const char * iNotificationClassName,
	                CATCommandClientData iUsefulData);

/**
 * Removes a callback for a given notification published by a given command.
 * @param iPublishingCommand
 *   The command that publishes the notification
 * @param iPublishedNotification
 *   The notification published by <tt>iPublishingCATCommand</tt>
 */
      void RemoveAnalyseNotificationCB(
                        CATCommand * iPublishingCommand,
		        CATNotification * iPublishedNotification);

/**
 * Removes a callback for a given notification published by a given command.
 * @param iPublishingCommand
 *   The command that publishes the notification
 * @param iNotificationClassName
 *   The class name of the notification sent by <tt>iPublishingCATCommand</tt>.
 *   <br><b>Legal values</b>: Use the <tt>ClassName</tt> static method of
 *   the notification class.
 *   <br>Example: <tt>MyNotification::ClassName()</tt>, where MyNotification
 *   is the notification class from which an instance is expected 
 */
      void RemoveAnalyseNotificationCB(
                        CATCommand * iPublishingCommand,
		        const char * iNotificationClassName);

/**
 * Removes a callback.
 * @param iCallbackToRemove
 *   The callback to remove
 */
      void RemoveAnalyseNotificationCB(
                        CATCallback iCallbackToRemove);

   public:
   
      // Controler Managment
      // -------------------

/**
 * @nodoc
 * Get the controler of the command.
 * @return the controller.
 */       
      
      void * GetControler();
/**
 * @nodoc
 * Set associated controler.
 * @param iControler
 *   The Controler of the command . 
 */      
      void SetControler ( void * iControler );

      // Calling CATCommand
      // -------------------
/**
 * @nodoc
 * Set the calling command.
 * @param iCallingCommand
 *   The calling command. 
 */
      void SetCalling(CATCommand * iCallingCommand);
/**
 * @nodoc
 * Returns the calling command.
 */
      CATCommand * GetCalling();

      // Prompt Management
      // ------------------
/**
 * @nodoc
 * Sets the prompt associated with the command.
 * This method applies to interactive commands only.
 * @param iTextToPrompt
 *   The text to prompt with the command.
 */
      virtual void SetPrompt(CATString iTextToPrompt);
/**
 * @nodoc
 * Returns the text prompted with the command.
 */
      virtual CATString GetPrompt();
/**
 * @nodoc
 * Returns the status of text prompted with the command, that is
 * whether the prompted text can be displayed.
 */
      virtual int GetStatusPrompt();

/**
 * @nodoc
 * Begins the command.
 * This method should be redefined.
 * It is executed automatically after the command constuctor and is
 * generally used to instantiate the command's data member.
 */
      virtual void BeginCommand();

/**
 * @nodoc
 * Ends the command.
 * This method should be redefined.
 * It is executed automatically before the command destructor and is
 * generally used to deallocate or delete command resources.
 */
      virtual void EndCommand();

/**
 * @nodoc
 * Resets the command.
 * This method should be redefined.
 */
      virtual void Reset();

/**
 * @nodoc
 * Undoes the command.
 * This method should be redefined.
 */
      virtual void UndoCommand();

/**
 * @nodoc
 * Returns the command global undo.
 */
      virtual CATCommandGlobalUndo * GetGlobalUndo();

      // CATCommand Data Management
      // --------------------------
/**
 * @nodoc
 * Saves the notification and its associated data for the command to record
 * in Record capture mode.
 * This method should be redefined for each recordable command.
 * @param iNotification 
 *   The notification sent.
 * @param iDataToSave
 *   The associated data to save.
 * @return
 *   CATRecordSave (=1) to request saving <tt>iNotification</tt>
 *   and <tt>iDataToSave</tt>,
 *   and CATRecordNoSave (=2) otherwise.
 * @see CATNotificationRecord#CATCommandSaveState
 */
      virtual int SaveState(CATNotification * iNotification,
                            CATMarshal      & iDataToSave);
/**
 * @nodoc
 * Restores the notification and its associated data for the command to record
 * in Record replay mode.
 * This method should be redefined for each recordable command.
 * @param iNotification
 *   The notification sent.
 * @param oDataToRestore
 *   The associated data to restore.
 * @return
 *   Unused
 */
      virtual int RestoreState(CATNotification * iNotification,
                               CATMarshal      & oDataToRestore);

/**
 * @nodoc
 * Returns 1 if the Record is replayed and 0 otherwise.
 */
      int IsReplay();
/**
 * @nodoc
 * Returns 1 if the Record is captured and 0 otherwise.
 */
      int IsCapture();

/**
 * @nodoc
 * Requests the ClassName, IsA, and IsAKindOf methods to be created. 
 */
      CATDeclareClass;

  /**
   * @nodoc
   * Get access to command data 
   */
  void *AccessCommandData();
  private :
    void *_CommandData;
};

#endif
