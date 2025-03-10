
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATAssert.h"

#ifndef CATCOLLEC_ExportedBy
/** @nodoc 
 * Macro for Windows NT to trigger the export of the list class
 * defined in a DLL.
 */
#define CATCOLLEC_ExportedBy
#endif

#ifndef CATLISTPP_DECLARE
/** @nodoc 
 * Macros to declare classes which implement lists of pointers 
 * to a user-defined type X with a pre-allocated
 * static buffer.
 * <br><b>Role:</b> The functionalities supported by the list class
 * can be turned on and off depending on your needs, using #define
 * symbols (if the proper #defined symbol is present, the
 * functionality is added to the class, otherwise it is omitted).
 * There is a trade-off between code-size and functionalities:
 * the more functionalities, the larger the code size.
 * The list of available functionalities is defined in 
 * <tt>CATLISTPP_AllFunct.h</tt>
 */
#define CATLISTPP_DECLARE( T )           CATLISTPP_DECLARE_TNS( T, T, 1 )
#endif

#ifndef CATLISTPP_DECLARE_TS
/** @nodoc 
 * Macro to declare a list class with a pre-allocated static buffer.
 * The size of the buffer is specified by the user.
 */
#define CATLISTPP_DECLARE_TS( T, S )     CATLISTPP_DECLARE_TNS( T, T, S )
#endif

#ifndef CATLISTPP_DECLARE_TN
/** @nodoc 
 * Macro to declare a list class with a user-defined name.
 */
#define CATLISTPP_DECLARE_TN( T, N )     CATLISTPP_DECLARE_TNS( T, N, 1 )
#endif

#ifndef CATLISTPP_DECLARE_TNS
/** @nodoc 
 * Macro to declare a list class with a pre-allocated static buffer.
 * The size of the buffer and the name of the class are specified 
 * by the user.
 */
#define CATLISTPP_DECLARE_TNS( T, N, S )                                    \
class CATCOLLEC_ExportedBy CATListPtr##N : public CATCollecRoot             \
{                                                                           \
  public :                                                                  \
                                                                            \
    CATListPtr##N ( int iInitAlloc = 0 );                                   \
    CATListPtr##N ( const CATListPtr##N& iCopy );                           \
    dclCATLISTPP_CtorFromArrayPtrs( T, N, S )                               \
    virtual ~CATListPtr##N ();                                              \
                                                                            \
    CATListPtr##N& operator= ( const CATListPtr##N& iCopy );                \
                                                                            \
    dclCATLISTPP_Append( T, N, S )                                          \
    dclCATLISTPP_AppendList( T, N, S )                                      \
                                                                            \
    dclCATLISTPP_InsertAt( T, N, S )                                        \
                                                                            \
    inline int Size () const { return _Size; }                              \
    dclCATLISTPP_ReSize( T, N, S )                                          \
                                                                            \
    dclCATLISTPP_Storage( T, N, S )                                         \
    inline T*& operator[] ( int iPos ) {                                    \
        CATAssert ( iPos > 0 && iPos <= _Size ) ;                           \
        return ( (T*&) (_Block[iPos-1]) ); }                                \
    inline T* operator[] ( int iPos ) const {                               \
        CATAssert ( iPos > 0 && iPos <= _Size ) ;                           \
		return ( (T*) (_Block[iPos-1]) ); }                         \
                                                                            \
    dclCATLISTPP_Locate( T, N, S )                                          \
                                                                            \
    dclCATLISTPP_RemoveValue( T, N, S )                                     \
    dclCATLISTPP_RemoveList( T, N, S )                                      \
    dclCATLISTPP_RemovePosition( T, N, S )                                  \
    dclCATLISTPP_RemoveAll( T, N, S )                                       \
    dclCATLISTPP_RemoveNulls( T, N, S )                                     \
    dclCATLISTPP_RemoveDuplicatesCount( T, N, S )                           \
                                                                            \
    inline int operator == ( const CATListPtr##N& iL ) const                \
                      { return ( *this != iL  ?  0  :  1 ); }               \
    int operator != ( const CATListPtr##N& iL ) const;                      \
                                                                            \
    dclCATLISTPP_Compare( T, N, S )                                         \
                                                                            \
    dclCATLISTPP_Swap( T, N, S )                                            \
    dclCATLISTPP_QuickSort( T, N, S )                                       \
                                                                            \
    dclCATLISTPP_FillArrayPtrs( T, N, S )                                   \
    dclCATLISTPP_NbOccur( T, N, S )                                         \
    dclCATLISTPP_Intersection( T, N, S )                                    \
                                                                            \
  protected :                                                               \
                                                                            \
    inline void GetStorage ( int iNbElem, T**& oBlock )                     \
                   { oBlock = new T* [ iNbElem ]; }                         \
    inline void FreeStorage ( T**& ioBlock )                                \
    {                                                                       \
      if ( ioBlock ) {                                                      \
    	delete [] ioBlock ;                                                 \
    	ioBlock = NULL ;                                                    \
      }                                                                     \
    }                                                                       \
                                                                            \
    static const int _SBSize ;                                              \
    int     _Size ;                                                         \
    int     _MaxSize ;                                                      \
    T*      _SBlock [S] ;                                                   \
    T**     _Block ;                                                        \
};

#endif          // CATLISTPP_DECLARE_TNS

#if  defined(CATLISTPP_Compare)  ||  defined(CATLISTPP_QuickSort)
/** @nodoc */
#define  CATLISTPP_GenericCompare
#endif


#ifndef CATLISTPP_APPLY_DELETE
/** @nodoc 
 * Macro to apply the <tt>delete</tt> C++ operator
 * on all the elements of a pointer list.
 */
#define CATLISTPP_APPLY_DELETE( LP )                                        \
{                                                                           \
  for ( int i = (LP).Size(); i > 0; i-- ) {                                 \
    void*& PV = (void*&) (LP)[i] ;                                          \
    delete (LP)[i] ;                                                        \
    PV = (void*) 0 ;                                                        \
  }                                                                         \
}
#endif


#undef  dclCATLISTPP_CtorFromArrayPtrs
#undef  dclCATLISTPP_Append
#undef  dclCATLISTPP_AppendList
#undef  dclCATLISTPP_InsertAt
#undef  dclCATLISTPP_ReSize
#undef  dclCATLISTPP_Storage
#undef  dclCATLISTPP_Locate
#undef  dclCATLISTPP_RemoveValue
#undef  dclCATLISTPP_RemoveList
#undef  dclCATLISTPP_RemovePosition
#undef  dclCATLISTPP_RemoveAll
#undef  dclCATLISTPP_RemoveNulls
#undef  dclCATLISTPP_RemoveDuplicatesCount
#undef  dclCATLISTPP_Compare
#undef  dclCATLISTPP_Swap
#undef  dclCATLISTPP_QuickSort
#undef  dclCATLISTPP_FillArrayPtrs
#undef  dclCATLISTPP_NbOccur
#undef  dclCATLISTPP_Intersection


#ifndef CATLISTPP_CtorFromArrayPtrs
/** @nodoc
 * Macros to declare optional member functions
 * in the list class.
 */
#define dclCATLISTPP_CtorFromArrayPtrs( T, N, S )
#else
/** @nodoc */
#define dclCATLISTPP_CtorFromArrayPtrs( T, N, S )                           \
CATListPtr##N ( T** iArray, int iSize );
#endif


#ifndef  CATLISTPP_Append
/** @nodoc */
#define dclCATLISTPP_Append( T, N, S )
#else         
/** @nodoc */
#define dclCATLISTPP_Append( T, N, S )                                     \
void Append ( T* iAdd );
#endif


#ifndef  CATLISTPP_AppendList
/** @nodoc */
#define dclCATLISTPP_AppendList( T, N, S )
#else         
/** @nodoc */
#define dclCATLISTPP_AppendList( T, N, S )                                 \
void Append  ( const CATListPtr##N& iConcat );
#endif


#ifndef CATLISTPP_InsertAt
/** @nodoc */
#define dclCATLISTPP_InsertAt( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_InsertAt( T, N, S )                                    \
void InsertAt ( int iPos, T* iAdd );
#endif


#ifndef CATLISTPP_ReSize
/** @nodoc */
#define dclCATLISTPP_ReSize( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_ReSize( T, N, S )                                      \
void Size ( int iSize );
#endif


#ifndef CATLISTPP_Storage
/** @nodoc */
#define dclCATLISTPP_Storage( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_Storage( T, N, S )                                     \
T** Storage ( ) const  { return _Block ; }
#endif


#ifndef CATLISTPP_Locate
/** @nodoc */
#define dclCATLISTPP_Locate( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_Locate( T, N, S )                                      \
int Locate ( T* iLocate, int iFrom = 1 ) const;
#endif


#ifndef CATLISTPP_RemoveValue
/** @nodoc */
#define dclCATLISTPP_RemoveValue( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_RemoveValue( T, N, S )                                 \
int RemoveValue ( T* iRemove );
#endif


#ifndef CATLISTPP_RemoveList
/** @nodoc */
#define dclCATLISTPP_RemoveList( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_RemoveList( T, N, S )                                  \
int Remove ( const CATListPtr##N& iSubstract );
#endif


#ifndef CATLISTPP_RemovePosition
/** @nodoc */
#define dclCATLISTPP_RemovePosition( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_RemovePosition( T, N, S )                              \
void RemovePosition ( int iPos );
#endif


#ifndef CATLISTPP_RemoveAll
/** @nodoc */
#define dclCATLISTPP_RemoveAll( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_RemoveAll( T, N, S )                                   \
void RemoveAll ( CATCollec::MemoryHandling iMH = CATCollec::ReleaseAllocation );
#endif


#ifndef CATLISTPP_RemoveNulls
/** @nodoc */
#define dclCATLISTPP_RemoveNulls( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_RemoveNulls( T, N, S )                                 \
int RemoveNulls ();
#endif


#if !defined(CATLISTPP_RemoveDuplicatesCount) && !defined(CATLISTPP_RemoveDuplicates)
/** @nodoc */
#define dclCATLISTPP_RemoveDuplicatesCount( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_RemoveDuplicatesCount( T, N, S )                       \
int RemoveDuplicates( CATListPtr##N* ioExtract = NULL );
#endif


#ifndef CATLISTPP_Compare
/** @nodoc */
#define dclCATLISTPP_Compare( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_Compare( T, N, S )                                     \
static int Compare ( const CATListPtr##N& iL1,                              \
                     const CATListPtr##N& iL2,                              \
                     int (*iPFCompare)( T*, T*) );
#endif


#ifndef CATLISTPP_Swap
/** @nodoc */
#define dclCATLISTPP_Swap( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_Swap( T, N, S )                                        \
void Swap ( int iPos1, int iPos2 );
#endif


#ifndef CATLISTPP_QuickSort
/** @nodoc */
#define dclCATLISTPP_QuickSort( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_QuickSort( T, N, S )                                   \
void QuickSort ( int (*iPFCompare)( T*, T* ) );
#endif


#ifndef CATLISTPP_FillArrayPtrs
/** @nodoc */
#define dclCATLISTPP_FillArrayPtrs( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_FillArrayPtrs( T, N, S )                               \
void FillArray ( T** ioArray, int iMaxSize ) const;
#endif


#ifndef CATLISTPP_NbOccur
/** @nodoc */
#define dclCATLISTPP_NbOccur( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_NbOccur( T, N, S )                                     \
int NbOccur ( T* iTest );
#endif


#ifndef CATLISTPP_Intersection
/** @nodoc */
#define dclCATLISTPP_Intersection( T, N, S )
#else        
/** @nodoc */
#define dclCATLISTPP_Intersection( T, N, S )                                \
static void Intersection ( const CATListPtr##N& iL1,                        \
                           const CATListPtr##N& iL2,                        \
                           CATListPtr##N& ioResult );
#endif


#include "CATCollec.h"
#include "CATCollecRoot.h"
#include  <stdlib.h>
