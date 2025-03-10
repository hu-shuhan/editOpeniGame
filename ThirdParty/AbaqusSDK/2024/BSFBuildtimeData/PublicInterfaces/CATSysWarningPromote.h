// COPYRIGHT DASSAULT SYSTEMES 2000

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

#ifdef _WINDOWS_SOURCE
#ifndef H_CATNoWarningPromotion_HH
#define H_CATNoWarningPromotion_HH

#ifndef _CATNoWarningPromotion_

#pragma warning(error: 4700)
#pragma warning(error: 4355)
#pragma warning(error: 4309)
#pragma warning(error: 4305)
#pragma warning(error: 4101)
#pragma warning(error: 4013)
#pragma warning(error: 4259)
#pragma warning(error: 4060)
#pragma warning(error: 4065)
#pragma warning(error: 4146)
#pragma warning(error: 4150)
#pragma warning(error: 4156)
#pragma warning(error: 4200)
#pragma warning(error: 4172)
#pragma warning(error: 4243)
#pragma warning(error: 4508)
#if defined(_MFC_VER) && _MFC_VER>=0x0800
#else
#pragma warning(error: 4530)
#endif
#pragma warning(error: 4552)
#pragma warning(error: 4553)
#pragma warning(error: 4554)
#pragma warning(error: 4715)

#if defined(_MSC_VER) && _MSC_VER>=1300
#pragma warning(error: 4717)
#pragma warning(error: 4318)
#pragma warning(error: 4700)
#pragma warning(error: 4183)
#pragma warning(error: 4930)
#pragma warning(error: 4311)
//#pragma warning(error: 4312)
//#pragma warning(error: 4313)
#endif

#endif
#endif
#endif
