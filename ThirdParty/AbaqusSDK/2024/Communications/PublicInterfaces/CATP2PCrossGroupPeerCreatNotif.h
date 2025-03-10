/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PCrossGroupPeerCreatNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PCrossGroupPeerCreatNotif
#define _CATP2PCrossGroupPeerCreatNotif

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
 * This notification is emitted each time a cross group peer is found.
 */

class ExportedByCATP2PCore CATP2PCrossGroupPeerCreatNotif : public CATP2PNotif
{

 public:

   CATP2PCrossGroupPeerCreatNotif(const CATUnicodeString* peer_name, const CATUnicodeString* group_name);

   virtual ~CATP2PCrossGroupPeerCreatNotif();


/** 
 *  Notification information method
 * <br><b>Role</b>: This method is gives you the group of the discovered peer
 */
   const CATUnicodeString* getGroupName();

 
   CATDeclareClass;


 private:

	 const CATUnicodeString* _groupName;




};


#endif
