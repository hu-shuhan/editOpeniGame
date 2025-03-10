/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PClosePeerNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PClosePeerNotif
#define _CATP2PClosePeerNotif

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
 * <b>Role</b>: Close peer notification.
 * This notification is emitted each time a peer is closed.
 */

class ExportedByCATP2PCore CATP2PClosePeerNotif : public CATP2PNotif
{

 public:

   CATP2PClosePeerNotif(const CATUnicodeString* peer_name);

   virtual ~CATP2PClosePeerNotif();

 
   CATDeclareClass;


 private:




};


#endif
