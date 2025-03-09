//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef thr_Mutex_h
#define thr_Mutex_h

// Begin local includes
#include <thr_ThreadingModel.h>
#include <SMABasAtiMutex.h>
// End local includes


//-----------------------------------------------------------------
// PTHREADS IMPLEMENTATION
//-----------------------------------------------------------------
#if defined HKS_PTHREADS

class thr_Mutex  : public SMABasAtiMutex
{
public:
     enum thr_MutexStatus { thr_Unlocked, thr_Locked };
     thr_Mutex(const thr_MutexStatus initialState = thr_Unlocked);
     virtual ~thr_Mutex();

     void Lock();
     void Unlock();
     // TryLock - returns 1 if lock is acquired, 0 otherwise
     int  TryLock();
     thr_MutexStatus GetStatus() { return m_status; }

     virtual int id() const { return 201; }

private:
     thr_Mutex(const thr_Mutex& source); // Do not allow Mutex copy.
     thr_Mutex& operator=(thr_Mutex& source); // Do not allow use of assignment for mutexes.

     // m_lock is pointer to pthread_mutex_t in order to avoid requiring 
     // pthread.h in header file. 
     void*              m_lock;
     // Status is unnecessary except as required by GetStatus.
     thr_MutexStatus    m_status;
};

//-----------------------------------------------------------------
// WINDOWS THREADS IMPLEMENTATION
//-----------------------------------------------------------------
#elif defined HKS_WINTHREADS

#include <SynchAPI.h>
// Begin local includes
#include <thr_SpinMacros.h>
// End local includes

class thr_Mutex  : public SMABasAtiMutex
{
public:
  enum thr_MutexStatus { thr_Unlocked=0, thr_Locked=1 };
  thr_Mutex(const unsigned int initialState = thr_Unlocked);
  virtual ~thr_Mutex();
  void Lock();
  void Unlock();
  // returns 1 if lock is acquired, 0 otherwise
  int TryLock();
  unsigned int GetStatus() { return m_lock; }

  virtual int id() const { return 202; }

private:
  thr_Mutex(const thr_Mutex& source); // Do not allow Mutex copy.
  thr_Mutex& operator=(thr_Mutex& source); // Do not allow use of assignment for mutexes.

  int   m_lock;
  CRITICAL_SECTION m_criticalSection;
};

//-----------------------------------------------------------------
// NO THREADS
//-----------------------------------------------------------------
#else

class thr_Mutex  : public SMABasAtiMutex
{
public:
    enum thr_MutexStatus { thr_Unlocked, thr_Locked };
    thr_Mutex(const thr_MutexStatus initialState = thr_Unlocked);
    virtual ~thr_Mutex();

    void Lock();
    void Unlock();
    // returns 1 if lock is acquired, 0 otherwise
    int  TryLock();
    thr_MutexStatus GetStatus() const { return thr_Unlocked; }    

    virtual int id() const { return 203; }
private:
    thr_Mutex(const thr_Mutex& source); // Do not allow Mutex copy. Confusing for user.
    thr_Mutex& operator=(thr_Mutex& source); // Do not allow use of assignment for mutexes.     
    
    thr_MutexStatus  m_state;
};

#endif /* HKS_PTHREADS, HKS_WINTHREADS, NO THREADS */

#endif // thr_ThreadPrimitives_h




