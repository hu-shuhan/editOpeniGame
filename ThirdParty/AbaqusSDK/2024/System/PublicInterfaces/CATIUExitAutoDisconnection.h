#ifndef CATIUExitAutoDisconnection_H 
#define CATIUExitAutoDisconnection_H

/* COPYRIGHT DASSAULT SYSTEMES 2001 */
/**
 * @CAA2Level L1
 * @CAA2Usage U5
 */

#include "IUnknown.h"
#include "CATMacForIUnknown.h"
#include "JS0ERROR.h"


/**
* If the termination may occur.
* @see CATIUExitAutoDisconnection
*/
#define CATIUExitDisconnectionAuthorized -1
/**
* if the termination must NOT occur.
* @see CATIUExitAutoDisconnection
*/
#define CAIIUExitDisconnectionForbidden  0 
 
extern ExportedByJS0ERROR  const  IID IID_CATIUExitAutoDisconnection;
     
  /**
   *  This is the interface of the AutoDisconnection User Exit.
   *  <b>Role</b>: This interface is an user exit, that allows a customer administrator
   *  or user (at administrator convenience)  to perform specific operation 
   *  before the automatic disconnection of a CATIA Session 
   *  In particular this allow the administrator or user to decide if the disconnection
   *  may occur.  
   *  <br>This interface must be implemented on the <tt>CATUExittAutoDisconnection</tt> 
   *  component by creating
   *  a data extension of this object.
   **/


class ExportedByJS0ERROR CATIUExitAutoDisconnection : public IUnknown
{
   CATDeclareInterface;

public:
  /**
   * This method will be called when the automatic disconnection processus is triggered.
   *
   * <br><b>Role</b>: This method allows the administrator/user to perform some operations 
   * when the automatic disconnection processus is triggered but before the effective
   * session termination. It allows the administrator/user to take into account parameters
   * unknown to the V5 application to decide if the processus termination must occur or not.
   * @param oDisconnectionAllowed
   *  2 values possible: 
   *  <ul>
   *  <li><b>CATIUExitDisconnectionForbidden</b>  :if the termination must NOT occur.</li>
   *  <li><b>CATIUExitDisconnectionAuthorized</b> :if the termination may occur. </li>
   *  </ul>
   *  
   */

  virtual HRESULT OnDisconnection (int *oDisconnectionAllowed )=0;

  
};

#endif
