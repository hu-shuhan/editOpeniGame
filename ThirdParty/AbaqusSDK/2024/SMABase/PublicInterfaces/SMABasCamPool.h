//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasCamPool_h
#define SMABasCamPool_h

#include <sys/types.h>

//Begin local includes
//#include <SMABasCamMemoryVerbose.h>
#include <mem_AllocationOperators.h>
#include <SMABasAtiMutex.h>
//End local includes


class SMABasCamCacheManager;

class SMABasCamPool : public mem_AllocationOperators
{
public:
    friend class SMABasCamCacheManager;

    //returns the allocation size for a pointer allocated through the pool
    size_t AllocSize(void* ptr) const;
    //returns the usable size for a pointer allocated through the pool
    size_t ActualSizeForUse(void* ptr) const;
   
    //memory allocation and de-allocation
    void*  Alloc(const size_t size);
    size_t Free(void* ptr);        

    //set the memory upper and lower bound for memory enforcement
    void SetMemoryLimit(const size_t memLow,
                        const size_t memHigh,
                        const double tolerance);  
    void MemoryReport();

    void SetDbFile(void* dbFile);
    
    size_t MemoryCurrent() {return m_memoryCurrent;}
    size_t MemoryMax() { return m_maxMemoryUsed;}
    size_t MemoryStagedMax() {return m_stagedMaxMemoryUsed;}

    void ZeroMax() { 
        m_memoryLimitLock->Lock(); 
        m_stagedMaxMemoryUsed = m_memoryCurrent;
        m_memoryLimitLock->Unlock();
    }
    
    void SetReportingMode(bool flag) { m_reportingMode = true; }
  
private:
    //friend class SMABasCamCacheManager handles the creation and
    //destruction of the pools
    SMABasCamPool(const int poolId,
                  const char* poolName,
                  SMABasCamCacheManager* cacheManager);
    ~SMABasCamPool();

    void UpdateMemoryInUse(const bool toIncrease,
                           const size_t size);
    
    int                        m_poolId;
    char                       m_poolName[1024];
    SMABasCamCacheManager*     m_cacheManager;

    size_t                     m_memoryCurrent;
    size_t                     m_maxMemoryUsed;
    size_t                     m_stagedMaxMemoryUsed;

    size_t                     m_memoryLow;
    size_t                     m_memoryHigh;
    double                     m_tolerance;
    SMABasAtiMutex*            m_memoryLimitLock;

    void*                      m_dbFile;
    bool                       m_reportingMode;
};

#endif /* SMABasCamPool_h */
