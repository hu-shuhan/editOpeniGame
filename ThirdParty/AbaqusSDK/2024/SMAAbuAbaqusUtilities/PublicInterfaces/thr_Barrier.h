//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef thr_Barrier_h
#define thr_Barrier_h

// Begin local includes

#include <mem_AllocationOperatorsTiny.h>
#include <thr_ThreadingModel.h>
#include <SMABasAtiBarrier.h>
#include <thr_Mutex.h>
#include <thr_CondVar.h>

// End local includes

//---------------------------------------------------------------------------
// THREADED IMPLEMENTATION.
//---------------------------------------------------------------------------
#if defined(HKS_PTHREADS) || defined(HKS_WINTHREADS)

typedef void (*BarrierErrHandler)();

class thr_Barrier  : public SMABasAtiBarrier
{
public:
    enum thr_BarrierStatus { THR_BAR_RECEIVING, THR_BAR_RELEASING };

    thr_Barrier();
    virtual ~thr_Barrier();
    virtual void BarrierSync(const unsigned int gangSize);
    static void SetSuspendFlag(const int flag); // See the comment by m_suspendFlag
    static void CheckSuspendOnBarrier(const int flag);
    
private:
    // Prevent use of copy constructor and assignement as copying a barrier 
    // gets ugly.
    thr_Barrier(const thr_Barrier& source);
    thr_Barrier& operator=(const thr_Barrier& source);

    thr_Mutex          m_lock;
    int                m_counter;
    volatile thr_BarrierStatus  m_state;
    int                m_errorflag;
    thr_CondVar        m_cv;
    unsigned int       m_numberOfSpins;
    
    // This flag can by set by calling SetSuspendFlag (generally done via the licensing code)
    // In the case of a licensing suspension, this flag will be set by the licensing thread. 
    // All threads that then call BarrierSync will wait patiently for this flag to be cleared
    // (instead of spinning at 100%)
    static int         m_suspendFlag;
    static int         m_allowSuspendCheck;

};

//---------------------------------------------------------------------------
// HKS_NOTHREADS IMPLEMENTATION
//---------------------------------------------------------------------------
#else // HKS_NOTHREADS

class thr_Barrier  : public SMABasAtiBarrier
{
public:
    thr_Barrier() { }
    ~thr_Barrier() { }
    
    void BarrierSync(const unsigned int gangSize) { }
    static void SetSuspendFlag(const int flag) { }
    static void SetMPIBcastFlag(const int flag) { }
    static void CheckSuspendOnBarrier(const int flag) { }

private:
    // Prevent use of copy constructor and assignement as copying a barrier 
    // gets ugly.
    thr_Barrier(const thr_Barrier& source);
    thr_Barrier& operator=(const thr_Barrier& source);
};

#endif /* THREADED, NO THREADS */
#endif /* thr_Barrier_h */
