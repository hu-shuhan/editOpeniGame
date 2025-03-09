//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasMemPool_h
#define SMABasMemPool_h

///////////////////////////////////////////////////////////////////////////////
//
// Standalone, self-sufficient memory pool, independent of CacheManager.
// Used for tracking memory allocations outside of CacheManager/SRM.
// ---------------------------------------------------------------------------
// By default, it is in inactive mode. A call to .Activate() is needed
// to enable its functionality. This is done in the ABAQUS initialization
// sequence.
//
///////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>

//Begin local includes
//End local includes


// TODO: Proper Locks


class SMABasMemCacheManager;

class SMABasMemPool
{
    friend class SMABasMemCacheManager;

    enum IncreaseDecrease { e_Increase, e_Decrease };

public:

    SMABasMemPool(          const int  poolId,
                          const char*  poolName,
               SMABasMemCacheManager*  cacheManager);

    ~SMABasMemPool();


    void*   Alloc(const size_t size); /// Allocate
    size_t  Free (void* ptr);         /// Deallocate. Returns the size of memory freed.

    size_t MemoryCurrent();    /// Return the current memory total, in bytes.
    size_t MemoryPeak();       /// Return the historic memory peak, in bytes.

    int    PoolID();     /// Return the ID of this pool.
    char*  PoolName();   /// Return the Name of this pool.


    /// Given a pointer, returns the size of the allocation associated with it (complete chunk size).
    size_t AllocSize(void* ptr) const;
   
    /// Given a pointer, returns the usable (payload) size of the allocation associated with it.
    size_t ActualSizeForUse(void* ptr) const;

    // Set the memory upper and lower bound for memory enforcement
    void SetMemoryLimit(const size_t memLow,
            const size_t memHigh,
            const double tolerance);  

    void MemoryReport();

    size_t MemoryStagedMax() { return m_stagedMaxMemoryUsed; }

    void ZeroMax() { 
      // m_memoryLimitLock->Lock(); 
      m_stagedMaxMemoryUsed = m_memoryCurrent;
      // m_memoryLimitLock->Unlock();
    }
    
    void Activate()   { m_active = true;  }
    void Deactivate() { m_active = false; }

    bool Active()  const { return m_active; }

 private:

    void UpdateMemoryInUse(enum IncreaseDecrease, const size_t size);

 protected:

    bool                m_active;

    
    size_t              m_memoryCurrent;
    size_t              m_maxMemoryUsed;

    int                 m_poolId;
    char                m_poolName[1024];

    SMABasMemCacheManager*     m_cacheManager;

    size_t                     m_stagedMaxMemoryUsed;

    size_t                     m_memoryLow;
    size_t                     m_memoryHigh;
    double                     m_tolerance;

    // SMABasAtiMutex*            m_memoryLimitLock; 
};

#endif // SMABasMemPool_h
