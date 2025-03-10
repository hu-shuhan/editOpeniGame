#ifndef CATICommunicatorLoop_H
#define CATICommunicatorLoop_H
#include "CATICommMsg.h"

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

//class CATIMessageReceiver_var ; // - 03.02.2010
#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByCATSysCommunication IID IID_CATICommunicatorLoop;
#else
extern "C" const IID IID_CATICommunicatorLoop;
#endif

/**
 * Interface to wait on Backbone.            
 * <b>Role</b>: This interface must be used in none interactive
 * backbone clients and  never be called in interactive clients
 * like Dialog applications or CATIA V5.
 * @see CATIStreamMsg, CATIBBStreamer, CATICommMsg, CATIMessageReceiver, CATBBMessage, CATICommunicator
 */
class ExportedByCATSysCommunication CATICommunicatorLoop : public IUnknown
{
  CATDeclareInterface;
  public:
  /**
   * Allows to wait on the backbone.
   * <b>Role</b>: This method will wait for backbone messages until one 
   *  of the following conditions are met :
   * <ul>
   * <li> A period of TimeOut ms is passed without any backbone messages occurence </li>
   * <li> iEndCondition argument was non null and its value becomes null  </li>
   * </ul>
   * @param iGeneralWaiting 
   *   must be set to -1
   *
   * @param iTimeOut 
   * <ul>
   *   <li>-1: No timeout, so infinite wait </li>
   *   <li> 0: no wait: <tt>WaitingLoop</tt> will treat the current messages and exit immediately </li>
   *   <li>>0: timeout represents milliseconds to wait</li>
   * </ul>
   *
   * @param iEndCondition  
   *    If you don't use this parameter, set NULL else set the pointer of a variable 
   *    whose contains is not null. 
   */
  virtual HRESULT WaitingLoop(int iGeneralWaiting=-1, int iTimeOut=-1 , int *iEndCondition=NULL)=0;
};
/**
 * @nodoc 
 * Declare the handler class for CATICommunicatorLoop :  CATICommunicatorLoop_var
 */
CATDeclareHandler (CATICommunicatorLoop,IUnknown);

#endif
