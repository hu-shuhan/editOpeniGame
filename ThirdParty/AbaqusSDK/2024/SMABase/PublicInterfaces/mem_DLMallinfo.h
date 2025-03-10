//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mem_DLMallinfo_h
#define mem_DLMallinfo_h

#include <sys/types.h>

struct mem_DLMallinfoType {
    size_t m_arena;    // bytes allocated from the system using sbrk
    size_t m_ordblks;  // free bytes
    size_t m_hblkhd;   // bytes allocated from the system using mmap   
    size_t m_usmblks;  // max footprint
    size_t m_uordblks; // bytes in use
    size_t m_fordblks; // free bytes
    size_t m_keepcost; // topsize in bytes
};

struct mem_DLMallinfoType mem_DLMallinfo();

#endif
