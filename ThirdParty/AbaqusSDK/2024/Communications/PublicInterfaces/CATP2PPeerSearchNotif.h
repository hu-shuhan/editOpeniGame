/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PPeerSearchNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PPeerSearchNotif
#define _CATP2PPeerSearchNotif

// COPYRIGHT DASSAULT SYSTEMES 2003

/**
 * @CAA2Level L0
 * @CAA2Usage U1
 */

#include "CATP2PNotif.h"

class CATUnicodeString;
class CATListValCATUnicodeString;

/**
 * Concrete class.
 * <b>Role</b>: Peer search notification.
 * This notification is emitted each time a peer search is launched.
 */


class ExportedByCATP2PCore CATP2PPeerSearchNotif : public CATP2PNotif
{

 public:

   CATP2PPeerSearchNotif(const CATUnicodeString* peer_name);

   virtual ~CATP2PPeerSearchNotif();

 
   CATDeclareClass;


 private:




};


#endif
