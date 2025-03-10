//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2013
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef mem_Definitions_h
#define mem_Definitions_h

#include <new>

// Begin local includes

// End local includes

#if defined HP
#   define HKS_BAD_ALLOC        bad_alloc
#   define HKS_NOTHROW          nothrow_t
#   define HKS_NEW_HANDLER_TYPE new_handler
#   define HKS_NEW_HANDLER_CALL set_new_handler

#   define HKS_NEW_HANDLER_RETTYPE  void
#   define HKS_NEW_HANDLER_ARGS
#   define HKS_NEW_HANDLER_RETVAL
#else
#   define HKS_BAD_ALLOC        std::bad_alloc
#   define HKS_NOTHROW          std::nothrow_t
#   define HKS_NEW_HANDLER_TYPE std::new_handler
#   define HKS_NEW_HANDLER_CALL std::set_new_handler

#   define HKS_NEW_HANDLER_RETTYPE  void
#   define HKS_NEW_HANDLER_ARGS
#   define HKS_NEW_HANDLER_RETVAL
#endif


#endif // mem_Definitions_h
