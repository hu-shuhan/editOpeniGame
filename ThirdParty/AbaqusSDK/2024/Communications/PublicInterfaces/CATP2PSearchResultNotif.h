/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PSearchResultNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PSearchResultNotif
#define _CATP2PSearchResultNotif

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
 * <b>Role</b>: Peer search result notification.
 * This notification is emitted each time a peer search result is received.
 */

class ExportedByCATP2PCore CATP2PSearchResultNotif : public CATP2PNotif
{

 public:

   CATP2PSearchResultNotif(const CATUnicodeString* peer_name, const CATUnicodeString* group_name);

   virtual ~CATP2PSearchResultNotif();

 /** 
 *  Notification information method.
 * <br><b>Role</b>: This method return the discovered peer group name.
 */


   const CATUnicodeString* getGroupName();

 
   CATDeclareClass;


 private:

	 const CATUnicodeString* _groupName;




};


#endif
