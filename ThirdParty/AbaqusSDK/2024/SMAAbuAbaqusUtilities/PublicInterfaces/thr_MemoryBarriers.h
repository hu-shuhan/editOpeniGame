//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef thr_MemoryBarriers_h
#define thr_MemoryBarriers_h

//
// Memory Barriers 
//
// Memory order may be different from program order.
// Loads and stores to different physical locations are allowed 
// to complete out of order on the issuing processor as observed 
// by the other processors.
//
// This is bacause memory accesses may be aggregated and/or buffered 
// and may complete in an order different from the program order. 
// MemoryBarriers ensure that memory operations before the barrier 
// complete prior to all the memory operations following the barrier.
//

// Implementation is in thr/impl/thr_MemoryBarriers.C for now until
// things stabilize. They can be moved in to the header for inlining
// if needed later - Harun.

// Full memory barrier for loads and stores
void thr_MemoryBarrier();

// WriteMemoryBarrier is a subset of MemoryBarrier. It ensures that 
// all stores before this call complete prior to all the stores 
// after the call.
void thr_WriteMemoryBarrier();

// ReadMemoryBarrier is a subset of MemoryBarrier. It ensures that 
// all loads before this call complete prior to all the loads 
// after the call.
void thr_ReadMemoryBarrier();

#endif // thr_MemoryBarriers_h



