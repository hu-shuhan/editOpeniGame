/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PPeerCreatNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PPeerCreatNotif
#define _CATP2PPeerCreatNotif

// COPYRIGHT DASSAULT SYSTEMES 2003


/**
 * @CAA2Level L0
 * @CAA2Usage U1
 */


#include "CATP2PPeerModifNotif.h"

class CATUnicodeString;
class CATListValCATUnicodeString;

/**
 * Concrete class. 
 * <b>Role</b>: Close peer notification.
 * This notification is emitted each time a peer enters a group.
 */

class ExportedByCATP2PCore CATP2PPeerCreatNotif : public CATP2PPeerModifNotif
{

 public:

   CATP2PPeerCreatNotif(const CATUnicodeString* peer_name);

   virtual ~CATP2PPeerCreatNotif();

 
   CATDeclareClass;


 private:




};


#endif
