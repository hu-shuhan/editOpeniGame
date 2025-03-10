/*=======================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                      */
/*=======================================================================*/
/*                                                                       */
/*  CATP2PGroupModifNotif                                                        */
/*                                                                       */
/*  Usage Notes:            */
/*                                                                       */
/*=======================================================================*/
/*  Creation May	2002                                 jnm             */
/*=======================================================================*/
#ifndef _CATP2PGroupModifNotif
#define _CATP2PGroupModifNotif

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
 * <b>Role</b>: Group modification notification.
 * This notification is emitted each time something in the group is changed :
 * or the current group or the subgroups.
 * see the subclasses CATP2PCurrentGroupModifNotif and CATP2PSubGroupModifNotif.
 */


class ExportedByCATP2PCore CATP2PGroupModifNotif : public CATP2PNotif
{

 public:

   CATP2PGroupModifNotif(const CATUnicodeString* peer_name, CATListValCATUnicodeString* subGroup, const CATUnicodeString * currentGroup);

   virtual ~CATP2PGroupModifNotif();

/** 
 *  Notification information method.
 * <br><b>Role</b>: This method return the new list of known subgroups.
 */

   CATListValCATUnicodeString* getSubGroups();

   /** 
 *  Notification information method.
 * <br><b>Role</b>: This method return the current group.
 */

    const CATUnicodeString * getCurrentGroup();

   CATDeclareClass;


 private:

	 const CATUnicodeString * _currentGroup;
	 CATListValCATUnicodeString* _currentSubGroups;


};


#endif
