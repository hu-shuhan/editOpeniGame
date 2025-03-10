#ifndef CATVecCxxSort_H
#define CATVecCxxSort_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
/* COPYRIGHT DASSAULT SYSTEMES 2000 */

#if !defined(CATCollec_CXX_SORT)
/** @nodoc */
#define CATCollec_CXX_SORT 1
#endif


#include "CO0LSTPV.h"   // ExportedByCO0LSTPV
#include <algorithm>    // std::sort
#include <stdlib.h>     // qsort_s/qsort_r/qsort...
#include <string.h>     // memcpy...


#if (CATCollec_CXX_SORT == 0)
    #if defined(_MSC_VER)
        /** @nodoc */
        #define dsy_qsort_safe_decl(ptr,count,size,ctx,e1,e2) \
            qsort_s(ptr, count, size, [](void *ctx, const void *e1, const void *e2) -> int
    #elif defined(_LINUX_SOURCE)
        /** @nodoc */
        #define dsy_qsort_safe_decl(ptr,count,size,ctx,e1,e2) \
            qsort_r(ptr, count, size, [](const void *e1, const void *e2, void *ctx) -> int
    #else
        #undef CATCollec_CXX_SORT
        /** @nodoc */
        #define CATCollec_CXX_SORT  1
    #endif
#endif


#if (CATCollec_CXX_SORT != 0)
    

namespace dsy
{
  namespace internal
  {
    ExportedByCO0LSTPV bool CATCollecSortCheck(bool const*);
    
    /** @nodoc */
    template <typename _Ty>
    void CATVecCxxSort(_Ty* const ptr, size_t const count, int (*iPFComp)(_Ty const*, _Ty const*))
    {   
        if (!CATCollecSortCheck(nullptr)) {
            // qsort is not safe to use! If 'T' is not a "TriviallyCopyable" type, the behavior is undefined.
            std::sort(ptr, (ptr + count), [iPFComp](_Ty const& a, _Ty const& b) -> bool {
                return (*iPFComp)(&a, &b) < 0;   // a < b
            });
            return;
        }
        
        if(count <= 1)
            return;  // Nothing to sort
        bool bResultComp = false;
        constexpr size_t size = sizeof(_Ty);
        void* ptr2 = malloc(count * size);
        if (ptr && ptr2)
        {
            memcpy(ptr2, ptr, count * size);

            // Compare the source list using qsort
            qsort(ptr, count, size, reinterpret_cast<int(*)(void const*, void const*)>(iPFComp));

            // Compare the source list using C++ sort
            {
                struct CItemWrap {  // trivially movable
                    alignas(_Ty) unsigned char data[size];
                };
                CItemWrap * const ptr3 = reinterpret_cast<CItemWrap*>(ptr2);
                std::sort(ptr3, (ptr3 + count), [iPFComp](CItemWrap const& a, CItemWrap const& b) -> bool {
                    return (*iPFComp)(reinterpret_cast<_Ty const*>(&a), reinterpret_cast<_Ty const*>(&b)) < 0;   // a < b
                });
            }
            
            // Compare the results of both algorithms (at memory level)
            bResultComp = !memcmp(ptr, ptr2, count * size);
        }
        
        CATCollecSortCheck(&bResultComp);

        // Cleanup
        free(ptr2);
    }

    /** @nodoc */
    template <typename _Ty>
    void CATVecCxxSort(_Ty* const ptr, size_t const count, int (*iPFComp)(_Ty*, _Ty*)) {
        CATVecCxxSort(ptr, count, reinterpret_cast<int(*)(_Ty const*, _Ty const*)>(iPFComp));
    }
  }  // namespace internal
}  // namespace dsy

/** @nodoc */
#define CATVecCxxSort(ptr, count, iPFComp)  dsy::internal::CATVecCxxSort(ptr, count, iPFComp)


#else  // NOT CATCollec_CXX_SORT


namespace dsy
{
  namespace internal
  {
    /** @nodoc */
    template <typename _Ty>
    void CATVecCxxSort_Compat(_Ty* const ptr, size_t const count, int (*iPFComp)(_Ty*, _Ty*))
    {
        dsy_qsort_safe_decl(ptr,count,sizeof(_Ty),ctx,e1,e2)
        {
            auto pComparer = reinterpret_cast<decltype(iPFComp)>(ctx);
            return (*pComparer)(const_cast<_Ty*>(reinterpret_cast<_Ty const*>(e1)),
                                const_cast<_Ty*>(reinterpret_cast<_Ty const*>(e2)));
        }, reinterpret_cast<void*>(iPFComp));
    }
  }  // namespace internal
}  // namespace dsy

/** @nodoc */
#define CATVecCxxSort(ptr, count, iPFComp)  dsy::internal::CATVecCxxSort_Compat(ptr, count, iPFComp)

#endif  // NOT CATCollec_CXX_SORT

#endif
