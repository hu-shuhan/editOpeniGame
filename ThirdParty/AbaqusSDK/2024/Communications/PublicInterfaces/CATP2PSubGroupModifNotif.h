/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PSubGroupModifNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PSubGroupModifNotif
#define _CATP2PSubGroupModifNotif

// COPYRIGHT DASSAULT SYSTEMES 2003


/**
 * @CAA2Level L0
 * @CAA2Usage U1
 */


#include "CATP2PGroupModifNotif.h"


/**
 * Concrete class.
 * <b>Role</b>: Sub group modification notification.
 * This notification is emitted each time the sub group list is changed.
 */


class ExportedByCATP2PCore CATP2PSubGroupModifNotif : public CATP2PGroupModifNotif
{

 public:

   CATP2PSubGroupModifNotif(const CATUnicodeString* peer_name, CATListValCATUnicodeString* subGroup, const CATUnicodeString * currentGroup);

   virtual ~CATP2PSubGroupModifNotif();

   CATDeclareClass;


};


#endif
