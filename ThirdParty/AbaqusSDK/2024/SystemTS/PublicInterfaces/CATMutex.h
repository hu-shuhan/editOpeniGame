#ifndef CATMutex_h
#define CATMutex_h

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

//=============================================================================
// Macros for fast locks and normal locks
//=============================================================================
#include "CATConstantsForThreads.h"
#define CATMutexLock(a) (a)->Lock()
#define CATMutexUnlock(a) (a)->Unlock()
#define CATMutexTryLock(a) (a)->TryLock()
#define CATMutexFastLock(a)  ((a)->*CATMutex::LockF)()
#define CATMutexFastUnlock(a)  ((a)->*CATMutex::UnlockF)()
#define CATMutexFastTryLock(a)  ((a)->*CATMutex::TryLockF)()
  
class ExportedByJS0MT CATThreadsData {
};

class CATMutexRealData;

/**
 * Class used to implement mutex on all OS.
 */
class ExportedByJS0MT CATMutex {
	
	public:
	
		/**
		 * Constructor.
		 * @param iThreadLockIt
		 * Specifies whether the mutex must be locked after creation.
		 * @param iRecursive
		 * Specifies whether the mutex can be recursive or not. 
		 * Because this parameter is not effective on Windows where CATMutex are always recursive, 	set it preferably to 1.
		 * @param iLazyCreation
		 * Specifies whether the mutex creation is deferred on the first
		 * call to Lock() or is made directly in the constructor.
		 */
		CATMutex(int iThreadLockIt = 0,
				 int iRecursive = 1,
				 int iLazyCreation = 0);
		
		/**
		 * Destructor
		 */
		 ~CATMutex();
		
		//============================================================================
		// METHODS
		//============================================================================

		/**
		 * Acquires a lock on the mutex
		 */
		void Lock();
		
		/**
		 * Releases a lock on the mutex
		 */
		void Unlock();
		
		/**
		 * Try the lock the mutex. 
		 * @return AllreadyLocked if it fails.
		 */
		int TryLock(); 
		
		/**
		 * Returns 1 if the mutex is created, 0 otherwise (case of lazy creation).
		 */
		inline int IsCreated();
		
		static void (CATMutex::*LockF)(void);
		
		// Releasing a lock on the mutex
		static void (CATMutex::*UnlockF)(void);
		
		// Trying the lock the mutex.
		// or AllreadyLocked if it fails.
		static int (CATMutex::*TryLockF)(void);
		
	private:

		/**
		 * Copy Constructor.
		 */
		CATMutex(const CATMutex&);
        
        void LockM();
		void UnlockM();
		int TryLockM();
        
		//============================================================================
		//  DATAS
		//============================================================================
		
		CATMutexRealData * _Data;
};

inline int CATMutex::IsCreated() {
	return (_Data ? 1: 0);
}

#endif // CATMutex_h

