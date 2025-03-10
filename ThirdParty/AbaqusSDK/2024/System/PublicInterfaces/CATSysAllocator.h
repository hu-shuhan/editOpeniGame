#ifndef CATSysAllocator_H
#define CATSysAllocator_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
//=========================================================
// Copyright DASSAULT SYSTEMES 2001
//=========================================================



#include <stdlib.h>
#include "DSYExport.h"

#if defined(__CATSysAllocator)
# define ExportedByCATSysAllocator  DSYExport
#else // __CATSysAllocator
# define ExportedByCATSysAllocator  DSYImport
#endif  // __CATSysAllocator


//================================================
/**
 * Define the allocator policy for use with CATNew
 */
//================================================
#define CATNew_DefaultPolicy   (void *) 0x0000000L
#define CATNew_PermanentObject (void *) 0x0000001L
#define CATNew_FlushableObject (void *) 0x0000002L
#define CATNew_VolatileObject  (void *) 0x0000004L
#define CATNew_ProximityObject (void *) 0x0000008L
#define CATNew_Reservation     (void *) 0x0000010L
#define CATNew_ThreadedObject  (void *) 0x0000020L

//================================================
/**
  * Defines the possible flags to use in 
  * CATSysCreatePool
 **/
//================================================
/* flag:CATPool_Permanent  : the objects in the pool have long or undefined */
/*  life-span                                                               */
#define CATPool_Permanent       0x00000000L
/* flag:CATPool_ThreadSafe : the pool management is thread-safe             */
#define CATPool_ThreadSafe      0x00000008L
/* flag:CATPool_Flushable  :  the pool object may be flushed globally       */
#define CATPool_Flushable       0x00000020L
/* flag:CATPool_Volatile   : the pool objects have very short life-span     */
#define CATPool_Volatile        0x00000040L
/* flag:CATPool_SingleSize : the pool objects  have all the same size       */
#define CATPool_SingleSize      0x00000080L
/* flag:CATPool_Unresizable : the pool can not be resized or see its size   */
/*                            increase                                      */
#define CATPool_Unresizable     0x00000200L
/* flag:CATPool_ConstantSPSize : all the subpool will have the same size    */
/*                               in other terms the pool size increase      */
/*                               linearly                                   */
#define CATPool_ConstantSPSize  0x00000100L
/* flag:CATPool_UsePreferedSP : reserved do not use                         */
#define CATPool_UsePreferedSP  0x00200000L
/* flag:CATPool_Paged         : the pool is paged pool with no              */ 
/*                              per-allocation overhead                     */
#define CATPool_Paged          0x00400000L


/***
 * special optimization
***/
#define CATPool_OptimizedSPRun  0x00100000L

//================================================
/**
  * Defines the possible flags to use in  
  * CATSysFlushPool
 **/
//================================================
/* ask the pool to be freed from memory */
#define CATPool_FreeMemory        0x0000001L
/* ask the destuctors to be called      */
#define CATPool_CallDestructors   0x0000002L
/* free the pool memory only if the pool is empty */
#define CATPool_PoolDestructionIfEmpty  0x0000004L
//================================================
/**
  * Defines the possible flags to use in  
  * CATSysOptimizePool
 **/
//================================================
/* default optimization                       */
#define CATPool_OptDefault        0xffffffffL
/* optimize size                              */
#define CATPool_OptForSize        0x000000ffL
/* optimize by suppressing free subpools      */
#define CATPool_OptFreeEmptySP    0x00000001L
/* optimize performances                      */
#define CATPool_OptForPerfo       0x0000ff00L
/* optimize by unifying holes when possibles  */
#define CATPool_OptUnifyHoles     0x00000100L

#define CATPool_Default         0x00000000L

#ifndef CATUseOnlyDefaultNew
//===============================================================
  /** 
   * new and delete specialized macros
   */
//===============================================================
/** 
  * macros for specifying  allocator 
  */
#define CATSysNew(alo)   new (alo) 
#define CATSysDelete(alo) delete 

/** 
  * macros for reservation and proximity  allocator 
  */
#define CATSysReservationNew(alo,rsz)   new (alo,CATNew_Reservation,rsz) 
#define CATSysProximityNew(addr)   new (CATNew_ProximityObject,addr) 
#define CATSysDeleteProximity(addr) delete 

/** 
  * macros for flushable pool allocation
  */
#define CATSysFlushableNew(poolAddr)   new (CATNew_FlushableObject,poolAddr) 
#define CATSysDeleteFlushable(addr) delete 


//===============================================================
/**
 * This macro permits to declare object as using the new override
**/
//===============================================================
#ifdef _WINDOWS_SOURCE

#define CATDeclareNewOverride                               \
       public:                                              \
       inline void operator delete(void *iAddr)             \
       {                                                    \
          CATSpecializedDelete( iAddr) ;                    \
                                                            \
       };                                                   \
       inline void operator delete(void *iAddr,void *iAllocType, \
                      void *iRefAddr,int iReservedSize) \
       {                                                    \
          CATSpecializedDelete( iAddr) ;                    \
                                                            \
       };                                                   \
       inline void *operator new(size_t iSize,void *iAllocType=NULL, \
                      void *iAddr=NULL,int iReservedSize=0) \
       {                                                    \
          return  CATSpecializedNew(iSize,iAllocType,       \
                                     iAddr,iReservedSize);  \
       };
#else
#define CATDeclareNewOverride                               \
       public:                                              \
       inline void operator delete(void *iAddr)             \
       {                                                    \
          CATSpecializedDelete( iAddr) ;                    \
                                                            \
       };                                                   \
       inline void *operator new(size_t iSize,void *iAllocType=NULL, \
                      void *iAddr=NULL,int iReservedSize=0) \
       {                                                    \
          return  CATSpecializedNew(iSize,iAllocType,       \
                                     iAddr,iReservedSize);  \
       };
#endif

//===============================================================
/**
 * This macro is similar to CATDeclareNewOverride but permits 
 * to specify the default allocator policy
**/
//===============================================================
#define CATDeclareDefaultNewOverride(iDefaultAllocator )    \
       public:                                              \
       inline void operator delete(void *iAddr)             \
       {                                                    \
          CATSpecializedDelete( iAddr) ;                    \
                                                            \
       };                                                   \
       inline void *operator new(size_t iSize,              \
                      void *iAllocType=iDefaultAllocator,    \
                      void *iAddr=NULL,int iReservedSize=0) \
       {                                                    \
          return  CATSpecializedNew(iSize,iAllocType,       \
                                     iAddr,iReservedSize);  \
       };                                                   \



//===============================================================
/**
 * This macro permits to declare object as using the paged new
 *  override   ( no placement operator provided  - no inlining)
 *  By default use CATDeclarePagedNewOverride                
 *  and            CATImplementPagedNewOverride                
 **/
//===============================================================

#define CATDeclarePagedNewOverride(iClass )               \
   void *operator new(  size_t iSize);                    \
   void operator delete(void *iAddr );                    \
   private:                                               \
   static void *S_##iClass##Pool;                         \


#define CATImplementPagedNewOverride(iClass,iType )          \
void *iClass::S_##iClass##Pool=NULL;                     \
void *iClass::operator new(  size_t iSize)               \
{                                                            \
   if (S_##iClass##Pool==NULL)                               \
   {                                                         \
     S_##iClass##Pool=CATSysCreatePool(iType|CATPool_Paged,  \
                                                 0,#iClass); \
   }                                                         \
   if (S_##iClass##Pool)                                     \
   {                                                         \
      return  CATSpecializedNew(iSize,S_##iClass##Pool,      \
                                     NULL ,0,CATPool_Paged); \
   }                                                         \
   return NULL;                                              \
}                                                            \
void iClass::operator delete(  void *iObject)              \
{                                                            \
    if (S_##iClass##Pool==NULL)   return;                    \
    CATSpecializedDelete(iObject,S_##iClass##Pool,CATPool_Paged);\
}                                                            \


/**
 *
 *  CATDeclarePagedNewOverrideAdv is provided only for
 *  advanced users who want to provide their own specific 
 *  implementation  of new and delete 
**/
#define CATDeclarePagedNewOverrideAdv( )                  \
   public:                                                \
   void *operator new(  size_t iSize);                    \
   void operator delete(void *iAddr );                    \
   private:                                               \

/**
 *
 *  CATDeclarePagedNewOverridePlc is provided only for
 *  advanced users who want to benefit of placement 
 *  allocators 
**/
#ifdef _WINDOWS_SOURCE

#define CATDeclarePagedNewOverridePlc( )       \
       public:                         \
       inline void operator delete(void *iAddr)             \
       {                                                    \
          CATSpecializedDelete( iAddr, NULL,CATPool_Paged); \
                                                            \
       };                                                   \
       inline void operator delete(void *iAddr,void *iPool) \
       {                                                    \
          CATSpecializedDelete( iAddr,iPool,CATPool_Paged); \
                                                            \
       };                                                   \
                                                            \
       inline void *operator new(size_t iSize,              \
                      void *iAllocType,                     \
                      void *iAddr=NULL,int iReservedSize=0) \
       {                                                    \
          return  CATSpecializedNew(iSize,iAllocType,       \
                         iAddr,iReservedSize,CATPool_Paged);\
       };                                                   \

#else
#define CATDeclarePagedNewOverridePlc( )                    \
       public:                                              \
       inline void operator delete(void *iAddr)             \
       {                                                    \
          CATSpecializedDelete( iAddr, NULL,CATPool_Paged); \
                                                            \
       };                                                   \
       inline void *operator new(size_t iSize,              \
                      void *iAllocType,                     \
                      void *iAddr=NULL,int iReservedSize=0) \
       {                                                    \
          return  CATSpecializedNew(iSize,iAllocType,       \
                         iAddr,iReservedSize,CATPool_Paged);\
       };                                                   \

#endif

//=====================================================================================
// POOLS SERVICES
//=====================================================================================
/*** 
 *   
 *-------------------------------------------------------------------------------------
 *  CATSysCreatePool permits to create a memory pool
 *   
 *-------------------------------------------------------------------------------------
 * The following flags may be added to specify the pool behaviour or information
 * useful to optimize pool management 
 *      CATPool_Permanent  : the objects in the pool have long or undefined life-span *
 *      CATPool_ThreadSafe : the pool management is thread-safe                       *
 *      CATPool_Flushable  :  the pool object may be flushed globally                 *
 *      CATPool_Volatile   : the pool objects have very short life-span               *
 *      CATPool_SingleSize : the pool objects  have all the same size                 *
 *      CATPool_Unresizable : the pool can not be resized or see its size increase    *
 *      CATPool_ConstantSPSize : all the subpool will have the same size              *
 *-------------------------------------------------------------------------------------
 * RETURN CODE
 * returns the pools ID as (void *) or NULL         
 * 
***/
ExportedByCATSysAllocator void * CATSysCreatePool( int iTypePool, size_t iInitialSize  , const char *iPoolName=NULL);


/**
 *   
 *-------------------------------------------------------------------------------------
 *  CATSysFlushPool is used to ask for the suppression of a pool
 *   
 *-------------------------------------------------------------------------------------
 * The following flags may be added to specify the pool flush behaviour           
 *   
 *  CATPool_FreeMemory             : ask  the pool to be freed from memory 
 *  CATPool_CallDestructors        : asks the destuctors to be called      (unsupported today)
 *  CATPool_PoolDestructionIfEmpty : free the pool memory only if the pool is empty 
 *-------------------------------------------------------------------------------------
 * RETURN CODE
 * returns 0 if something was done
 * returns -1 if nothing was done
 */
ExportedByCATSysAllocator int CATSysFlushPool( void *iPool, int iFlags=0 );

/**
 *   
 *-------------------------------------------------------------------------------------
 *  CATSysOptimizePool is used to optimize the performance and/or the size of a pool
 *   
 *-------------------------------------------------------------------------------------
 * The following flags may be added to specify the pool optimize behaviour           
 *   
 *      CATPool_Default        : default optimizations
 *      CATPool_OptForSize     : optimize size 
 *      CATPool_OptFreeEmptySP : optimize by suppressing free subpools
 *      CATPool_OptForPerfo    : optimize performances                    
 *      CATPool_OptUnifyHoles  : optimize by unifying holes when possibles
 *-------------------------------------------------------------------------------------
 * RETURN CODE
 * returns 0 if something was done
 * returns -1 if nothing was done
 *
 *   
 *-------------------------------------------------------------------------------------
 * The following flags may be added to specify the pool flush behaviour           
 *   
 *  CATPool_FreeMemory             : ask  the pool to be freed from memory 
 *  CATPool_CallDestructors        : asks the destuctors to be called      (unsupported today)
 *  CATPool_PoolDestructionIfEmpty : free the pool memory only if the pool is empty 
 *-------------------------------------------------------------------------------------
 * RETURN CODE
 * returns 0 if something was done
 * returns -1 if nothing was done
 */
ExportedByCATSysAllocator int CATSysOptimizePool( void *iPool, int iFlags=0 );




//=====================================================================================
// Allocation services
//=====================================================================================
/**
 *   
 *  CATSpecializeNew : allocate an object
 *-------------------------------------------------------------------------------------
 */

ExportedByCATSysAllocator void * CATSpecializedNew(size_t s, void *iAllocType=NULL,void *ProximityAddr=NULL,int iReserve=0,int iType=CATPool_Default);
/**
 *   
 *  CATSpecializeDelete : remove  an object 
 *-------------------------------------------------------------------------------------
 */
ExportedByCATSysAllocator void CATSpecializedDelete(void *iToDelete, void *Allocator=0, int iType=CATPool_Default  );
/**
 *   
 *  CATSpecializeResize : resize  an allocation
 *-------------------------------------------------------------------------------------
 */
ExportedByCATSysAllocator void *CATSpecializedResize(void *iToResize, size_t iNewSize, void *Allocator=0);
/**
 *   
 *  CATSysGetAllocatorFromAddress : obtains the owning pool of an object allocated
 *                                  with CATSpecializeNew 
 *-------------------------------------------------------------------------------------
 */
ExportedByCATSysAllocator void *CATSysGetAllocatorFromAddress(void * iAddress) ;

#else
/** 
  * macros for specifying  allocator 
  */
#define CATSysNew(alo)   new 
#define CATSysDelete(alo) delete 

/** 
  * macros for reservation and proximity  allocator 
  */
#define CATSysReservationNew(alo,rsz)   new 
#define CATSysProximityNew(addr)   new 
#define CATSysDeleteProximity(addr) delete 

/** 
  * macros for flushable pool allocation
  */
#define CATSysFlushableNew(poolAddr)   new

#define CATDeclareNewOverride                               
#define CATDeclareDefaultNewOverride(iDefaultAllocator )    

/**
 * utilities
 **/
static inline void * CATSysCreatePool( int iTypePool, size_t iInitialSize )
{
  return NULL;
}
static inline int  CATSysFlushPool( void * iPool, int iFlags=0 )
{
  return -1 ;
}
static inline int  CATSysOptimizePool( void * iPool, int iFlags=0 )
{
  return -1;
}
static inline void * CATSpecializedNew(size_t s, void *iAllocType=NULL,void *ProximityAddr=NULL,int iReserve=0)
{
  return NULL;
}
static inline void CATSpecializedDelete(void *iToDelete, void *Allocator=0)
{
  return ;
}
static inline void * CATSysGetAllocatorFromAddress(void * iAddress) 
{
  return NULL;
}
static inline void * CATSysGetAllocatorObject(void * iAllocType)
{
  return NULL;
}
static inline void *CATSpecializedResize(void *iToResize, size_t iNewSize, void *Allocator=0);
{
  return NULL;
}
#endif

#endif
