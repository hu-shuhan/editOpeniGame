
#ifndef		CATRCOLL_Misc_h
#define		CATRCOLL_Misc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

//
// To ease loop programming
// ========================

#define CATRCOLL_FORWARD_LOOP( RC, ITR )                                \
{                                                                       \
  int const CATRCOLL_LoopLimit##ITR = (RC).Size() + 1 ;                 \
  int ITR ;                                                             \
  for ( ITR = 1; ITR < CATRCOLL_LoopLimit##ITR; ITR++ )


#define CATRCOLL_BACKWARD_LOOP( RC, ITR )                               \
{                                                                       \
  int ITR ;                                                             \
  for ( ITR = (RC).Size(); ITR > 0; ITR-- )


#define CATRCOLL_END_LOOP  }

// 
// OSTREAM
// =======

#define	CATRCOLL_APPLY_OSTRM( RC, OS )			                \
{                                                                       \
  int LoopLimit = (RC).Size() ;                                         \
  for ( int i = 1 ; i <= LoopLimit ; i++ )                              \
  {                                                                     \
    OS << "RC[" << i << "]=" << (RC)[i] << endl ;                       \
  }                                                                     \
}

#endif
