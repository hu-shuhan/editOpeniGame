#ifndef CATLMMutex_H
#define CATLMMutex_H

// COPYRIGHT DASSAULT SYSTEMES 2004
/**
 * @CAA2Level L0
 * @CAA2Usage U0
 */

#include "JS0LIB0.h"
class CATMutex;

/**
 * Try succesful.
 */
#define CATLMMutexSuccessful 0

/**
 * Try failure.
 */
#define CATLMMutexFailure    1

/**
 * Try error.
 */
#define CATLMMutexError      -1

/**
 * Class providing a mutex.
 */
class ExportedByJS0LIB0 CATLMMutex
{
  public:

  /**
    * Constructor.
    */
  CATLMMutex();

  /**
    * Destructor.
    */
   ~CATLMMutex();

  /**
    * Acquires a lock on the mutex.
    */
  void Lock();

  /**
    * Releases a lock on the mutex.
    */
  void Unlock();

  /**
    * Tries the lock of the mutex.
    * @return
    *   CATLMMutexSuccessful<br>
    *   CATLMMutexFailure<br>
    */
  int TryLock();

  private:
  /**
    * @nodoc
    */
    CATMutex *_Mutex;

  /**
    * @nodoc
    */
    CATLMMutex(const CATLMMutex&);
};
#endif

