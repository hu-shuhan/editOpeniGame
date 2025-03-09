/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PPeerModifNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PPeerModifNotif
#define _CATP2PPeerModifNotif

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
 * <b>Role</b>: Peer modification notification.
 * This notification is emitted each time a peer enter or leave a group.
 */


class ExportedByCATP2PCore CATP2PPeerModifNotif : public CATP2PNotif
{

 public:

   CATP2PPeerModifNotif(const CATUnicodeString* peer_name);

   virtual ~CATP2PPeerModifNotif();

 
   CATDeclareClass;


 private:




};


#endif
