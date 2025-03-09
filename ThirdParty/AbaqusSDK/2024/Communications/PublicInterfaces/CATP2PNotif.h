/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PNotif
#define _CATP2PNotif

// COPYRIGHT DASSAULT SYSTEMES 2003


/**
 * @CAA2Level L0
 * @CAA2Usage U1
 */

#include "CATNotification.h"
#include "CATP2PCore.h"

class CATUnicodeString;


/**
 * Concrete class. 
 * <b>Role</b>: P2P Notif services base class.
 * This class is the base class for all the notification used in the P2P plateform.
 */



class ExportedByCATP2PCore CATP2PNotif : public CATNotification
{

 public:

   CATP2PNotif(const CATUnicodeString* peer_name);
   virtual ~CATP2PNotif();

   /** 
 *  Notif information method.
 * <br><b>Role</b>: This method retrieve the peer name attached to the notification.
 */
   const CATUnicodeString* getPeerName();

   CATDeclareClass;


 private:

	 const CATUnicodeString* _peerName;

};


#endif
