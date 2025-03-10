
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include  "CATAssert.h"

#ifndef CATLISTPP_DEFINE
/** @nodoc 
 * Macros to define classes which implement lists of pointers 
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
#define CATLISTPP_DEFINE( T )            CATLISTPP_DEFINE_TNS( T, T, 1 )
#endif        
        
#ifndef CATLISTPP_DEFINE_TS
/** @nodoc */
#define CATLISTPP_DEFINE_TS( T, S )      CATLISTPP_DEFINE_TNS( T, T, S )
#endif        
        
#ifndef CATLISTPP_DEFINE_TN
/** @nodoc */
#define CATLISTPP_DEFINE_TN( T, N )      CATLISTPP_DEFINE_TNS( T, N, 1 )
#endif        
        
#ifndef CATLISTPP_DEFINE_TNS
/** @nodoc */
#define CATLISTPP_DEFINE_TNS( T, N, S )                                                 \
const int CATListPtr##N::_SBSize = S;                                                   \
                                                                                        \
/* CDTORS */                                                                            \
CATListPtr##N::CATListPtr##N ( int iInitAlloc ) :_Size ( 0 )                            \
{                                                                                       \
  /* Initial allocation (if requested) */                                               \
  if ( iInitAlloc > _SBSize ) {                                                         \
    _MaxSize = iInitAlloc;                                                              \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
  }                                                                                     \
  else {                                                                                \
    _MaxSize = _SBSize;                                                                 \
    _Block = _SBlock;                                                                   \
  }                                                                                     \
}                                                                                       \
CATListPtr##N::CATListPtr##N ( const CATListPtr##N&  iCopy ) : _Size ( iCopy._Size )    \
{                                                                                       \
  /* 1. initial allocation to required size (if necessary) */                           \
  if ( iCopy._Size > _SBSize ) {                                                        \
    _MaxSize = iCopy._Size ;                                                            \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
  }                                                                                     \
  else {                                                                                \
    _MaxSize = _SBSize ;                                                                \
    _Block = _SBlock ;                                                                  \
  }                                                                                     \
  /* 2. copy elements */                                                                \
  memcpy ( (void*) _Block, (const void*) iCopy._Block, sizeof(T*) * _Size );            \
}                                                                                       \
                                                                                        \
defCATLISTPP_CtorFromArrayPtrs( T, N, S )                                               \
                                                                                        \
CATListPtr##N::~CATListPtr##N ()                                                        \
{                                                                                       \
  if ( _MaxSize > _SBSize ) FreeStorage ( _Block ) ;                                    \
}                                                                                       \
                                                                                        \
/* "COPY" OPERATOR */                                                                   \
CATListPtr##N& CATListPtr##N::operator= ( const CATListPtr##N& iCopy )                  \
{                                                                                       \
  if ( &iCopy == this ) return *this ;                                                  \
                                                                                        \
  _Size = iCopy._Size ;                                                                 \
  if ( _Size > _MaxSize ) {                                                             \
    if ( _MaxSize > _SBSize )                                                           \
      FreeStorage ( _Block ) ;                                                          \
                                                                                        \
    _MaxSize = _Size ;                                                                  \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
  }                                                                                     \
  memcpy ( (void*) _Block, (const void*) iCopy._Block, sizeof(T*) * _Size ) ;           \
  return *this ;                                                                        \
}                                                                                       \
                                                                                        \
/* APPEND ELEMENT(S) */                                                                 \
defCATLISTPP_Append( T, N, S )                                                          \
defCATLISTPP_AppendList( T, N, S )                                                      \
                                                                                        \
/* INSERT ELEMENT */                                                                    \
defCATLISTPP_InsertAt( T, N, S )                                                        \
                                                                                        \
/* GET/SET NUMBER OF ELEMENTS */                                                        \
defCATLISTPP_ReSize( T, N, S )                                                          \
                                                                                        \
/* LOCATE ELEMENT */                                                                    \
defCATLISTPP_Locate( T, N, S )                                                          \
                                                                                        \
/* REMOVE ELEMENT(S) */                                                                 \
defCATLISTPP_RemoveValue( T, N, S )                                                     \
defCATLISTPP_RemoveList( T, N, S )                                                      \
defCATLISTPP_RemovePosition( T, N, S )                                                  \
defCATLISTPP_RemoveAll( T, N, S )                                                       \
defCATLISTPP_RemoveNulls( T, N, S )                                                     \
defCATLISTPP_RemoveDuplicatesCount( T, N, S )                                           \
                                                                                        \
/* COMPARISON OPERATORS */                                                              \
int CATListPtr##N::operator != ( const CATListPtr##N& iL ) const                        \
{                                                                                       \
  if ( &iL == this )       return  0 ;                                                  \
  if ( _Size != iL._Size ) return  1 ;                                                  \
                                                                                        \
  /* if one element is different from its corresponding element, */                     \
  /* the lists are different */                                                         \
  for ( int i = 0;  i < _Size;  i++ )                                                   \
    if ( _Block[i] != iL._Block[i] ) return 1 ;                                         \
                                                                                        \
  return 0 ;                                                                            \
}                                                                                       \
                                                                                        \
/* COMPARE FUNCTION */                                                                  \
defCATLISTPP_Compare( T, N, S )                                                         \
                                                                                        \
/* SWAP/SORT ELEMENT(S) */                                                              \
defCATLISTPP_Swap( T, N, S )                                                            \
defCATLISTPP_QuickSort( T, N, S )                                                       \
                                                                                        \
/* MISCELLANEOUS */                                                                     \
defCATLISTPP_FillArrayPtrs( T, N, S )                                                   \
defCATLISTPP_NbOccur( T, N, S )                                                         \
defCATLISTPP_Intersection( T, N, S )

#endif          // CATLISTPP_DEFINE_TN



#undef  defCATLISTPP_CtorFromArrayPtrs
#undef  defCATLISTPP_Append
#undef  defCATLISTPP_AppendList
#undef  defCATLISTPP_InsertAt
#undef  defCATLISTPP_ReSize
#undef  defCATLISTPP_Locate
#undef  defCATLISTPP_RemoveValue
#undef  defCATLISTPP_RemoveList
#undef  defCATLISTPP_RemovePosition
#undef  defCATLISTPP_RemoveAll
#undef  defCATLISTPP_RemoveNulls
#undef  defCATLISTPP_RemoveDuplicatesCount
#undef  defCATLISTPP_Compare
#undef  defCATLISTPP_Swap
#undef  defCATLISTPP_QuickSort
#undef  defCATLISTPP_FillArrayPtrs
#undef  defCATLISTPP_NbOccur
#undef  defCATLISTPP_Intersection


#ifndef CATLISTPP_CtorFromArrayPtrs
/** @nodoc */
#define defCATLISTPP_CtorFromArrayPtrs( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_CtorFromArrayPtrs( T, N, S )                                       \
CATListPtr##N::CATListPtr##N ( T** iArray, int iSize ) : _Size ( iSize )                \
{                                                                                       \
  /* 1. initial allocation to required size (if necessary) */                           \
  if ( iSize > _SBSize ) {                                                              \
    _MaxSize = iSize ;                                                                  \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
  }                                                                                     \
  else {                                                                                \
    _MaxSize = _SBSize ;                                                                \
    _Block = _SBlock ;                                                                  \
  }                                                                                     \
  /* 2. copy elements */                                                                \
  memcpy ( (void*) _Block, (const void*) iArray, sizeof(T*) * _Size );                  \
}
#endif


#ifndef CATLISTPP_Append
/** @nodoc */
#define defCATLISTPP_Append( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_Append( T, N, S )                                                  \
void CATListPtr##N::Append ( T* iAdd )                                                  \
{                                                                                       \
  /* 1. if block is too small, reallocate it */                                         \
  if ( _Size == _MaxSize ) {                                                            \
                                                                                        \
    /* 1.1 save old data */                                                             \
    int oldMaxSize = _MaxSize ;                                                         \
    T** oldBlock = _Block ;                                                             \
                                                                                        \
    /* 1.2 compute new block max size */                                                \
    _MaxSize <<= 1;                                                                     \
                                                                                        \
    /* 1.3 allocate a new big enough block */                                           \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
                                                                                        \
    /* 1.4 copy initial block to reallocated block */                                   \
    memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T*) * _Size ) ;             \
                                                                                        \
    /* 1.5 get rid of old too little block */                                           \
    if ( oldMaxSize > _SBSize )                                                         \
       FreeStorage ( oldBlock ) ;                                                       \
  }                                                                                     \
                                                                                        \
  /* 2. add new element at the end */                                                   \
  _Block[_Size++] = iAdd ;                                                              \
}
#endif


#ifndef CATLISTPP_AppendList
/** @nodoc */
#define defCATLISTPP_AppendList( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_AppendList( T, N, S )                                              \
void                                                                                    \
CATListPtr##N::Append ( const CATListPtr##N& iConcat )                                  \
{                                                                                       \
  /* 1. if block is too small, reallocate it */                                         \
  int newSize = _Size + iConcat._Size ;                                                 \
  if ( newSize > _MaxSize ) {                                                           \
                                                                                        \
    /* 1.1 save old data */                                                             \
    int oldMaxSize = _MaxSize ;                                                         \
    T** oldBlock = _Block ;                                                             \
                                                                                        \
    /* 1.2 compute new block max size */                                                \
    _MaxSize = newSize;                                                                 \
                                                                                        \
    /* 1.3 allocate a new big enough block */                                           \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
                                                                                        \
    /* 1.4 copy initial block to reallocated block */                                   \
    memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T*) * _Size ) ;             \
                                                                                        \
    /* 1.5 get rid of old too little block */                                           \
    if ( oldMaxSize > _SBSize )                                                         \
       FreeStorage ( oldBlock ) ;                                                       \
  }                                                                                     \
  /* 2. add new elements at the end */                                                  \
  memcpy ( (void*) &_Block[_Size], (const void*) iConcat._Block,                        \
           sizeof(T*) * iConcat._Size ) ;                                               \
  _Size = newSize ;                                                                     \
}
#endif


#ifndef CATLISTPP_InsertAt
/** @nodoc */
#define defCATLISTPP_InsertAt( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_InsertAt( T, N, S )                                                \
void CATListPtr##N::InsertAt ( int iPos, T* iAdd )                                      \
{                                                                                       \
  CATAssert ( iPos >= 1  &&  iPos <= _Size+1 ) ;                                           \
                                                                                        \
  /* 1. if block is too small, reallocate it */                                         \
  if ( _Size == _MaxSize ) {                                                            \
                                                                                        \
    /* 1.1 save old data */                                                             \
    int oldMaxSize = _MaxSize ;                                                         \
    T** oldBlock = _Block ;                                                             \
                                                                                        \
    /* 1.2 compute new block max size */                                                \
    _MaxSize <<= 1;                                                                     \
                                                                                        \
    /* 1.3 allocate a new big enough block */                                           \
    GetStorage ( _MaxSize, _Block ) ;                                                   \
                                                                                        \
    /* 1.4 copy first elements from initial block to reallocated block */               \
    if ( iPos > 1 )                                                                     \
      memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T*) * (iPos - 1) ) ;      \
                                                                                        \
    /* 1.5 insert new element */                                                        \
    _Block[iPos-1] = iAdd ;                                                             \
                                                                                        \
    /* 1.6 copy last elements from initial block to reallocated block */                \
    if ( iPos <= _Size )                                                                \
      memcpy ( (void*) &_Block[iPos], (const void*) &oldBlock[iPos-1],                  \
                sizeof(T*) * (_Size - iPos + 1) ) ;                                     \
                                                                                        \
    /* 1.7 get rid of old too little block */                                           \
    if ( oldMaxSize > _SBSize )                                                         \
       FreeStorage ( oldBlock ) ;                                                       \
  }                                                                                     \
  /* 2. block is big enough */                                                          \
  else {                                                                                \
    /* 2.1 move last elements to create a hole */                                       \
    if ( iPos <= _Size )                                                                \
      memmove ( &_Block[iPos], &_Block[iPos-1], sizeof(T*) * (_Size - iPos + 1) );      \
                                                                                        \
    /* 2.2 insert element in hole */                                                    \
    _Block[iPos-1] = iAdd ;                                                             \
  }                                                                                     \
  _Size ++ ;                                                                            \
}
#endif


#ifndef CATLISTPP_ReSize
/** @nodoc */
#define defCATLISTPP_ReSize( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_ReSize( T, N, S )                                                  \
void CATListPtr##N::Size ( int iSize )                                                  \
{                                                                                       \
  if ( iSize > _Size ) {                                                                \
                                                                                        \
    /* 1. if block is too small, reallocate it */                                       \
    if ( iSize > _MaxSize ) {                                                           \
                                                                                        \
       /* 1.1 save old data */                                                          \
       int oldMaxSize = _MaxSize ;                                                      \
       T** oldBlock = _Block ;                                                          \
                                                                                        \
       /* 1.2 allocate a new big enough block */                                        \
       _MaxSize = iSize ;                                                               \
       GetStorage ( _MaxSize, _Block ) ;                                                \
                                                                                        \
       /* 1.3 copy elements from initial block to reallocated block */                  \
       memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T*) * _Size ) ;          \
                                                                                        \
       /* 1.4 set values */                                                             \
       memset ( &_Block[_Size], 0, sizeof(T*) * (iSize - _Size) ) ;                     \
                                                                                        \
       /* 1.5 get rid of old too little block */                                        \
       if ( oldMaxSize > _SBSize )                                                      \
         FreeStorage ( oldBlock ) ;                                                     \
    }                                                                                   \
    /* 2. else set values */                                                            \
    else {                                                                              \
      memset ( &_Block[_Size], '\0', sizeof(T*) * (iSize - _Size) ) ;                   \
    }                                                                                   \
  }                                                                                     \
  _Size = iSize ;                                                                       \
}
#endif


#ifndef CATLISTPP_Locate
/** @nodoc */
#define defCATLISTPP_Locate( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_Locate( T, N, S )                                                  \
int CATListPtr##N::Locate ( T* iLocate, int iFrom ) const                               \
{                                                                                       \
  for ( int i = (iFrom - 1) ; i < _Size ; i++ )                                         \
    if ( _Block[i] == iLocate )                                                         \
      return i+1 ;                                                                      \
  return 0 ; /* not found */                                                            \
}
#endif


#ifndef CATLISTPP_RemoveValue
/** @nodoc */
#define defCATLISTPP_RemoveValue( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_RemoveValue( T, N, S )                                             \
int CATListPtr##N::RemoveValue ( T* iRemove )                                           \
{                                                                                       \
  int pos = Locate ( iRemove ) ;                                                        \
  if ( pos )                                                                            \
    RemovePosition ( pos ) ;                                                            \
  return pos ;                                                                          \
}
#endif


#ifndef CATLISTPP_RemoveList
/** @nodoc */
#define defCATLISTPP_RemoveList( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_RemoveList( T, N, S )                                              \
int CATListPtr##N::Remove ( const CATListPtr##N& iSubstract )                           \
{                                                                                       \
  /* crazy case */                                                                      \
  if ( &iSubstract == this ) {                                                          \
    int size = _Size ;                                                                  \
    RemoveAll() ;                                                                       \
    return size ;                                                                       \
  }                                                                                     \
  int count = 0 ;                                                                       \
  for ( int i = 0 ; i < iSubstract._Size ; i++ )                                        \
    if ( RemoveValue ( iSubstract._Block[i] ) )                                         \
       count ++ ;                                                                       \
  return count ;                                                                        \
}
#endif


#ifndef CATLISTPP_RemovePosition
/** @nodoc */
#define defCATLISTPP_RemovePosition( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_RemovePosition( T, N, S )                                          \
void CATListPtr##N::RemovePosition ( int iPos )                                         \
{                                                                                       \
  CATAssert ( iPos > 0  &&  iPos <= _Size ) ;                                              \
  /* fill the gap (if necessary) */                                                     \
  if ( iPos < _Size )                                                                   \
    memmove ( &_Block[iPos-1], &_Block[iPos], sizeof(T*) * (_Size - iPos) ) ;           \
  _Size--;                                                                              \
}
#endif


#ifndef CATLISTPP_RemoveAll
/** @nodoc */
#define defCATLISTPP_RemoveAll( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_RemoveAll( T, N, S )                                               \
void CATListPtr##N::RemoveAll ( CATCollec::MemoryHandling iMH )                         \
{                                                                                       \
  _Size = 0 ;                                                                           \
  if ( iMH == CATCollec::ReleaseAllocation ) {                                          \
    if ( _MaxSize > _SBSize ) {                                                         \
      FreeStorage ( _Block ) ;                                                          \
      _Block = _SBlock ;                                                                \
      _MaxSize = _SBSize ;                                                              \
    }                                                                                   \
  }                                                                                     \
}
#endif


#ifndef CATLISTPP_RemoveNulls
/** @nodoc */
#define defCATLISTPP_RemoveNulls( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_RemoveNulls( T, N, S )                                             \
int CATListPtr##N::RemoveNulls ( )                                                      \
{                                                                                       \
  int count = 0 ;                                                                       \
  int pos = 1 ;                                                                         \
  int nullPos = 0 ;                                                                     \
  T* nullPtr = 0 ;                                                                      \
  do {                                                                                  \
    nullPos = Locate ( nullPtr, pos ) ;                                                 \
    if ( nullPos ) {                                                                    \
      RemovePosition ( nullPos ) ;                                                      \
      count++;                                                                          \
      pos = nullPos ;                                                                   \
    }                                                                                   \
  } while ( nullPos ) ;                                                                 \
                                                                                        \
  return count ;                                                                        \
}
#endif


#if !defined(CATLISTPP_RemoveDuplicatesCount) && !defined(CATLISTPP_RemoveDuplicates)
/** @nodoc */
#define defCATLISTPP_RemoveDuplicatesCount( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_RemoveDuplicatesCount( T, N, S )                                   \
int CATListPtr##N::RemoveDuplicates ( CATListPtr##N* ioExtract )                        \
{                                                                                       \
  int count = 0 ;                                                                       \
  for ( int i = 0  ;  i < _Size  ;  i ++ ) {                                            \
    int j = i+1 ;                                                                       \
    while ( j < _Size ) {                                                               \
      if ( _Block[i] == _Block[j] ) {                                                   \
        if ( ioExtract )  ioExtract -> Append ( _Block[j] );                            \
        RemovePosition ( j+1 );                                                         \
        count ++ ;                                                                      \
      }                                                                                 \
      else                                                                              \
        j ++ ;                                                                          \
    }                                                                                   \
  }                                                                                     \
  return count ;                                                                        \
}
#endif


#ifndef CATLISTPP_Compare
/** @nodoc */
#define defCATLISTPP_Compare( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_Compare( T, N, S )                                                 \
int CATListPtr##N::Compare  ( const CATListPtr##N& iL1,                                 \
                              const CATListPtr##N& iL2,                                 \
                              int (*iPFCompare)( T*, T*) )                              \
{                                                                                       \
  if ( &iL1 == &iL2 ) return  0 ;                                                       \
  if      ( iL1._Size < iL2._Size ) return -1 ;                                         \
  else if ( iL1._Size > iL2._Size ) return +1 ;                                         \
                                                                                        \
  /* collections have the same size: try to find a difference between elements */       \
  for ( int i = 0  ;  i < iL1._Size  ; i++ ) {                                          \
    int test = (*iPFCompare)( iL1._Block[i], iL2._Block[i] );                           \
    if ( test != 0 ) return test ;                                                      \
  }                                                                                     \
  return 0 ;                                                                            \
}
#endif


#ifndef CATLISTPP_Swap
/** @nodoc */
#define defCATLISTPP_Swap( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_Swap( T, N, S )                                                    \
void CATListPtr##N::Swap ( int iPos1, int iPos2 )                                       \
{                                                                                       \
  CATAssert ( iPos1 > 0  &&  iPos1 <= _Size  &&  iPos2 > 0  &&  iPos2 <= _Size );          \
  if ( iPos1 == iPos2 ) return ;                                                        \
                                                                                        \
  T*     tempo = _Block[iPos1-1] ;                                                      \
  _Block[iPos1-1] = _Block[iPos2-1] ;                                                   \
  _Block[iPos2-1] = tempo ;                                                             \
}
#endif


#ifndef CATLISTPP_QuickSort
/** @nodoc */
#define defCATLISTPP_QuickSort( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_QuickSort( T, N, S )                                               \
namespace dsy { namespace internal { ExportedByCO0LSTPV void CATListQSort(void **, int, int (*iPFCompare)(const void*, const void*)); } }\
void CATListPtr##N::QuickSort ( int (*iPFCompare)( T*, T* ) )                           \
{                                                                                       \
  dsy::internal::CATListQSort((void **)_Block, _Size, (CATCollec::PFCompare)iPFCompare ) ;\
}
#endif


#ifndef CATLISTPP_FillArrayPtrs
/** @nodoc */
#define defCATLISTPP_FillArrayPtrs( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_FillArrayPtrs( T, N, S )                                           \
void CATListPtr##N::FillArray ( T** ioArray, int  iMaxSize ) const                      \
{                                                                                       \
  int limit = (iMaxSize < _Size) ? iMaxSize : _Size ;                                   \
  for ( int i = 0  ;  i < limit  ;  i++ )                                               \
    ioArray[i] = _Block[i] ;                                                            \
}
#endif


#ifndef CATLISTPP_NbOccur
/** @nodoc */
#define defCATLISTPP_NbOccur( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_NbOccur( T, N, S )                                                 \
int CATListPtr##N::NbOccur ( T* iTest )                                                 \
{                                                                                       \
  int nb_occ = 0 ;                                                                      \
  for ( int i = 0;  i < _Size;  i++ )                                                   \
  if ( _Block[i] == iTest )                                                             \
    nb_occ ++ ;                                                                         \
  return nb_occ ;                                                                       \
}
#endif


#ifndef CATLISTPP_Intersection
/** @nodoc */
#define defCATLISTPP_Intersection( T, N, S )
#else        
/** @nodoc */
#define defCATLISTPP_Intersection( T, N, S )                                            \
void CATListPtr##N::Intersection ( const CATListPtr##N& iL1,                            \
                                   const CATListPtr##N& iL2,                            \
                                   CATListPtr##N& ioResult )                            \
{                                                                                       \
  /* crazy case */                                                                      \
  if ( &iL1 == &iL2 ) {                                                                 \
    ioResult.Append ( iL1 );                                                            \
    return ;                                                                            \
  }                                                                                     \
  /* easy case */                                                                       \
  if ( ( iL1._Size == 0 ) || ( iL2._Size == 0 ) )                                       \
    return ;                                                                            \
                                                                                        \
  int size = iL1._Size ; /* avoid mistakes if ( &iL1 == &ioResult ) */                  \
  for ( int  i = 0; i < size; i++ )                                                     \
    if ( iL2.Locate ( iL1._Block[i] ) )                                                 \
      ioResult.Append ( iL1._Block[i] );                                                \
}
#endif



#include  "CATAssert.h"
#include  <string.h>
