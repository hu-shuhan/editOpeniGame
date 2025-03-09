//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mem_C_Allocation_h
#define mem_C_Allocation_h

#include <sys/types.h>
#include <stddef.h>

//Begin local includes

#include <mem_DLMallinfo.h>

#if defined __cplusplus
extern "C" {
#endif

void* mem_Malloc(size_t size);

  /* Next is a version of mem_Malloc, which, when a memory allocation fails,will transfer control 
   * to a registered handler routine. Registered hanlders for CAE, Standard, Explicit, etc. will 
   * convey the appropriate "out-of-memory" message to the user, log the exception, and shut down 
   * the program gracefully. If no handler for out-of-memory conditions is registered, the function 
   * will return 0. */

void* mem_Malloc_Handled(size_t size);

void  mem_Free(void* ptr);

void* mem_Realloc(void* ptr, size_t size);

void* mem_Calloc(size_t nelem, size_t elsize);

void* mem_Malign(size_t size, size_t alignment); // These two functions must be used as a matched pair.
void  mem_MalignFree(void* ptr);

void  mem_Release(void* ptr);

void  mem_Acquire(void* ptr);

int   mem_Trim(size_t pad);

int   mem_Mallopt(int param_number, size_t value);

size_t mem_Footprint();

size_t mem_MaxFootprint();

size_t mem_MemoryLimit();

size_t mem_FragmentationInfo();

int mem_ErrorMemoryLimit();

int mem_usingDL(); 

size_t mem_FootprintFromResources();

void mem_ReleaseMemoryLock();

#define mem_USE_LOCKS (-4)
#define mem_MEMORY_LIMIT (-5)

#if defined __cplusplus
}
#endif

#endif /* mem_C_Allocation_h */
