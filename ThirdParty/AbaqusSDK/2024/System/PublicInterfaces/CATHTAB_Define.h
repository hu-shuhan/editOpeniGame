
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#ifndef CATHTAB_DEFINE
/**
 * @nodoc 
 * Macros to define classes which implement hash tables of pointers 
 * to a user-defined type X.
 * <br><b>Role:</b> The functionalities supported by the hash table class
 * can be turned on and off depending on your needs, using #define
 * symbols (if the proper #defined symbol is present, the
 * functionality is added to the class, otherwise it is omitted).
 * There is a trade-off between code-size and functionalities:
 * the more functionalities, the larger the code size.
 * The list of available functionalities is defined in 
 * <tt>CATHTAB_AllFunct.h</tt>
 */
#define CATHTAB_DEFINE( T ) CATHTAB_DEFINE_TN( T, T )
#endif	
	
#ifndef CATHTAB_DEFINE_TN
/**
 * @nodoc
 */
#define CATHTAB_DEFINE_TN( T, N )                                      \
defCATHTAB_Dtor( T, N )                                                \
                                                                       \
CATHashTab##N& CATHashTab##N::operator= ( const CATHashTab##N& iCopy ) \
{                                                                      \
  if ( &iCopy == this ) return *this ;                                 \
  _HT = iCopy._HT;                                                     \
  return  *this ;                                                      \
}                                                                      \
                                                                       \
defCATHTAB_ApplyMemberFunct( T, N )                                    \
defCATHTAB_ApplyMemberFunctConst( T, N )                               \
defCATHTAB_ApplyDelete( T, N )

#endif

#undef  defCATHTAB_Dtor
#undef  defCATHTAB_ApplyMemberFunct
#undef  defCATHTAB_ApplyMemberFunctConst
#undef  defCATHTAB_ApplyDelete

#ifndef __hpux
/**
 * @nodoc
 */
#define defCATHTAB_Dtor( T, N )
#else
/**
 * @nodoc
 */
#define defCATHTAB_Dtor( T, N )                             \
CATHashTab##N::~CATHashTab##N () { }
#endif

#ifndef  CATHTAB_ApplyMemberFunct
/**
 * @nodoc
 */
#define defCATHTAB_ApplyMemberFunct( T, N )
#else
/**
 * @nodoc
 */
#define defCATHTAB_ApplyMemberFunct( T, N )                                    \
int CATHashTab##N::ApplyMemberFunct ( PtrMbrFunct iPF,                         \
                                      T*   iFrom     ,                         \
                                      T**  iPLast    ,                         \
                                      int* iPRC      ) const                   \
{                                                                              \
  T* pt ;                                                                      \
  if ( ! iPLast ) iPLast = &pt ;                                               \
  *iPLast = NULL ;                                                             \
  int RC;                                                                      \
  if ( ! iPRC ) iPRC = &RC;                                                    \
  *iPRC = 0 ;                                                                  \
  int oBucket   = 1 ;                                                          \
  int oPosition = 0 ;                                                          \
  if ( iFrom )                                                                 \
    *iPLast = (T*)_HT.LocatePosition ( (void*)iFrom, oBucket, oPosition );     \
  else                                                                         \
    *iPLast = (T*)_HT.Next ( oBucket, oPosition );                             \
  int count = 0 ;                                                              \
  do                                                                           \
  {                                                                            \
    count++ ;                                                                  \
    *iPRC = ((*iPLast)->*iPF)() ;                                              \
    if ( *iPRC )                                                               \
      return count ;                                                           \
    else                                                                       \
    {                                                                          \
      *iPLast = (T*)_HT.Next ( oBucket, oPosition );                           \
      if ( ! *iPLast ) return count ;                                          \
    }                                                                          \
  }                                                                            \
  while ( 1 );                                                                 \
}
#endif

#ifndef CATHTAB_ApplyMemberFunctConst
/**
 * @nodoc
 */
#define defCATHTAB_ApplyMemberFunctConst( T, N )
#else
/**
 * @nodoc
 */
#define defCATHTAB_ApplyMemberFunctConst( T, N )                               \
int CATHashTab##N::ApplyMemberFunctConst ( PtrMbrFunctConst iPF,               \
                                           T*   iFrom          ,               \
                                           T**  iPLast         ,               \
                                           int* iPRC           ) const         \
{                                                                              \
  T* pt ;                                                                      \
  if ( ! iPLast ) iPLast = &pt ;                                               \
  *iPLast = NULL ;                                                             \
  int RC ;                                                                     \
  if ( ! iPRC )   iPRC = &RC ;                                                 \
  *iPRC = 0 ;                                                                  \
  int oBucket   = 1 ;                                                          \
  int oPosition = 0 ;                                                          \
  if ( iFrom )                                                                 \
    *iPLast = (T*)_HT.LocatePosition ( (void*)iFrom, oBucket, oPosition );     \
  else                                                                         \
    *iPLast = (T*)_HT.Next ( oBucket, oPosition );                             \
  int count = 0 ;                                                              \
  do                                                                           \
  {                                                                            \
    count++ ;                                                                  \
    *iPRC = ((*iPLast)->*iPF)() ;                                              \
    if ( *iPRC )                                                               \
      return count ;                                                           \
    else                                                                       \
    {                                                                          \
      *iPLast = (T*)_HT.Next ( oBucket, oPosition );                           \
      if ( ! *iPLast ) return count ;                                          \
    }                                                                          \
  }                                                                            \
  while ( 1 );                                                                 \
}
#endif

#ifndef CATHTAB_ApplyDelete
/**
 * @nodoc
 */
#define defCATHTAB_ApplyDelete( T, N )
#else
/**
 * @nodoc
 */
#define defCATHTAB_ApplyDelete( T, N )                                         \
void CATHashTab##N::ApplyDelete ()                                             \
{                                                                              \
  T* pt;                                                                       \
  int oBucket   = 1 ;                                                          \
  int oPosition = 0 ;                                                          \
  do                                                                           \
  {                                                                            \
    pt = (T*)_HT.Next ( oBucket, oPosition );                                  \
    delete pt ;                                                                \
  }                                                                            \
  while ( pt );                                                                \
  RemoveAll() ;                                                                \
}
#endif

