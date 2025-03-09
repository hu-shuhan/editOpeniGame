/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PCurrentGroupModifNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PCurrentGroupModifNotif
#define _CATP2PCurrentGroupModifNotif

// COPYRIGHT DASSAULT SYSTEMES 2003

/**
 * @CAA2Level L0
 * @CAA2Usage U1
 */


#include "CATP2PGroupModifNotif.h"

/**
 * Concrete class.
 * <b>Role</b>: Current group notification.
 * This notification is emitted each time the current group is changed.
 */


class ExportedByCATP2PCore CATP2PCurrentGroupModifNotif : public CATP2PGroupModifNotif
{

 public:

   CATP2PCurrentGroupModifNotif(const CATUnicodeString* peer_name, CATListValCATUnicodeString* subGroup, const CATUnicodeString * currentGroup);

   virtual ~CATP2PCurrentGroupModifNotif();

   CATDeclareClass;


};


#endif
