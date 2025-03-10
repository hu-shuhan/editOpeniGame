#ifndef _CATNotificationWithData
#define _CATNotificationWithData
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
#include "CATNotification.h"

// COPYRIGHT DASSAULT SYSTEMES 2001

/*  Usage Notes: Event Container */

#include "JS0SCBAK.h"


class ExportedByJS0SCBAK CATNotificationWithData : public CATNotification
{
  protected:
   
   CATNotificationWithData();
   CATNotificationWithData( void *idata );

  public:
 
   virtual ~CATNotificationWithData();

   CATDeclareClass;
    
   // Set/Get the Notification string
   // -------------------------------
   void* GetNotificationData();
   void  SetNotificationData(void* idata);


 private:

   void* _data;
};
#endif
