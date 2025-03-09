
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#ifndef CATLISTV_DECLARE
/** @nodoc 
 * Macros to declare classes which implement lists of values 
 * of user-defined type X.
 * <br><b>Role:</b> The functionalities supported by the list class
 * can be turned on and off depending on your needs, using #define
 * symbols (if the proper #defined symbol is present, the
 * functionality is added to the class, otherwise it is omitted).
 * There is a trade-off between code-size and functionalities:
 * the more functionalities, the larger the code size.
 * The list of available functionalities is defined in 
 * <tt>CATLISTP_AllFunct.h</tt>
 */
#define CATLISTV_DECLARE( T ) class T; CATLISTV_DECLARE_TN( T, T )
#endif

#ifndef CATCOLLEC_ExportedBy
/** @nodoc 
 * Macro for Windows NT to trigger the export of the list class
 * defined in a DLL.
 */
#define CATCOLLEC_ExportedBy
#endif

#ifndef CATLISTV_DECLARE_TN
/** @nodoc 
 * Macro to declare a list class with a user-defined name.
 */
#define CATLISTV_DECLARE_TN( T, N )                                             \
class CATCOLLEC_ExportedBy CATListVal##N : public CATCollecRoot                 \
{                          /***********/                                        \
  public :                                                                      \
    CATListVal##N ();                                                           \
    CATListVal##N ( int iInitAlloc );                                           \
    CATListVal##N ( int iInitAlloc, int iExtensAlloc );                         \
    CATListVal##N ( const CATListVal##N& iCopy );                               \
    dclCATLISTV_CtorFromArrayVals( T, N )                                       \
    dclCATLISTV_CtorFromArrayPtrs( T, N )                                       \
    ~CATListVal##N ();                                                          \
                                                                                \
    CATListVal##N& operator= ( const CATListVal##N& iCopy );                    \
                                                                                \
    void Append ( const T& iAdd );                                              \
    dclCATLISTV_AppendList( T, N )                                              \
                                                                                \
    dclCATLISTV_InsertAfter( T, N )                                             \
    dclCATLISTV_InsertBefore( T, N )                                            \
    dclCATLISTV_InsertAt( T, N )                                                \
                                                                                \
    int Size () const ;                                                         \
    dclCATLISTV_ReSize( T, N )                                                  \
                                                                                \
    T& operator[] ( int iPos );                                                 \
    const T& operator[] ( int iPos ) const;                                     \
                                                                                \
    dclCATLISTV_Locate( T, N )                                                  \
                                                                                \
    void RemoveAll ( CATCollec::MemoryHandling iMH                              \
                      = CATCollec::ReleaseAllocation );                         \
    dclCATLISTV_RemoveValue( T, N )                                             \
    dclCATLISTV_RemoveList( T, N )                                              \
    dclCATLISTV_RemovePosition( T, N )                                          \
    dclCATLISTV_RemoveDuplicatesCount( T, N )                                   \
    dclCATLISTV_RemoveDuplicatesExtract( T, N )                                 \
                                                                                \
    dclCATLISTV_eqOP( T, N )                                                    \
    dclCATLISTV_neOP( T, N )                                                    \
    dclCATLISTV_gtOP( T, N )                                                    \
    dclCATLISTV_ltOP( T, N )                                                    \
    dclCATLISTV_geOP( T, N )                                                    \
    dclCATLISTV_leOP( T, N )                                                    \
                                                                                \
    dclCATLISTV_Compare( T, N )                                                 \
                                                                                \
    dclCATLISTV_Replace( T, N )                                                 \
    dclCATLISTV_Swap( T, N )                                                    \
    dclCATLISTV_QuickSort( T, N )                                               \
                                                                                \
    dclCATLISTV_ArrayVals( T, N )                                               \
    dclCATLISTV_ArrayPtrs( T, N )                                               \
    dclCATLISTV_NbOccur( T, N )                                                 \
    dclCATLISTV_Intersection( T, N )                                            \
                                                                                \
    dclCATLISTV_ApplyMemberFunct( T, N )                                        \
    dclCATLISTV_ApplyMemberFunctConst( T, N )                                   \
    dclCATLISTV_ApplyGlobalFunct( T, N )                                        \
                                                                                \
  private :                                                                     \
    CATListPV _Lval;                                                            \
};

#endif                // CATLISTV_DECLARE_TN

#ifndef CATLISTV_MinimalFunct
/** @nodoc */
#define CATLISTV_MinimalFunct
/** @nodoc */
#define CATLISTV_eqOP
/** @nodoc */
#define CATLISTV_neOP
#endif

#ifdef CATLISTV_RemoveList
#ifndef CATLISTV_RemoveValue
/** @nodoc */
#define CATLISTV_RemoveValue
#endif
#endif

#ifdef CATLISTV_RemoveValue
#ifndef CATLISTV_Locate
/** @nodoc */
#define CATLISTV_Locate
#endif
#ifndef CATLISTV_RemovePosition
/** @nodoc */
#define CATLISTV_RemovePosition
#endif
#endif

#ifdef CATLISTV_RemoveDuplicatesCount
#ifndef CATLISTV_RemovePosition
/** @nodoc */
#define CATLISTV_RemovePosition
#endif
#endif

#ifdef CATLISTV_RemoveDuplicatesExtract
#ifndef CATLISTV_RemovePosition
/** @nodoc */
#define CATLISTV_RemovePosition
#endif
#endif

#ifdef CATLISTV_eqOP
#ifndef CATLISTV_neOP
/** @nodoc */
#define CATLISTV_neOP
#endif
#endif

#ifdef CATLISTV_ltOP
#ifndef CATLISTV_geOP
/** @nodoc */
#define CATLISTV_geOP
#endif
#endif

#ifdef CATLISTV_gtOP
#ifndef CATLISTV_leOP
/** @nodoc */
#define CATLISTV_leOP
#endif
#endif


#undef  dclCATLISTV_CtorFromArrayVals
#undef  dclCATLISTV_CtorFromArrayPtrs
#undef  dclCATLISTV_AppendList
#undef  dclCATLISTV_InsertAfter
#undef  dclCATLISTV_InsertBefore
#undef  dclCATLISTV_InsertAt
#undef  dclCATLISTV_ReSize
#undef  dclCATLISTV_Locate
#undef  dclCATLISTV_RemoveValue
#undef  dclCATLISTV_RemoveList
#undef  dclCATLISTV_RemovePosition
#undef  dclCATLISTV_RemoveDuplicatesExtract
#undef  dclCATLISTV_RemoveDuplicatesCount
#undef  dclCATLISTV_eqOP
#undef  dclCATLISTV_neOP
#undef  dclCATLISTV_leOP
#undef  dclCATLISTV_geOP
#undef  dclCATLISTV_ltOP
#undef  dclCATLISTV_gtOP
#undef  dclCATLISTV_Compare
#undef  dclCATLISTV_Replace
#undef  dclCATLISTV_Swap
#undef  dclCATLISTV_QuickSort
#undef  dclCATLISTV_ArrayVals
#undef  dclCATLISTV_ArrayPtrs
#undef  dclCATLISTV_NbOccur
#undef  dclCATLISTV_Intersection
#undef  dclCATLISTV_ApplyMemberFunct
#undef  dclCATLISTV_ApplyMemberFunctConst
#undef  dclCATLISTV_ApplyGlobalFunct


#ifndef CATLISTV_CtorFromArrayVals
/** @nodoc
 * Macros to declare optional member functions
 * in the list class.
 */
#define dclCATLISTV_CtorFromArrayVals( T, N )
#else
/** @nodoc */
#define dclCATLISTV_CtorFromArrayVals( T, N )                               \
CATListVal##N ( T* iArray, int iSize );
#endif


#ifndef CATLISTV_CtorFromArrayPtrs
/** @nodoc */
#define dclCATLISTV_CtorFromArrayPtrs( T, N )
#else
/** @nodoc */
#define dclCATLISTV_CtorFromArrayPtrs( T, N )                               \
CATListVal##N ( T** iArray, int iSize );
#endif


#ifndef CATLISTV_AppendList
/** @nodoc */
#define dclCATLISTV_AppendList( T, N )
#else
/** @nodoc */
#define dclCATLISTV_AppendList( T, N )                                      \
void  Append ( const CATListVal##N& iConcat );
#endif


#ifndef CATLISTV_InsertAfter
/** @nodoc */
#define dclCATLISTV_InsertAfter( T, N )
#else
/** @nodoc */
#define dclCATLISTV_InsertAfter( T, N )                                     \
void InsertAfter ( int iPos, const T& iAdd );
#endif


#ifndef CATLISTV_InsertBefore
/** @nodoc */
#define dclCATLISTV_InsertBefore( T, N )
#else
/** @nodoc */
#define dclCATLISTV_InsertBefore( T, N )                                    \
void InsertBefore ( int iPos, const T& iAdd );
#endif


#ifndef CATLISTV_InsertAt
/** @nodoc */
#define dclCATLISTV_InsertAt( T, N )
#else
/** @nodoc */
#define dclCATLISTV_InsertAt( T, N )                                        \
void InsertAt ( int iPos, const T& iAdd );
#endif


#ifndef CATLISTV_ReSize
/** @nodoc */
#define dclCATLISTV_ReSize( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ReSize( T, N )                                          \
void Size ( int iSize, const T* iFiller = 0 );
#endif


#ifndef CATLISTV_Locate
/** @nodoc */
#define dclCATLISTV_Locate( T, N )
#else
/** @nodoc */
#define dclCATLISTV_Locate( T, N )                                          \
int Locate ( const T& iLocate, int iFrom = 1 ) const ;
#endif


#ifndef CATLISTV_RemoveValue
/** @nodoc */
#define dclCATLISTV_RemoveValue( T, N )
#else
/** @nodoc */
#define dclCATLISTV_RemoveValue( T, N )                                     \
int RemoveValue ( const T& iRemove );
#endif


#ifndef CATLISTV_RemoveList
/** @nodoc */
#define dclCATLISTV_RemoveList( T, N )
#else
/** @nodoc */
#define dclCATLISTV_RemoveList( T, N )                                      \
int Remove ( const CATListVal##N& iSubstract );
#endif


#ifndef CATLISTV_RemovePosition
/** @nodoc */
#define dclCATLISTV_RemovePosition( T, N )
#else
/** @nodoc */
#define dclCATLISTV_RemovePosition( T, N )                                  \
void RemovePosition ( int iPos );
#endif


#ifndef CATLISTV_RemoveDuplicatesExtract
/** @nodoc */
#define dclCATLISTV_RemoveDuplicatesExtract( T, N )
#else
/** @nodoc */
#define dclCATLISTV_RemoveDuplicatesExtract( T, N )                         \
void RemoveDuplicates ( CATListVal##N& ioExtract );
#endif


#ifndef CATLISTV_RemoveDuplicatesCount
/** @nodoc */
#define dclCATLISTV_RemoveDuplicatesCount( T, N )
#else
/** @nodoc */
#define dclCATLISTV_RemoveDuplicatesCount( T, N )                           \
int RemoveDuplicates ();
#endif


#ifndef CATLISTV_eqOP
/** @nodoc */
#define dclCATLISTV_eqOP( T, N )
#else
/** @nodoc */
#define dclCATLISTV_eqOP( T, N )                                            \
int operator == ( const CATListVal##N& iLV ) const ;
#endif


#ifndef CATLISTV_neOP
/** @nodoc */
#define dclCATLISTV_neOP( T, N )
#else
/** @nodoc */
#define dclCATLISTV_neOP( T, N )                                            \
int operator != ( const CATListVal##N& iLV ) const ;
#endif


#ifndef CATLISTV_leOP
/** @nodoc */
#define dclCATLISTV_leOP( T, N )
#else
/** @nodoc */
#define dclCATLISTV_leOP( T, N )                                            \
int operator <= ( const CATListVal##N& iLV ) const ;
#endif


#ifndef CATLISTV_geOP
/** @nodoc */
#define dclCATLISTV_geOP( T, N )
#else
/** @nodoc */
#define dclCATLISTV_geOP( T, N )                                            \
int operator >= ( const CATListVal##N& iLV ) const ;
#endif


#ifndef CATLISTV_ltOP
/** @nodoc */
#define dclCATLISTV_ltOP( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ltOP( T, N )                                            \
int operator < ( const CATListVal##N& iLV ) const ;
#endif


#ifndef CATLISTV_gtOP
/** @nodoc */
#define dclCATLISTV_gtOP( T, N )
#else
/** @nodoc */
#define dclCATLISTV_gtOP( T, N )                                            \
int operator > ( const CATListVal##N& iLV ) const ;
#endif


#ifndef CATLISTV_Compare
/** @nodoc */
#define dclCATLISTV_Compare( T, N )
#else
/** @nodoc */
#define dclCATLISTV_Compare( T, N )                                    \
static int Compare ( const CATListVal##N& iLV1,                        \
                     const CATListVal##N& iLV2,                        \
                     int (*iPFCompare)( T*, T* ) );
#endif


#ifndef CATLISTV_Replace
/** @nodoc */
#define dclCATLISTV_Replace( T, N )
#else
/** @nodoc */
#define dclCATLISTV_Replace( T, N )                                   \
void Replace ( int iPos, const T& iReplace );
#endif


#ifndef CATLISTV_Swap
/** @nodoc */
#define dclCATLISTV_Swap( T, N )
#else
/** @nodoc */
#define dclCATLISTV_Swap( T, N )                                      \
void Swap ( int iPos1, int iPos2 );
#endif


#ifndef CATLISTV_QuickSort
/** @nodoc */
#define dclCATLISTV_QuickSort( T, N )
#else
/** @nodoc */
#define dclCATLISTV_QuickSort( T, N )                                 \
void QuickSort ( int (*iPFCompare)( T*, T* ) );
#endif


#ifndef CATLISTV_ArrayVals
/** @nodoc */
#define dclCATLISTV_ArrayVals( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ArrayVals( T, N )                                 \
void Array ( T* ioArray ) const ;
#endif


#ifndef CATLISTV_ArrayPtrs
/** @nodoc */
#define dclCATLISTV_ArrayPtrs( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ArrayPtrs( T, N )                                 \
void Array ( T** ioArray ) const ;
#endif


#ifndef CATLISTV_NbOccur
/** @nodoc */
#define dclCATLISTV_NbOccur( T, N )
#else
/** @nodoc */
#define dclCATLISTV_NbOccur( T, N )                                   \
int NbOccur ( const T& iTest );
#endif


#ifndef CATLISTV_Intersection
/** @nodoc */
#define dclCATLISTV_Intersection( T, N )
#else
/** @nodoc */
#define dclCATLISTV_Intersection( T, N )                                    \
static void Intersection ( const CATListVal##N& iL1,                        \
                           const CATListVal##N& iL2,                        \
                           CATListVal##N& ioResult );
#endif


#ifndef CATLISTV_ApplyMemberFunct
/** @nodoc */
#define dclCATLISTV_ApplyMemberFunct( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ApplyMemberFunct( T, N )                               \
typedef int (T::*PtrMbrFunct) () ;                                         \
int ApplyMemberFunct ( PtrMbrFunct iPF               ,                     \
                       int iFrom  = 1, int  iTo  = 0 ,                     \
                       T** iPLast = 0, int* iPRC = 0 ) const ;

#endif


#ifndef CATLISTV_ApplyMemberFunctConst
/** @nodoc */
#define dclCATLISTV_ApplyMemberFunctConst( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ApplyMemberFunctConst( T, N )                        \
typedef int (T::*PtrMbrFunctConst) () const ;                            \
int ApplyMemberFunctConst ( PtrMbrFunctConst iPF          ,              \
                            int iFrom  = 1, int  iTo  = 0 ,              \
                            T** iPLast = 0, int* iPRC = 0 ) const ;
#endif


#ifndef CATLISTV_ApplyGlobalFunct
/** @nodoc */
#define dclCATLISTV_ApplyGlobalFunct( T, N )
#else
/** @nodoc */
#define dclCATLISTV_ApplyGlobalFunct( T, N )                            \
typedef int (*PtrGlbFunct) (T*) ;                                       \
int ApplyGlobalFunct ( PtrGlbFunct iPF               ,                  \
                       int iFrom  = 1, int  iTo  = 0 ,                  \
                       T** iPLast = 0, int* iPRC = 0 ) const ;
#endif


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CATListPV.h"

