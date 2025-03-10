#ifndef __CATCommandSelector
#define __CATCommandSelector

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
  * @CAA2Level L1
  * @CAA2Usage U1
  */

#include "CATCommand.h"
class CATAppBaseList ;
class CATNotification ;

/**
  * @nodoc
  * CATCommandSelector mode for events and notifications.
  * @param CATCommandSelectorModeOFF
  *  The CATCommandSelector has a behavior of a CATCommand.
  * @param CATCommandSelectorModeON
  *  The CATCommandSelector has a behavior of a CATCommandSelector.
  * @param CATCommandSelectorModeABSORB
  *  The CATCommandSelector has a behavior of a CATCommandSelector
  * for the focus notification and does not propagate the other
  * notifications.
  */
enum CATCommandSelectorMode { CATCommandSelectorModeOFF    =0,
			      CATCommandSelectorModeON     =1,
			      CATCommandSelectorModeABSORB  =2 };

/**
  * Class to manage notifications.
  * <b>Role</b>: CATCommandSelector manage
  *  the scheduling of Focus between the CATCommands
  *  and dispatch the Notifications coming from the CATCommands
  */

class ExportedByJS0FM CATCommandSelector : public CATCommand
{

   public :

/**
  * @nodoc
  * Constructs a CATCommandSelector
  * @param iCmd
  *   The command's parents
  * @param iIdentifier
  *   The focus manager identifier
  */
      CATCommandSelector ( const CATString &iIdentifier,
			   CATCommand *iCmd=NULL );
/**
  * @nodoc
  * Destructor
  */
      ~CATCommandSelector ( ) ;

/**
  * @nodoc
  * Returns the root CATCommandSelector.
  * @return CATCommandSelector
  */
      static CATCommandSelector* GetCommandSelectorInstance();
/**
  * @nodoc
  * Returns the CATCommandSelector father of a CATCommand.
  * @param iCmd
  * @param iCheckMode
  * @return CATCommandSelector
  */      
      static CATCommandSelector* GetCommandSelectorInstance(
	                  CATCommand *iCmd,int iCheckMode=0 );

/**
  * @nodoc
  * Returns the default CATCommandSelector.
  * @return CATCommandSelector
  */   
      static CATCommandSelector* GetDefaultCommandSelectorInstance();
/**
  * @nodoc
  * Sets the default CATCommandSelector.
  * @param iCcs
  * @return error code.
  *    0 if OK
  *   -1 if iCcs is not a CATCommandSelector type
  */         
      static int SetDefaultCommandSelectorInstance( CATCommandSelector* iCcs);

/**
  * @nodoc
  * Sets the CATCommandSelector mode.
  * @param iNewMode
  *   CATCommandSelector mode
  * @return error code
  *    0 if OK
  *   -1 if iNewMode is invalid
  */   
      virtual int SetSelectorMode(CATCommandSelectorMode iNewMode );
      
/**
  * @nodoc
  * Returns the CATCommandSelector mode.
  * @return the mode
  */         
      virtual CATCommandSelectorMode GetSelectorMode();

/**
  * @nodoc
  * Event receiver.
  */
      virtual int SelectorReceiveNotification ( CATCommand* ,
					 CATNotification* EvtDat );
/**
  * @nodoc
  * Event receiver.
  */					 
      virtual void* SelectorSendObject( const char *ObjectClass,
				CATNotification* EvtDat );
/**
  * @nodoc
  * Returns the call status of the callbacks which indicates 
  * that more no command has the focus are disconnected.
  * @return the status
  *   connected    -1
  *   disconnected 0
  */
      int  GetSubscribeStatus( );
/**
  * @nodoc
  * Indicates that the call of the callbacks which indicates that 
  * more no command has the focus must be or not disconnected.
  * @param iNewValue
  *   connected    -1
  *   disconnected 0 
  */      
      void SetSubscribeStatus( int iNewValue );
/**
  * @nodoc
  * Indicate if a CATCommand has or not the focus.
  * @param iCmd
  *   The command 
  * @return the status
  *   not activated 0
  *   activated     <> 0
  */       
      int IsActivated (CATCommand* iCmd);

/**
  * @nodoc
  * Returns the CATCommand which has the focus.
  * @return the command.
  */     
      CATCommand* GetActivatedCommand();
/**
  * @nodoc
  * CATCommandSelector Event indicating that there is no more
  * CATCommand activated.
  */   
      CATDeclareCBEvent(NoMoreActivatedCATCommand);

/**
  * @nodoc
  * CATCommandSelector status.   
  */   
      int Dump( );

/**
  * @nodoc
  * Requests the ClassName, IsA, and IsAKindOf methods to be created.   
  */   
      CATDeclareClass;

   private :

    //=====================================================
    // Data
    //=====================================================

      // CommandSelector Status
      // ----------------------
      int FM_Status;
      int _SubscribeAuthorized;

      // Information about the CATCommand who has the focus
      // --------------------------------------------------
      int         CMDF_Focus_Status;       // CATCommand start state
      CATCommand *CMDF_Focus_CATCommand;   // CATCommand

      // Stack of Active Notification
      // -----------------------
      CATAppBaseList *FM_Liste_Acc;

      // Stack of Differs Notification
      // -----------------------------
      CATAppBaseList *FM_Liste_Dif;

      // Active Stack processing or Differ Stack processing
      // --------------------------------------------------
      int FMSourceEventType ;


      int EventType_temp;
      CATCommand *FromClient_temp;
      CATNotification* EvtDat_temp;

      // CATCommandSelector Mode
      // -----------------------
      CATCommandSelectorMode _SelectorMode;

      // Default CATCommandSelector
      // ---------------------------
      static CATCommandSelector *_DefaultCommandSelector ;


    //=====================================================
    // Methods
    //=====================================================

      void InternalConstructor( CATCommand *, CATString* const );

      // Internal Receive
      // -----------------
      int ReceiveInternal ( int EventType, CATCommand* FromClient,
			CATNotification* );

      // Internal RequestSharedMode (GrabFocus Shared) processing
      // --------------------------------------------------------
      void FM_GFS_READY
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_GFS_TGFE
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_GFS_TGFS
	       (int EventType,CATCommand* FromClient,
		CATNotification* );

      // Internal RequestExclusiveMode (GrabFocus Exclusive) processing
      // --------------------------------------------------------------
      void FM_GFE_READY
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_GFE_TGFE
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_GFE_TGFS
	       (int EventType,CATCommand* FromClient,
		CATNotification* );

      // Internal Canceled (UnGrabFocus) processing
      // ----------------------------------------------
      void FM_UF_READY
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_UF_TGFE
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_UF_TGFS
	       (int EventType,CATCommand* FromClient,
		CATNotification* );

      // Internal Desactivated (FocusRelease) processing
      // -----------------------------------------------
      void FM_FR_READY
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_FR_TGFE
	       (int EventType,CATCommand* FromClient,
		CATNotification* );
      void FM_FR_TGFS
	       (int EventType,CATCommand* FromClient,
		CATNotification* );


      // Put Differ Notification in a Differ Stack
      // -----------------------------------------
      int LstDifPut
       (int EventType,CATCommand* FromClient,
	      CATNotification* EvtDat );

      // Get Differ Notification
      // -----------------------
      int LstDifGet
       (int &EventType,CATCommand** FromClient,
	      CATNotification** EvtDat );

      // Start the processing of Differs Notification
      // ---------------------------------------------
      int LstDifRun ( );


      // Send to CATCommand
      // ------------------
      void FMSendEvent ( int, CATCommand* ,
			 CATCommand*, CATNotification* );

      // Send to CATCommand
      // -------------------
      int FMSend ( int, CATCommand* , CATCommand*,
		   CATNotification*);

      // Send to CATCommand Focus
      // ------------------------
      void FMSendToFocus ( CATCommand*, CATNotification* );


};


#endif
