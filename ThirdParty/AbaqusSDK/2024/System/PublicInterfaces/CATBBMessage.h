#ifndef  CATBBMessage_H
#define  CATBBMessage_H
#include "CATBaseUnknown.h"
#include "CATMessageClass.h"

/*
// COPYRIGHT DASSAULT SYSTEMES 1999
*/

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */


/**
 * Base class for backbone messages.
 * <b>Role:</b> A backbone message is a message which may be sent to another 
 * application through the backbone communicator.
 * <p>
 *  A backbone message is a component:
 * <ul>
 *  <li>Which OM derives from the CATBBMessage component</li>
 *  <li><b>Whose implementation class C++ derives from the CATBBMessage class</b></li>
 *  <li>Which implements @href CATIStreamMsg </li>
 *  <li>Which implements @href CATICreateInstance using a code extension.</li>
 * </ul>
 * @see CATICommMsg, CATIMessageReceiver, CATICommunicator, CATICommunicatorLoop
 */

class ExportedByCATSysCommunication CATBBMessage : public CATBaseUnknown
{
  /**
   * @nodoc
   */
  CATDeclareClass;

  /**
   * Default constructor.
   */
  CATBBMessage();
};

#endif
