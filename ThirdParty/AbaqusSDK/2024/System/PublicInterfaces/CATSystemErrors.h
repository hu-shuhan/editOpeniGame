#ifndef _CATSYSTEMERRORS_H
#define _CATSYSTEMERRORS_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 1999

typedef enum {

                    CATIntError1 = 0x00020001u,

                    CATInpError1 = 0x00020001u,

               CATResOutOfMemory = 0x00020001u,

                    CATSysSIGFPE = 0x00020001u,
                    CATSysSIGBUS = 0x00020002u,
                   CATSysSIGSEGV = 0x00020003u,
                    CATSysSIGILL = 0x00020004u

} SystemErrors;

#endif
