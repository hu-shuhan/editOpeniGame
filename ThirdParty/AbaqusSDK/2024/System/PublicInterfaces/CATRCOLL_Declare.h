////////////////////////////////////////////////////////////////////////////////

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

//
// FrameWork COLLECTIONS        --        Dassault Systems (JAN96)
// 
// Macros :  1- CATRCOLL_DECLARE( T )
// ======    
//           2- CATRCOLL_DECLARE_TN( T, N )
// 	    
//           where T stands for a type name
//             and N stands for a name appended to "CATRawColl"
//           (in the first case T and N are the same)
// 
// 
// Purpose : Generate body of a non-template class CATRawColl"N"
// =======   (raw collection of instances of the type "T")
// 
// Usage :   // 1- To generate the body of a class CATRawCollMyType,
// =====     //    create a file CATRawCollMyType.h containing those lines :
//           #ifndef        CATRawCollMyType_h
//           #define        CATRawCollMyType_h
//           //
//           // clean previous requests for extra functionalities :
//           //
//           #include  <CATRCOLL_Clean.h>
//           //
//           // define symbols for extra functionalities :
//           //
//           #define        CATRCOLL_Append
//           #define        CATRCOLL_Remove
//           #define        CATRCOLL_...
//           //
//           // define macros CATRCOLL_DECLARE :
//           //
//           #include  <CATRCOLL_Declare.h>
//           //
//           // DECLARE functions for class CATRawCollCATMyType :
//           // (generate body of class)
//           //
//           CATRCOLL_DECLARE( CATMyType )
//           //
//           // DECLARE functions for class CATRawCollConstMyType :
//           // (generate body of class)
//           //
//           CATRCOLL_DECLARE_TN( const CATMyType, ConstMyType )
//           //
//           #endif        // CATRawCollCATMyType_h
// 	    
//           // 2- Consult documentation "COLLECTIONS" (chapter Raw Collections)
//           //    to know the available functionalities
// 
// Authors : Philippe BAUCHER
// =======
//
////////////////////////////////////////////////////////////////////////////////
//
//        1- To produce declaration of functions for class CATRawColl...
//           ===========================================================
//
#ifndef CATRCOLL_DECLARE
#define CATRCOLL_DECLARE( T )  CATRCOLL_DECLARE_TN( T, T )
#endif
//
// Macro CATCOLLEC_ExportedBy is used for WindowsNT import/export DDL specifications
// 1- it should be defined to ExportedByXX0MODUL
// 2- but if not defined, it should not be kept as indesirable text :
#ifndef CATCOLLEC_ExportedBy
#define CATCOLLEC_ExportedBy
#endif

#ifndef CATRCOLL_DECLARE_TN
#define CATRCOLL_DECLARE_TN( T, N )                                         \
class CATCOLLEC_ExportedBy CATRawColl##N : public CATCollecRoot             \
{                          /***********/                                    \
  public :                                                                  \
    /* CDTORS */                                                            \
    CATRawColl##N ( int iInitAlloc = 0 );                                   \
    CATRawColl##N ( const CATRawColl##N& iCopy );                           \
    dclCATRCOLL_CtorFromArray( T, N )                                       \
    virtual ~CATRawColl##N ();                                              \
                                                                            \
    /* COPY OPERATOR */                                                     \
    CATRawColl##N& operator= ( const CATRawColl##N& iCopy );                \
                                                                            \
    /* APPEND ELEMENT(S) */                                                 \
    dclCATRCOLL_Append( T, N )                                              \
    dclCATRCOLL_AppendList( T, N )                                          \
                                                                            \
    /* INSERT ELEMENT */                                                    \
    dclCATRCOLL_InsertAt( T, N )                                            \
                                                                            \
    /* GET/SET NUMBER OF ELEMENTS */                                        \
    inline int Size () const  { return _Size; }                             \
    dclCATRCOLL_ReSize( T, N )                                              \
    dclCATRCOLL_ReSizeFill( T, N )                                          \
                                                                            \
    /* INDEXATION OPERATORS */                                              \
    T& operator[] ( int iPos );                                             \
    T  operator[] ( int iPos ) const ;                                      \
                                                                            \
    /* LOCALIZE ELEMENT */                                                  \
    dclCATRCOLL_Locate( T, N )                                              \
                                                                            \
    /* REMOVE ELEMENT(S) */                                                 \
    dclCATRCOLL_RemoveValue( T, N )                                         \
    dclCATRCOLL_RemoveList( T, N )                                          \
    dclCATRCOLL_RemovePosition( T, N )                                      \
    dclCATRCOLL_RemoveAll( T, N )                                           \
    dclCATRCOLL_RemoveNulls( T, N )                                         \
    dclCATRCOLL_RemoveDuplicates( T, N )                                    \
                                                                            \
    /* COMPARISON OPERATORS */                                              \
    inline int operator == ( const CATRawColl##N& iRC ) const               \
                     { return ( *this != iRC ? 0 : 1 ); }                   \
    int operator != ( const CATRawColl##N& iRC ) const ;                    \
                                                                            \
    /* COMPARE STATIC FUNCTION */                                           \
    dclCATRCOLL_PtrCompare( T, N )                                          \
    dclCATRCOLL_ValCompare( T, N )                                          \
                                                                            \
    /* CHANGE/MOVE ELEMENT(S) */                                            \
    dclCATRCOLL_Swap( T, N )                                                \
    dclCATRCOLL_PtrQuickSort( T, N )                                        \
    dclCATRCOLL_ValQuickSort( T, N )                                        \
                                                                            \
    /* MISCELLANEOUS */                                                     \
    dclCATRCOLL_FillArray( T, N )                                           \
    dclCATRCOLL_NbOccur( T, N )                                             \
    dclCATRCOLL_Intersection( T, N )                                        \
                                                                            \
  protected :                                                               \
    /* STORAGE */                                                           \
    inline void GetStorage ( int iNbElem, T*& oBlock )                      \
                    { oBlock = new T[ iNbElem ] ; }                         \
    void FreeStorage ( T*& ioBlock );                                       \
    inline T* Storage () { return _Block; }                                 \
                                                                            \
    /* DATA MEMBERS */                                                      \
    int _Size ;                                                             \
    int _MaxSize ;                                                          \
    T*  _Block ;                                                            \
};

#endif                // CATRCOLL_DECLARE_TN

//
// 2- To define required CATRCOLL_... symbols
//    =======================================
//
// To solve prerequisites
#if defined(CATRCOLL_PtrCompare) || defined(CATRCOLL_PtrQuickSort)
#define CATRCOLL_GenericCompare
#endif

// To ease loop programming
#include "CATRCOLL_Misc.h"

#undef  dclCATRCOLL_CtorFromArray
#undef  dclCATRCOLL_Append
#undef  dclCATRCOLL_AppendList
#undef  dclCATRCOLL_InsertAt
#undef  dclCATRCOLL_ReSize
#undef  dclCATRCOLL_ReSizeFill
#undef  dclCATRCOLL_Locate
#undef  dclCATRCOLL_RemoveValue
#undef  dclCATRCOLL_RemoveList
#undef  dclCATRCOLL_RemovePosition
#undef  dclCATRCOLL_RemoveAll
#undef  dclCATRCOLL_RemoveNulls
#undef  dclCATRCOLL_RemoveDuplicates
#undef  dclCATRCOLL_PtrCompare
#undef  dclCATRCOLL_ValCompare
#undef  dclCATRCOLL_Swap
#undef  dclCATRCOLL_PtrQuickSort
#undef  dclCATRCOLL_ValQuickSort
#undef  dclCATRCOLL_FillArray
#undef  dclCATRCOLL_NbOccur
#undef  dclCATRCOLL_Intersection
//
// 3- To prepare (non-)declaration of functions or class CATRawColl...
//    ================================================================
//
// ==========
// 3-1 CDTORS
// ==========
#ifndef CATRCOLL_CtorFromArray
#define dclCATRCOLL_CtorFromArray( T, N )
#else
#define dclCATRCOLL_CtorFromArray( T, N )                             \
CATRawColl##N ( T* iArray, int iSize );
#endif

// =====================
// 3-2 APPEND ELEMENT(S)
// =====================
#ifndef CATRCOLL_Append
#define dclCATRCOLL_Append( T, N )
#else
#define dclCATRCOLL_Append( T, N )                                                \
void Append ( T iAdd );
#endif

#ifndef CATRCOLL_AppendList
#define dclCATRCOLL_AppendList( T, N )
#else
#define dclCATRCOLL_AppendList( T, N )                                                \
void Append ( const CATRawColl##N& iConcat );
#endif

// ==================
// 3-3 INSERT ELEMENT
// ==================
#ifndef CATRCOLL_InsertAt
#define dclCATRCOLL_InsertAt( T, N )
#else	
#define dclCATRCOLL_InsertAt( T, N )                                                \
void InsertAt ( int iPos, T iAdd );
#endif

// ==============================
// 3-4 GET/SET NUMBER OF ELEMENTS
// ==============================
#ifndef CATRCOLL_ReSize
#define dclCATRCOLL_ReSize( T, N )
#else	
#define dclCATRCOLL_ReSize( T, N )                                                \
void Size ( int iSize );
#endif

#ifndef CATRCOLL_ReSizeFill
#define dclCATRCOLL_ReSizeFill( T, N )
#else	
#define dclCATRCOLL_ReSizeFill( T, N )                                                \
void Size ( int iSize, T iFiller );
#endif

// ====================
// 3-5 LOCALIZE ELEMENT
// ====================
#ifndef CATRCOLL_Locate
#define dclCATRCOLL_Locate( T, N )
#else	
#define dclCATRCOLL_Locate( T, N )                                                \
int Locate ( T iLocate, int iFrom = 1 ) const ;
#endif

// =====================
// 3-6 REMOVE ELEMENT(S)
// =====================
#ifndef CATRCOLL_RemoveValue
#define dclCATRCOLL_RemoveValue( T, N )
#else  
#define dclCATRCOLL_RemoveValue( T, N )                                                \
int RemoveValue ( T iRemove );
#endif

#ifndef CATRCOLL_RemoveList
#define dclCATRCOLL_RemoveList( T, N )
#else	
#define dclCATRCOLL_RemoveList( T, N )                                                \
int Remove ( const CATRawColl##N& iSubstract );
#endif

#ifndef CATRCOLL_RemovePosition
#define dclCATRCOLL_RemovePosition( T, N )
#else	
#define dclCATRCOLL_RemovePosition( T, N )                                        \
void RemovePosition ( int iPos );
#endif

#ifndef CATRCOLL_RemoveAll
#define dclCATRCOLL_RemoveAll( T, N )
#else	
#define dclCATRCOLL_RemoveAll( T, N )                                                \
void RemoveAll ( CATCollec::MemoryHandling iMH = CATCollec::ReleaseAllocation );
#endif

#ifndef CATRCOLL_RemoveNulls
#define dclCATRCOLL_RemoveNulls( T, N )
#else  
#define dclCATRCOLL_RemoveNulls( T, N )                                                \
int RemoveNulls ();
#endif

#if !defined(CATRCOLL_RemoveDuplicatesCount) && !defined(CATRCOLL_RemoveDuplicates)
#define dclCATRCOLL_RemoveDuplicates( T, N )
#else	
#define dclCATRCOLL_RemoveDuplicates( T, N )                                \
int RemoveDuplicates ( CATRawColl##N* ioExtract = NULL );
#endif

// ============================
// 3-7  COMPARE STATIC FUNCTION
// ============================
#ifndef CATRCOLL_PtrCompare
#define dclCATRCOLL_PtrCompare( T, N )
#else	
#define dclCATRCOLL_PtrCompare( T, N )                                         \
static int Compare ( const CATRawColl##N& iRC1,                                \
                     const CATRawColl##N& iRC2,                                \
                     int (*iPFCompare) ( const void*, const void* ) );
#endif

#ifndef CATRCOLL_ValCompare
#define dclCATRCOLL_ValCompare( T, N )
#else	
#define dclCATRCOLL_ValCompare( T, N )                                         \
static int Compare ( const CATRawColl##N& iRC1, const CATRawColl##N& iRC2 );
#endif

// ===========================
// 3-8  CHANGE/MOVE ELEMENT(S)
// ===========================
#ifndef CATRCOLL_Swap
#define dclCATRCOLL_Swap( T, N )
#else	
#define dclCATRCOLL_Swap( T, N )                                                \
void Swap ( int iPos1, int iPos2 );
#endif

#ifndef CATRCOLL_PtrQuickSort
#define dclCATRCOLL_PtrQuickSort( T, N )
#else	
#define dclCATRCOLL_PtrQuickSort( T, N )                                        \
void QuickSort ( int (*iPFCompare) ( const void*, const void* ) );
#endif

#ifndef CATRCOLL_ValQuickSort
#define dclCATRCOLL_ValQuickSort( T, N )
#else	
#define dclCATRCOLL_ValQuickSort( T, N )                                        \
void QuickSort ();
#endif

// ==================
// 3-9  MISCELLANEOUS
// ==================
#ifndef CATRCOLL_FillArray
#define dclCATRCOLL_FillArray( T, N )
#else	
#define dclCATRCOLL_FillArray( T, N )                                                \
void FillArray ( T* ioArray, int iMaxSize ) const ;
#endif

#ifndef CATRCOLL_NbOccur
#define dclCATRCOLL_NbOccur( T, N )
#else	
#define dclCATRCOLL_NbOccur( T, N )                                                \
int NbOccur ( T iTest );
#endif


#ifndef CATRCOLL_Intersection
#define dclCATRCOLL_Intersection( T, N )
#else  
#define dclCATRCOLL_Intersection( T, N )                                     \
static void Intersection ( const CATRawColl##N& iRC1,                        \
                           const CATRawColl##N& iRC2,                        \
                           CATRawColl##N&       ioResult );
#endif

//
// 4- Required includes and forward declarations
//    ==========================================
//
#include "CATCollec.h"
#include "CATCollecRoot.h"
#include  <stdlib.h>


