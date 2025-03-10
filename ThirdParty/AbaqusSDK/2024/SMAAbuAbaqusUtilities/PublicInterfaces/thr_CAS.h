#ifndef thr_CAS_h
#define thr_CAS_h
//==============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//==============================================================================
// Begin local includes
// End local includes
bool thr_CompareAndSwapUInt(volatile unsigned int* addr,unsigned int old_val, unsigned int new_val);
bool thr_CompareAndSwapInt(volatile int* addr,int old_val,int new_val);
int thr_AtomicAdd(volatile int& value, int incr);
#if defined _LINUX_SOURCE
int thr_FetchAndAdd(volatile int* p, int incr);
#endif

int thr_AtomicAnd(volatile int* valPtr, int bitOp);
int thr_AtomicAnd(volatile int& value, int bitOp);

int thr_AtomicOr(volatile int* valPtr, int bitOp);
int thr_AtomicOr(volatile int& value, int bitOp);
#endif //thr_CAS_h
