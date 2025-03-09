/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PPeerDestructNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PPeerDestructNotif
#define _CATP2PPeerDestructNotif

// COPYRIGHT DASSAULT SYSTEMES 2003

/**
 * @CAA2Level L0
 * @CAA2Usage U1
 */



#include "CATNotification.h"
#include "CATP2PPeerModifNotif.h"

/**
 * Concrete class.
 * <b>Role</b>: Desctuct peer notification.
 * This notification is emitted each time a peer leave the current group.
 */

class ExportedByCATP2PCore CATP2PPeerDestructNotif : public CATP2PPeerModifNotif
{

 public:

   CATP2PPeerDestructNotif(const CATUnicodeString* peer_name);
   virtual ~CATP2PPeerDestructNotif();

   CATDeclareClass;


 private:

};


#endif
