
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#ifndef CATLISTV_DEFINE
/**
 * Macros to define classes which implement lists of values 
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
#define CATLISTV_DEFINE( T )  CATLISTV_DEFINE_D( T, NULL )
#endif

#ifndef CATLISTV_DEFINE_D
/** @nodoc */
#define CATLISTV_DEFINE_D( T, D ) CATLISTV_DEFINE_TND( T, T, D )
#endif

#ifndef CATLISTV_DEFINE_TND
/** @nodoc */
#define CATLISTV_DEFINE_TND( T, N, D )                                                  \
/* CDTORS */                                                                            \
CATListVal##N::CATListVal##N ()                                                         \
{}                                                                                      \
CATListVal##N::CATListVal##N ( int iInitAlloc ) : _Lval ( iInitAlloc )                  \
{}                                                                                      \
CATListVal##N::CATListVal##N ( int iInitAlloc, int iExtensAlloc ): _Lval ( iInitAlloc ) \
{}                                                                                      \
CATListVal##N::CATListVal##N ( const CATListVal##N& iCopy ): _Lval ( iCopy.Size() )     \
{                                                                                       \
  int size = iCopy.Size() ;                                                             \
  for ( int  i = 1; i <= size; i++ )                                                    \
    Append ( iCopy[i] );                                                                \
}                                                                                       \
                                                                                        \
defCATLISTV_CtorFromArrayVals( T, N )                                                   \
defCATLISTV_CtorFromArrayPtrs( T, N )                                                   \
                                                                                        \
CATListVal##N::~CATListVal##N  ()                                                       \
{                                                                                       \
  for ( int i = _Lval.Size(); i > 0; i-- )                                              \
  {                                                                                     \
    T* pt = (T*) _Lval[i] ;                                                             \
    delete pt ;                                                                         \
  }                                                                                     \
}                                                                                       \
                                                                                        \
/* "COPY" OPERATOR */                                                                   \
CATListVal##N& CATListVal##N::operator= ( const CATListVal##N& iCopy )                  \
{                                                                                       \
  if ( &iCopy == this ) return *this ;                                                  \
                                                                                        \
  int i ;                                                                               \
  /* delete old contents */                                                             \
  for ( i = _Lval.Size(); i > 0; i-- )                                                  \
  {                                                                                     \
    T* pt = (T*) _Lval[i] ;                                                             \
    delete pt ;                                                                         \
  }                                                                                     \
  _Lval.RemoveAll ( CATCollec::KeepAllocation );                                        \
                                                                                        \
  /* copy new contents */                                                               \
  int size = iCopy.Size() ;                                                             \
  for ( i = 1; i <= size; i++ )                                                         \
    Append ( iCopy[i] );                                                                \
  return  *this ;                                                                       \
}                                                                                       \
                                                                                        \
/* APPEND ELEMENT(S) */                                                                 \
void CATListVal##N::Append ( const T& iAdd )                                            \
{ _Lval.Append ( new T (iAdd) ); }                                                      \
                                                                                        \
defCATLISTV_AppendList( T, N )                                                          \
                                                                                        \
/* INSERT ELEMENT */                                                                    \
defCATLISTV_InsertAfter( T, N )                                                         \
defCATLISTV_InsertBefore( T, N )                                                        \
defCATLISTV_InsertAt( T, N )                                                            \
                                                                                        \
/* GET/SET NUMBER OF ELEMENTS */                                                        \
int CATListVal##N::Size () const                                                        \
{ return  _Lval.Size(); }                                                               \
                                                                                        \
defCATLISTV_ReSize( T, N, D )                                                           \
                                                                                        \
/* INDEXATION OPERATORS */                                                              \
T& CATListVal##N::operator[] ( int iPos )                                               \
{ return  ( (T&) *( (T*) ( _Lval[iPos] ) ) ); }                                         \
                                                                                        \
const T& CATListVal##N::operator[] ( int iPos ) const                                   \
{ return  ( (const T&) *( (const T*) ( _Lval[iPos] ) ) ); }                             \
                                                                                        \
/* LOCALIZE ELEMENT */                                                                  \
defCATLISTV_Locate( T, N )                                                              \
                                                                                        \
/* REMOVE ELEMENT(S) */                                                                 \
void CATListVal##N::RemoveAll ( CATCollec::MemoryHandling iMH )                         \
{                                                                                       \
  for ( int  i = Size(); i > 0; i-- )                                                   \
  {                                                                                     \
    T* pt = (T*) _Lval[i] ;                                                             \
    delete pt ;                                                                         \
  }                                                                                     \
  _Lval.RemoveAll( iMH );                                                               \
}                                                                                       \
defCATLISTV_RemoveValue( T, N )                                                         \
defCATLISTV_RemoveList( T, N )                                                          \
defCATLISTV_RemovePosition( T, N )                                                      \
defCATLISTV_RemoveDuplicatesCount( T, N )                                               \
defCATLISTV_RemoveDuplicatesExtract( T, N )                                             \
                                                                                        \
/* COMPARISON OPERATORS */                                                              \
defCATLISTV_eqOP( T, N )                                                                \
defCATLISTV_neOP( T, N )                                                                \
defCATLISTV_gtOP( T, N )                                                                \
defCATLISTV_ltOP( T, N )                                                                \
defCATLISTV_geOP( T, N )                                                                \
defCATLISTV_leOP( T, N )                                                                \
                                                                                        \
/* COMPARE  (STATIC FUNCTION) */                                                        \
defCATLISTV_Compare( T, N )                                                             \
                                                                                        \
/* CHANGE/MOVE ELEMENT(S) */                                                            \
defCATLISTV_Replace( T, N )                                                             \
defCATLISTV_Swap( T, N )                                                                \
defCATLISTV_QuickSort( T, N )                                                           \
                                                                                        \
/* MISCELLANEOUS */                                                                     \
defCATLISTV_ArrayVals( T, N )                                                           \
defCATLISTV_ArrayPtrs( T, N )                                                           \
defCATLISTV_NbOccur( T, N )                                                             \
defCATLISTV_Intersection( T, N )                                                        \
                                                                                        \
/* APPLY */                                                                             \
defCATLISTV_ApplyMemberFunct( T, N )                                                    \
defCATLISTV_ApplyMemberFunctConst( T, N )                                               \
defCATLISTV_ApplyGlobalFunct( T, N )

#endif                // CATLISTV_DEFINE_TND


#undef  defCATLISTV_CtorFromArrayVals
#undef  defCATLISTV_CtorFromArrayPtrs
#undef  defCATLISTV_AppendList
#undef  defCATLISTV_InsertAfter
#undef  defCATLISTV_InsertBefore
#undef  defCATLISTV_InsertAt
#undef  defCATLISTV_ReSize
#undef  defCATLISTV_Locate
#undef  defCATLISTV_RemoveValue
#undef  defCATLISTV_RemoveList
#undef  defCATLISTV_RemovePosition
#undef  defCATLISTV_RemoveDuplicatesExtract
#undef  defCATLISTV_RemoveDuplicatesCount
#undef  defCATLISTV_eqOP
#undef  defCATLISTV_neOP
#undef  defCATLISTV_leOP
#undef  defCATLISTV_geOP
#undef  defCATLISTV_ltOP
#undef  defCATLISTV_gtOP
#undef  defCATLISTV_Compare
#undef  defCATLISTV_Replace
#undef  defCATLISTV_Swap
#undef  defCATLISTV_QuickSort
#undef  defCATLISTV_ArrayVals
#undef  defCATLISTV_ArrayPtrs
#undef  defCATLISTV_NbOccur
#undef  defCATLISTV_Intersection
#undef  defCATLISTV_ApplyMemberFunct
#undef  defCATLISTV_ApplyMemberFunctConst
#undef  defCATLISTV_ApplyGlobalFunct


#ifndef CATLISTV_CtorFromArrayVals
/** @nodoc */
#define defCATLISTV_CtorFromArrayVals( T, N )
#else
/** @nodoc */
#define defCATLISTV_CtorFromArrayVals( T, N )                                        \
CATListVal##N::CATListVal##N ( T* iArray, int iSize ) : _Lval ( iSize )              \
{                                                                                    \
  for ( int  i = 0; i < iSize; i++ )                                                 \
    _Lval.Append ( new T (iArray[i] ) );                                             \
}
#endif


#ifndef CATLISTV_CtorFromArrayPtrs
/** @nodoc */
#define defCATLISTV_CtorFromArrayPtrs( T, N )
#else
/** @nodoc */
#define defCATLISTV_CtorFromArrayPtrs( T, N )                                        \
CATListVal##N::CATListVal##N ( T** iArray, int iSize ) : _Lval ( iSize )             \
{                                                                                    \
  for ( int  i = 0; i < iSize; i++ )                                                 \
  {                                                                                  \
    T* pt = iArray[i] ;                                                              \
    if ( pt )                                                                        \
      _Lval.Append ( new T ( *pt ) );                                                \
  }                                                                                  \
}
#endif


#ifndef CATLISTV_AppendList
/** @nodoc */
#define defCATLISTV_AppendList( T, N )
#else
/** @nodoc */
#define defCATLISTV_AppendList( T, N )                                                \
void CATListVal##N::Append ( const CATListVal##N& iConcat )                           \
{                                                                                     \
  int  size = iConcat.Size() ;                                                        \
  for ( int  i = 1; i <= size; i++ )                                                  \
    Append ( iConcat[i] );                                                            \
}
#endif


#ifndef CATLISTV_InsertAfter
/** @nodoc */
#define defCATLISTV_InsertAfter( T, N )
#else
/** @nodoc */
#define defCATLISTV_InsertAfter( T, N )                                        \
void CATListVal##N::InsertAfter ( int iPos, const T& iAdd )                    \
{ _Lval.InsertAt ( iPos+1, new T (iAdd) ); }
#endif


#ifndef CATLISTV_InsertBefore
/** @nodoc */
#define defCATLISTV_InsertBefore( T, N )
#else
/** @nodoc */
#define defCATLISTV_InsertBefore( T, N )                                        \
void CATListVal##N::InsertBefore ( int iPos, const T& iAdd )                    \
{ _Lval.InsertAt ( iPos, new T (iAdd) ); }
#endif


#ifndef CATLISTV_InsertAt
/** @nodoc */
#define defCATLISTV_InsertAt( T, N )
#else
/** @nodoc */
#define defCATLISTV_InsertAt( T, N )                                            \
void CATListVal##N::InsertAt ( int iPos, const T& iAdd )                        \
{ _Lval.InsertAt ( iPos, new T (iAdd) ); }
#endif


#ifndef CATLISTV_ReSize
/** @nodoc */
#define defCATLISTV_ReSize( T, N, D )
#else
/** @nodoc */
#define defCATLISTV_ReSize( T, N, D )                                                \
void CATListVal##N::Size ( int iSize, const T* iFiller )                             \
{                                                                                    \
  int  old_size = _Lval.Size() ;                                                     \
  if ( old_size == iSize ) return ;                                                  \
                                                                                     \
  if ( old_size > iSize )                                                            \
  {                                                                                  \
    for ( int  i = iSize+1; i <= old_size; i++ )                                     \
    {                                                                                \
      T* pt = (T*) _Lval[i] ;                                                        \
      delete pt ;                                                                    \
    }                                                                                \
    _Lval.Size( iSize );                                                             \
  }                                                                                  \
  else                                                                               \
  {                                                                                  \
    _Lval.Size( iSize );                                                             \
    if ( ! iFiller ) iFiller = D ;                                                   \
    for ( int  i = old_size+1; i <= iSize; i++ )                                     \
      _Lval[i] = new T (*iFiller) ;                                                  \
  }                                                                                  \
}
#endif


#ifndef CATLISTV_Locate
/** @nodoc */
#define defCATLISTV_Locate( T, N )
#else
/** @nodoc */
#define defCATLISTV_Locate( T, N )                                                   \
int CATListVal##N::Locate ( const T& iLocate, int iFrom ) const                      \
{                                                                                    \
  int size = Size() ;                                                                \
  for ( int  i = iFrom; i <= size; i++ )                                             \
    if ( (*this)[i] ==  iLocate ) return i ;                                         \
  return  0 ;                                                                        \
}
#endif


#ifndef CATLISTV_RemoveValue
/** @nodoc */
#define defCATLISTV_RemoveValue( T, N )
#else
/** @nodoc */
#define defCATLISTV_RemoveValue( T, N )                                            \
int CATListVal##N::RemoveValue ( const T& iRemove )                                \
{                                                                                  \
  int pos = Locate ( iRemove );                                                    \
  if ( pos )                                                                       \
    RemovePosition ( pos );                                                        \
  return pos ;                                                                     \
}
#endif


#ifndef CATLISTV_RemoveList
/** @nodoc */
#define defCATLISTV_RemoveList( T, N )
#else
/** @nodoc */
#define defCATLISTV_RemoveList( T, N )                                            \
int CATListVal##N::Remove ( const CATListVal##N& iSubstract )                     \
{                                                                                 \
  int count = 0 ;                                                                 \
  int size = iSubstract.Size() ;                                                  \
  for ( int  i = 1 ; i <= size ; i++ )                                            \
    count += ( RemoveValue ( iSubstract[i] ) ) ? 1 : 0 ;                          \
  return count ;                                                                  \
}
#endif


#ifndef CATLISTV_RemovePosition
/** @nodoc */
#define defCATLISTV_RemovePosition( T, N )
#else
/** @nodoc */
#define defCATLISTV_RemovePosition( T, N )                                         \
void CATListVal##N::RemovePosition ( int iPos )                                    \
{                                                                                  \
  T* pt = (T*) _Lval[iPos] ;                                                       \
  delete pt ;                                                                      \
  _Lval.RemovePosition ( iPos );                                                   \
}
#endif


#ifndef CATLISTV_RemoveDuplicatesExtract
/** @nodoc */
#define defCATLISTV_RemoveDuplicatesExtract( T, N )
#else
/** @nodoc */
#define defCATLISTV_RemoveDuplicatesExtract( T, N )                            \
void CATListVal##N::RemoveDuplicates ( CATListVal##N& ioExtract )              \
{                                                                              \
  int i = 1 ;                                                                  \
  while ( i <= Size() )                                                        \
  {                                                                            \
    int j = i+1 ;                                                              \
    while ( j <= Size() )                                                      \
    {                                                                          \
      if ( (*this)[i] == (*this)[j] )                                          \
      {                                                                        \
        ioExtract.Append( (*this)[j] );                                        \
        RemovePosition( j );                                                   \
      }                                                                        \
      else                                                                     \
        j++ ;                                                                  \
    }                                                                          \
    i++ ;                                                                      \
  }                                                                            \
}
#endif


#ifndef CATLISTV_RemoveDuplicatesCount
/** @nodoc */
#define defCATLISTV_RemoveDuplicatesCount( T, N )
#else
/** @nodoc */
#define defCATLISTV_RemoveDuplicatesCount( T, N )                             \
int CATListVal##N::RemoveDuplicates ()                                        \
{                                                                             \
  int count = 0 ;                                                             \
  int i = 1 ;                                                                 \
  while ( i <= Size() )                                                       \
  {                                                                           \
    int j = i+1 ;                                                             \
    while ( j <= Size() )                                                     \
    {                                                                         \
      if ( (*this)[i] == (*this)[j] )                                         \
      {                                                                       \
        RemovePosition( j );                                                  \
        count ++ ;                                                            \
      }                                                                       \
      else                                                                    \
        j++ ;                                                                 \
    }                                                                         \
    i++ ;                                                                     \
  }                                                                           \
  return count ;                                                              \
}
#endif


#ifndef CATLISTV_eqOP
/** @nodoc */
#define defCATLISTV_eqOP( T, N )
#else
/** @nodoc */
#define defCATLISTV_eqOP( T, N )                                                \
int CATListVal##N::operator == ( const CATListVal##N& iLV ) const               \
{ return ( *this != iLV ? 0 : 1 ); }
#endif


#ifndef CATLISTV_neOP
/** @nodoc */
#define defCATLISTV_neOP( T, N )
#else
/** @nodoc */
#define defCATLISTV_neOP( T, N )                                                \
int CATListVal##N::operator != ( const CATListVal##N& iLV ) const               \
{                                                                               \
  if ( this == &iLV )         return 0 ;                                        \
  if ( Size() != iLV.Size() ) return 1 ;                                        \
                                                                                \
  int size = Size() ;                                                           \
  for ( int  i = 1 ; i <= size ; i++ )                                          \
    if ( (*this)[i] != iLV[i] ) return 1 ;                                      \
  /* no difference found */                                                     \
  return        0 ;                                                             \
}
#endif


#ifndef CATLISTV_leOP
/** @nodoc */
#define defCATLISTV_leOP( T, N )
#else
/** @nodoc */
#define defCATLISTV_leOP( T, N )                                                \
int CATListVal##N::operator <= ( const CATListVal##N& iLV ) const               \
{                                                                               \
  if ( this == &iLV )        return 1 ;                                         \
  if ( Size() > iLV.Size() ) return 0 ;                                         \
  if ( Size() < iLV.Size() ) return 1 ;                                         \
                                                                                \
  for ( int  i = 1 ; i <= Size() ; i++ )                                        \
  { if ( (*this)[i] > iLV[i] ) return 0 ;                                       \
    if ( (*this)[i] < iLV[i] ) return 1 ;                                       \
  }                                                                             \
  return 1 ;                                                                    \
}
#endif


#ifndef CATLISTV_geOP
/** @nodoc */
#define defCATLISTV_geOP( T, N )
#else
/** @nodoc */
#define defCATLISTV_geOP( T, N )                                                \
int CATListVal##N::operator >= ( const CATListVal##N& iLV ) const               \
{                                                                               \
  if ( this == &iLV )        return 1 ;                                         \
  if ( Size() < iLV.Size() ) return 0 ;                                         \
  if ( Size() > iLV.Size() ) return 1 ;                                         \
                                                                                \
  for ( int  i = 1 ; i <= Size() ; i++ )                                        \
  { if ( (*this)[i] < iLV[i] ) return 0 ;                                       \
    if ( (*this)[i] > iLV[i] ) return 1 ;                                       \
  }                                                                             \
  return 1 ;                                                                    \
}
#endif


#ifndef CATLISTV_ltOP
/** @nodoc */
#define defCATLISTV_ltOP( T, N )
#else
/** @nodoc */
#define defCATLISTV_ltOP( T, N )                                                \
int CATListVal##N::operator < ( const CATListVal##N& iLV ) const                \
{ return ( *this >= iLV ? 0 : 1 ); }
#endif


#ifndef CATLISTV_gtOP
/** @nodoc */
#define defCATLISTV_gtOP( T, N )
#else
/** @nodoc */
#define defCATLISTV_gtOP( T, N )                                                \
int CATListVal##N::operator > ( const CATListVal##N& iLV ) const                \
{ return ( *this <= iLV ? 0 : 1 ); }
#endif


#ifndef CATLISTV_Compare
/** @nodoc */
#define defCATLISTV_Compare( T, N )
#else
/** @nodoc */
#define defCATLISTV_Compare( T, N )                                                      \
int CATListVal##N::Compare ( const CATListVal##N& iL1,                                   \
                             const CATListVal##N& iL2,                                   \
                             int (*iPFCompare) ( T*, T* ) )                              \
{                                                                                        \
  return CATListPV::Compare ( iL1._Lval, iL2._Lval, (CATCollec::PFCompare) iPFCompare ); \
}
#endif


#ifndef CATLISTV_Replace
/** @nodoc */
#define defCATLISTV_Replace( T, N )
#else
/** @nodoc */
#define defCATLISTV_Replace( T, N )                                                \
void CATListVal##N::Replace ( int iPos, const T& iReplace )                        \
{                                                                                  \
  T* pt = (T*) _Lval[iPos] ;                                                       \
  delete pt ;                                                                      \
  _Lval[iPos] = new T ( iReplace );                                                \
}
#endif


#ifndef CATLISTV_Swap
/** @nodoc */
#define defCATLISTV_Swap( T, N )
#else
/** @nodoc */
#define defCATLISTV_Swap( T, N )                                                \
void CATListVal##N::Swap ( int iPos1, int iPos2 )                               \
{ _Lval.Swap ( iPos1, iPos2 ); }
#endif


#ifndef CATLISTV_QuickSort
/** @nodoc */
#define defCATLISTV_QuickSort( T, N )
#else
/** @nodoc */
#define defCATLISTV_QuickSort( T, N )                                           \
void CATListVal##N::QuickSort ( int (*iPFCompare) ( T*, T* ) )                  \
{ _Lval.QuickSort ( (CATCollec::PFCompare) iPFCompare ); }
#endif


#ifndef CATLISTV_ArrayVals
/** @nodoc */
#define defCATLISTV_ArrayVals( T, N )
#else
/** @nodoc */
#define defCATLISTV_ArrayVals( T, N )                                           \
void CATListVal##N::Array ( T* ioArray ) const                                  \
{                                                                               \
  for ( int  i = Size(); i > 0; i-- )                                           \
    ioArray[i-1] = (*this)[i] ;                                                 \
}
#endif


#ifndef CATLISTV_ArrayPtrs
/** @nodoc */
#define defCATLISTV_ArrayPtrs( T, N )
#else
/** @nodoc */
#define defCATLISTV_ArrayPtrs( T, N )                                           \
void CATListVal##N::Array ( T** ioArray ) const                                 \
{                                                                               \
  for ( int  i = Size(); i > 0; i-- )                                           \
    ioArray[i-1] = (T*) ( _Lval[i] );                                           \
}
#endif


#ifndef CATLISTV_NbOccur
/** @nodoc */
#define defCATLISTV_NbOccur( T, N )
#else
/** @nodoc */
#define defCATLISTV_NbOccur( T, N )                                            \
int CATListVal##N::NbOccur ( const T& iTest )                                  \
{                                                                              \
  int nb_occ = 0 ;                                                             \
  for ( int  i = Size(); i > 0; i-- )                                          \
    if ( (*this)[i] == iTest ) nb_occ++ ;                                      \
  return  nb_occ ;                                                             \
}
#endif


#ifndef CATLISTV_Intersection
/** @nodoc */
#define defCATLISTV_Intersection( T, N )
#else
/** @nodoc */
#define defCATLISTV_Intersection( T, N )                                        \
void CATListVal##N::Intersection ( const CATListVal##N& iLV1,                   \
                                   const CATListVal##N& iLV2,                   \
                                   CATListVal##N& ioResult  )                   \
{                                                                               \
  for ( int i = iLV1.Size(); i > 0; i-- )                                       \
    if ( iLV2.Locate ( iLV1[i] ) )                                              \
      ioResult.Append ( iLV1[i] );                                              \
}
#endif


#ifndef CATLISTV_ApplyMemberFunct
/** @nodoc */
#define defCATLISTV_ApplyMemberFunct( T, N )
#else
/** @nodoc */
#define defCATLISTV_ApplyMemberFunct( T, N )                                        \
int CATListVal##N::ApplyMemberFunct ( PtrMbrFunct iPF,                              \
                                      int iFrom , int  iTo   ,                      \
                                      T** iPLast, int* iPRC  ) const                \
{                                                                                   \
  if ( ! iTo ) iTo = Size() ;                                                       \
  T* pt ;                                                                           \
  if ( ! iPLast ) iPLast = &pt ;                                                    \
  *iPLast = NULL ;                                                                  \
  int RC ;                                                                          \
  if ( ! iPRC ) iPRC = &RC ;                                                        \
  for ( int  i = iFrom ; i <= iTo ; i++ )                                           \
  {                                                                                 \
    *iPLast = (T*) _Lval[i] ;                                                       \
    if ( *iPRC = ((*iPLast)->*iPF)() )                                              \
      return i ;                                                                    \
  }                                                                                 \
  return 0 ;                                                                        \
}
#endif


#ifndef CATLISTV_ApplyMemberFunctConst
/** @nodoc */
#define defCATLISTV_ApplyMemberFunctConst( T, N )
#else
/** @nodoc */
#define defCATLISTV_ApplyMemberFunctConst( T, N )                                  \
int CATListVal##N::ApplyMemberFunctConst ( PtrMbrFunctConst iPF   ,                \
                                           int iFrom , int  iTo   ,                \
                                           T** iPLast, int* iPRC  ) const          \
{                                                                                  \
  if ( ! iTo ) iTo = Size() ;                                                      \
  T* pt ;                                                                          \
  if ( ! iPLast ) iPLast = &pt ;                                                   \
  *iPLast = NULL ;                                                                 \
  int RC ;                                                                         \
  if ( ! iPRC ) iPRC = &RC ;                                                       \
  for ( int  i = iFrom ; i <= iTo ; i++ )                                          \
  {                                                                                \
    *iPLast = (T*) _Lval[i] ;                                                      \
    if ( *iPRC = ((*iPLast)->*iPF)() )                                             \
      return i ;                                                                   \
  }                                                                                \
  return 0;                                                                        \
}
#endif

#ifndef CATLISTV_ApplyGlobalFunct
/** @nodoc */
#define defCATLISTV_ApplyGlobalFunct( T, N )
#else
/** @nodoc */
#define defCATLISTV_ApplyGlobalFunct( T, N )                                     \
int CATListVal##N::ApplyGlobalFunct ( PtrGlbFunct iPF        ,                   \
                                      int iFrom , int  iTo   ,                   \
                                      T** iPLast, int* iPRC  ) const             \
{                                                                                \
  if ( ! iTo ) iTo = Size() ;                                                    \
  T* pt ;                                                                        \
  if ( ! iPLast ) iPLast = &pt ;                                                 \
  *iPLast = NULL ;                                                               \
  int RC ;                                                                       \
  if ( ! iPRC ) iPRC = &RC ;                                                     \
  for ( int i = iFrom; i <= iTo; i++ )                                           \
  {                                                                              \
    *iPLast = (T*) _Lval[i] ;                                                    \
    if ( *iPRC = (*iPF)(*(*iPLast)) )                                            \
      return i ;                                                                 \
  }                                                                              \
  return 0 ;                                                                     \
}
#endif



#include  "CATListPV.h"
