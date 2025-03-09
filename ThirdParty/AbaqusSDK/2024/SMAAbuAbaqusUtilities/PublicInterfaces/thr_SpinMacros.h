//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef thr_SpinMacros_h
#define thr_SpinMacros_h

// Begin local includes

#include <thr_ThreadingModel.h>

// End local includes

//-----------------------------------------------------------------
// PTHREADS IMPLEMENTATION
//-----------------------------------------------------------------
#if defined HKS_PTHREADS

// Remove definitions from compile files. 
#undef THR_SPIN_LOOP
#undef THR_SPIN_YIELD

// use THR_SPIN_LOOP for increased performance
#define THR_SPIN_LOOP

#if defined THR_SPIN_YIELD
#include	<sched.h>
#define THR_SPIN sched_yield()
#elif defined THR_SPIN_LOOP
#define THR_SPIN
#endif

//-----------------------------------------------------------------
// WINDOWS THREADS IMPLEMENTATION
//-----------------------------------------------------------------
#elif defined HKS_WINTHREADS

#undef THR_SPIN_LOOP
#undef THR_SPIN_YIELD

#define THR_SPIN_LOOP

#if defined THR_SPIN_YIELD
#define THR_SPIN Sleep(0)
#elif defined THR_SPIN_LOOP
#define THR_SPIN
#endif

//-----------------------------------------------------------------
// NO THREADS IMPLEMENTATION
//-----------------------------------------------------------------
#else

#undef THR_SPIN_LOOP
#undef THR_SPIN_YIELD

#define THR_SPIN_LOOP
#define THR_SPIN

#endif /* HKS_PTHREADS, HKS_WINTHREADS, NO THREADS */

#endif /* thr_SpinMacros_h */
