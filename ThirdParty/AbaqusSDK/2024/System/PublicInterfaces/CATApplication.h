#ifndef CATAPPLICATION_H
#define CATAPPLICATION_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "CATCommand.h"

// Type of Subscribe method
// -----------------------------------------------------------------
/**
 * Command subscribing type.
 * <b>Role</b>: The subscribing type , used as parameter of the methods Subscribe and
 * RemoveSubscribe.
 * @param CATSubscribeEndTransaction
 *   The subscribing command must be called when the transaction is completed
 * @param CATSubscribeIdle
 *   The subscribing command must be called when the process enters the main event loop 
 * @see CATApplication#Subscribe , CATApplication#RemoveSubscribe 
 */
enum { CATSubscribeEndTransaction=1, CATSubscribeIdle =2 };
/** @nodoc */
#define CATSubsribeIdle CATSubscribeIdle

class CATNotifier;
class CATMarshal;
 

/**
 * Prototype of the function or static method passed as callback in the
 * the @href CATApplication#AddTimeOut method.
 * <b>Role</b>: This callback static method is declared by means of
 * the @href CATApplication#AddTimeOut method.
 * @param iCmd
 *   A pointer to the command associated to the subscription time to 
 * the @href CATApplication#AddTimeOut method.
 * @param iTime
 *   The time value passe to the @href CATApplication#AddTimeOut method.
 * @param iClientData  
 *   Useful data to pass as argument at the subscription time to
 * the @href CATApplication#AddTimeOut method.
 * @see CATApplication#AddTimeOut , CATApplication#RemoveTimeOut
 */
typedef void (*CATSysTimeOutCB)( CATCommand *iCmd , int iTime , void *iClientData );


/**
 * Prototype of the function or static method passed as callback in the
 * the @href CATApplication#AddFileDescriptor method.
 * <b>Role</b>: This callback static method is declared by means of
 * the @href CATApplication#AddFileDescriptor method.
 * @param iCmd
 *   A pointer to the command associated to the subscription time to 
 * the @href CATApplication#AddFileDescriptor method.
 * @param iTime
 *   The time value passe to the @href CATApplication#AddFileDescriptor method.
 * @param iClientData  
 *   Useful data to pass as argument at the subscription time to
 * the @href CATApplication#AddFileDescriptor method.
 * @see CATApplication#AddFileDescriptor , CATApplication#RemoveFileDescriptor
 */
typedef void (*CATSysFileDescCB)( CATCommand *iCmd , int iFileDescriptor , void *iClientData );


/**
 * Prototype of the function or static method passed as callback in the
 * the @href CATApplication#Subscribe method.
 * <b>Role</b>: This callback static method is declared by means of
 * the @href CATApplication#Subscribe method.
 * @param iCmd
 *   A pointer to the command associated to the subscription time to 
 * the @href CATApplication#Subscribe method.
 * @param iSubscribingType
 *   The subscribing type.
 *   <br><b>Legal values</b>: Can be set to <tt>CATSubscribeEndTransaction</tt>
 *   (=1) or to <tt>CATSubscribeIdle</tt> (=2)
 * @param iClientData  
 *   Useful data to pass as argument at the subscription time to
 * the @href CATApplication#Subscribe method.
 * @see CATApplication#Subscribe , CATApplication#RemoveSubscribe
 */
typedef void (*CATSysSubscribeCB)( CATCommand *iCmd , int iSubscriptionType , void *iClientData );


/**
 * Base class for applications.
 * <b>Role</b>:
 * An application is the main object in a process and includes
 * related objects, such as windows, as data members.
 */
class ExportedByJS0FM CATApplication : public CATCommand
{
   CATDeclareClass;
   public :

/**
 * Constructs an application. 
 * @param iParent
 *   The parent command of the created application.
 *   <br><b>Legal values</b>:
 *   Set it to NULL, except if you have several application instances in
 *   the same process.
 * @param iMode
 *   A flag to indicate whether the application constructor supplies the main function
 *   <br><b>Legal values</b>:
 *   (<tt>iMode</tt>=0) or if the application is instantiated from an
 *   existing main function.
 */
      CATApplication(CATCommand * iParent,
                     void *       iMode=0);
      virtual ~CATApplication ( ) ;

      // Time out  Subscription
      //-----------------------
/**
 * Adds a timeout subscription to a command.
 * <br><b>Role</b>:
 * Recalls the command on a given static method when the given time is elapsed. 
 * @param iTime
 *   The time, expressed in milliseconds, to leave elaspe from the moment <tt>AddTimeOut</tt>
 *   is called, to the moment <tt>UserStaticMethod</tt> is called
 * @param iToCLient 
 *   The command to call using its <tt>UserStaticMethod</tt> method 
 * @param iClientData 
 *   Data to pass to <tt>UserStaticMethod</tt>
 * @param UserStaticMethod 
 *   The method to call when <tt>iTime</tt> is elapsed
 * @return 0 if the timeout subscription is successfully added and 1 otherwise.
 */
      int AddTimeOut(int          iTime,
                     CATCommand * iToClient,
                     void       * iClientData,
                     void (* UserStaticMethod)());



/**
 * Adds a timeout subscription to a command.
 * <br><b>Role</b>:
 * Recalls the command on a given static method when the given time is elapsed.
 * @param iTime
 *   The time, expressed in milliseconds, to leave elaspe from the moment <tt>AddTimeOut</tt>
 *   is called, to the moment <tt>iUserStaticMethod</tt> is called
 * @param iToCLient
 *   The command to call using its <tt>iUserStaticMethod</tt> method
 * @param iClientData
 *   Data to pass to <tt>iUserStaticMethod</tt>
 * @param iUserStaticMethod
 *   The static method or function to call when <tt>iTime</tt> is elapsed.
 * @param oTimeOutCBId
 *   <tt>oTimeOutCBId</tt> must be either NULL or point to the address of valid int variable
 *   If <tt>oTimeOutCBId</tt> is not NULL , at the successul  completion of the <tt>AddTimeOut</tt>
 *   method if will be set to a non negative value representing the id of the callback.
 * @return 0 if the timeout subscription is successfully added and 1 otherwise.
 */
      int AddTimeOut(int          iTime,
                     CATCommand * iToClient,
                     void       * iClientData,
                     CATSysTimeOutCB iUserStaticMethod,
                     int *oTimeOutCBId =NULL);

      // Remove Time out
      //----------------
/**
 * Removes a timeout subscription from a command.
 * <br><b>Role</b>:
 * To remove the subscription for a given command instance and a given time. 
 * @param iTime 
 *   The elapsed time
 * @param iToClient
 *   The command for which the subscription is to be removed
 * @return 0 if the timeout subscription is successfully removed and 1 otherwise.
 * @see #AddTimeOut
 */
      int RemoveTimeOut(int          iTime,
                        CATCommand * iToClient);
/**
 * Removes a timeout subscription from a command.
 * <br><b>Role</b>:
 * To remove the subscription for a given command instance and a given time. 
 * @param iTime 
 *   The elapsed time
 * @param iTimeOutCBId
 *   The id of the subscription  to be removed ( returned through the parameter 
 *   oTimeOutCBId of the  @href CATApplication#AddTimeOut method)
 * @param iReserved
 *   This parameter must me set to zero
 * @return 0 if the timeout subscription is successfully removed and 1 otherwise.
 * @see #AddTimeOut
 */
      int RemoveTimeOut(int          iTime,
                        int          iTimeOutCBId,
                        int iReserved=0);

      // File Descriptor Subscription
      //------------------------------
/**
 * Adds a file descriptor subscription to a command.
 * <br><b>Role</b>:
 * This recalls the command on a given static method when the given file
 * descriptor is modified. This applies generally to any entity homogeneous
 * to a file, such as an input flow, a pipe, a socket, but not to disk files.
 * The term file descriptor applies to UNIX and corresponds to file handle
 * with Windows.
 * @param iFileDescriptor
 *   The file descriptor whose modification requests to call the method
 *   <tt>UserStaticMethod</tt>
 * @param iToClient
 *   The command to call with its <tt>UserStaticMethod</tt> 
 * @param iClientData
 *   Data useful to <tt>UserStaticMethod</tt>
 * @param UserStaticMethod
 *   The method to call when <tt>iFileDescriptor</tt> is modified.
 * @return 0 if the file descriptor subscription is successfully added and 0 otherwise.
 */
      int AddFileDescriptor(int          iFileDescriptor,
                            CATCommand * iToClient,
                            void       * iClientData,
                            void (* UserStaticMethod)()
                           );

/**
 * Adds a file descriptor subscription to a command.
 * <br><b>Role</b>:
 * This recalls the command on a given static method when the given file
 * descriptor is modified. This applies generally to any entity homogeneous
 * to a file, such as an input flow, a pipe, a socket, but not to disk files.
 * The term file descriptor applies to UNIX and corresponds to file handle
 * with Windows.
 * @param iFileDescriptor
 *   The file descriptor whose modification requests to call the method
 *   <tt>iUserStaticMethod</tt>
 * @param iToClient
 *   The command to call with its <tt>iUserStaticMethod</tt> 
 * @param iClientData
 *   Data useful to <tt>iUserStaticMethod</tt>
 * @param iUserStaticMethod
 *   The method to call when <tt>iFileDescriptor</tt> is modified.
 * @param oFileDescCBId
 *   <tt>oFileDescCBId</tt> must be either NULL or point to the address of valid int variable
 *   If <tt>oFileDescCBId</tt> is not NULL , at the successul  completion of the <tt>AddFileDescriptor</tt>
 *   method  <tt>oFileDescCBId</tt>  will be set to a non negative value representing the id of the callback.
 * @return 0 if the file descriptor subscription is successfully added and 0 otherwise.
 */
      int AddFileDescriptor(int          iFileDescriptor,
                            CATCommand * iToClient,
                            void       * iClientData,
                            CATSysFileDescCB  iUserStaticMethod,
                            int          *oFileDescCBId=NULL);

      // Remove File Descriptor
      //-----------------------
/**
 * Removes a file descriptor subscription from a command.
 * @param iFileDescriptor
 *   The file descriptor.
 * @param iToClient
 *   The command for which the subscription is to be removed.
 * @return 0 if the file descriptor subscription is successfully removed and 0 otherwise.
 * @see #AddFileDescriptor
 */
      int RemoveFileDescriptor(int          iFileDescriptor,
                               CATCommand * iToClient);


/**
 * Removes a file descriptor subscription from a command.
 * @param iFileDescriptor
 *   The file descriptor.
 * @param iFileDescCBId
 *   The id of the subscription  to be removed ( returned through the parameter 
 *   oFileDescCBId of the  @href CATApplication#AddFileDescriptor method)
 * @param iReserved
 *   This parameter must me set to zero
 * @return 0 if the file descriptor subscription is successfully removed and 0 o
therwise.
 * @see #AddFileDescriptor
 */
      int RemoveFileDescriptor(int          iFileDescriptor,
                               int          iFileDescCBId,
                               int          iReserved=0);

      // Subscribe on Idle / End Transaction
      //------------------------------------
/**
 * Adds a transaction/idle subscription to a command. 
 * @param iSubscribingType 
 *   The subscribing type.
 *   <br><b>Legal values</b>: Can be set to <tt>CATSubscribeEndTransaction</tt>
 *   (=1) or to <tt>CATSubscribeIdle</tt> (=2) 
 * @param iToClient 
 *   The command to call with its <tt>UserStaticMethod</tt> 
 * @param iClientData
 *   Data useful to <tt>UserStaticMethod</tt>
 * @param UserStaticMethod 
 *   The method to call when the state defined by
 *   <tt>iSubscribingType</tt> is reached.
 * @return 0 if the transaction/idle subscription is successfully added and 0 otherwise.
 */
      int Subscribe(int          iSubscribingType,
                    CATCommand * iToClient,
		    void       * iClientData,
                    void (* UserStaticMethod)());

/**
 * Adds a transaction/idle subscription to a command.
 * @param iSubscribingType
 *   The subscribing type.
 *   <br><b>Legal values</b>: Can be set to <tt>CATSubscribeEndTransaction</tt>
 *   (=1) or to <tt>CATSubscribeIdle</tt> (=2)
 * @param iToClient
 *   The command to call with its <tt>iUserStaticMethod</tt>
 * @param iClientData
 *   Data useful to <tt>iUserStaticMethod</tt>
 * @param iUserStaticMethod
 *   The method to call when the state defined by
 *   <tt>iSubscribingType</tt> is reached.
 * @param oSubscribeCBId
 *   <tt>oSubscribeCBId</tt> must be either NULL or point to the address of valid int variable
 *   If <tt>oSubscribeCBId</tt> is not NULL , at the successul  completion of the <tt>AddFileDescriptor</tt>
 * @return 0 if the transaction/idle subscription is successfully added and 0 otherwise.
 */
      int Subscribe(int          iSubscribingType,
                    CATCommand * iToClient,
                    void       * iClientData,
                    CATSysSubscribeCB iUserStaticMethod,
                    int          *oSubscribeCBId=NULL
                   );



      // Remove Subscribe
      //-----------------
/**
 * Removes a transaction/idle subscription from a command.
 * @param iSubscribedType 
 *   The subscribed type.
 *   <br><b>Legal values</b>: Can be set to <tt>CATSubscribeEndTransaction</tt>
 *   (=1) or to <tt>CATSubscribeIdle</tt> (=2) 
 *  @param  iToClient
 *   The command for which the subscription is to be removed.
 * @return 0 if the transaction/idle subscription is successfully removed and 0 otherwise.
 * @see #Subscribe
 */
      int RemoveSubscribe(int          iSubscribedType,
                          CATCommand * iToClient);

/**
 * Removes a transaction/idle subscription from a command.
 * @param iSubscribedType 
 *   The subscribed type.
 *   <br><b>Legal values</b>: Can be set to <tt>CATSubscribeEndTransaction</tt>
 *   (=1) or to <tt>CATSubscribeIdle</tt> (=2) 
 * @param iSubscribeCBId
 *   The id of the subscription  to be removed ( returned through the parameter 
 *   oSubscribeCBId of the  @href CATApplication#Subscribe method)
 * @param iReserved
 *   This parameter must me set to zero
 * @return 0 if the transaction/idle subscription is successfully removed and 0 otherwise.
 * @see #Subscribe
 **/
      int RemoveSubscribe(int          iSubscribedType,
                          int          iSubscribeCBId,
                          int          iReserved=0 );

      // Objet du framework interne
      //---------------------------
/**
 * Returns a pointer to the application data member.
 * @return The application data member pointer, or the first application
 * created if several application instances exist.
 * Do never release this returned pointer.
 */
      static  CATApplication * MainApplication();

      // CATNotifier Instance
      // --------------------
	  /** @nodoc */
      CATNotifier * _ClientManagerPtr;

/**
 * @nodoc
 * Main function.
 * @param argc
 *   The <tt>argc</tt> command line argument containing the number
 *   of arguments passed to the main function (argument count).
 * @param argv
 *   The <tt>argv</tt> command line argument, pointer to a character
 *   string table containing the arguments (argument vector).
 * @param env
 *   The pointer to the environment list.
 */
  static int Main(int     argc,
                  char ** argv,
                  char ** env);

      // Application Starting
      //------------------------------
/**
 * Begins the application.
 * <br><b>Role</b>:
 * This method must be redefined to contain the main objects instantiation. 
 * It should contain the whole application code.
 */
      virtual void BeginApplication();

      // Application Termination 
      //------------------------------
/**
 * Ends the application.
 * <br><b>Role</b>:
 * This method should be redefined to include end application tasks,
 * such as closing files.
 * @return 
 *   The application return code.
 */
      virtual int EndApplication();

      // Application Building
      //------------------------------
/**
 * Builds the application environment.
 * <br><b>Role</b>:
 * This environment can be, for example, the different windows used by the
 * application.
 * @param argc
 *   The <tt>argc</tt> command line argument containing the number
 *   of arguments passed to the main function (argument count).
 * @param argv
 *   The <tt>argv</tt> command line argument, pointer to a character
 *   string table containing the arguments (argument vector).
 * @param env
 *   The pointer to the environment list.
 */
      virtual void BuildApplicationEnvironment(int     argc,
                                               char ** argv,
                                               char ** env);

      // Application Loop
      //---------------------
/**
 * Accommodates the event loop.
 * <br><b>Warning</b>: It cannot be used with Windows, since the event loop is
 * managed only by the operating system.
 */
      virtual void RunApplication();

      // CATCommand Data Management
      // --------------------------
/**
 * @nodoc
 * Redefinition of the SaveState method of the CATCommand class.
 * @see CATCommand#SaveState
 */
      virtual int SaveState(CATNotification * iNotification,
                            CATMarshal      & iDataToSave);
/**
 * @nodoc
 * Redefinition of the RestoreState method of the CATCommand class.
 * @see CATCommand#RestoreState
 */
      virtual int RestoreState(CATNotification * iNotification,
                               CATMarshal      & oDataToRestore);

 

   private :

      //===================================================
      // Data
      //===================================================

      // CATApplication Instance
      //-------------------------
      static CATApplication* First;

      void BeginCB(CATCommand*, CATNotification *, CATCommandClientData);


};
#endif
