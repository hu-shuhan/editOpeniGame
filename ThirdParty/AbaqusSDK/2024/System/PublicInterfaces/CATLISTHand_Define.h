
// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//
//      1- To produce definition of the functions bodies for CATListHand
//         ================================================================
//
////////////////////////////////////////////////////////////////////////////////

#include "CATAssert.h"
#include "CATDataType.h"

#ifndef CATLISTHand_DEFINE
/** @nodoc */
#define CATLISTHand_DEFINE(T) CATLISTHand_DEFINE_TNS(T, T, 1)
#endif

#ifndef CATLISTHand_DEFINE_TS
/** @nodoc */
#define CATLISTHand_DEFINE_TS(T, S) CATLISTHand_DEFINE_TNS(T, T, S)
#endif

#ifndef CATLISTHand_DEFINE_TN
/** @nodoc */
#define CATLISTHand_DEFINE_TN(T, N) CATLISTHand_DEFINE_TNS(T, N, 1)
#endif

#ifndef CATLISTHand_DEFINE_TNS
/** @nodoc */
#define CATLISTHand_DEFINE_TNS(T, N, S)			\
                                                                        \
const int CATListVal##N::_SBSize = S;                                   \
                                                                        \
/* fonction pour la hash table temporaire */ 		                \
int CompareHashListCollec##N( const void* s1, const void* s2)           \
{						                        \
/*IA64*/								\
/*    return ( (unsigned int)(s1) == (unsigned int)(s2) ) ? 0 : 1;*/	\
 return ((s1 == s2) ) ? 0 : 1;						\
}						                                    \
						                                    \
unsigned int ComputeHashListCollec##N( const void* s )                              \
{ return ( CATPtrToUINT32 (s)); }                                                    \
						                                    \
/*-------------------------------------------*/                                     \
/* Constructor                               */					    \
/*-------------------------------------------*/                                     \
CATListVal##N::CATListVal##N () : _Size (0)                                         \
{                                                                                   \
  _MaxSize = _SBSize;                                                               \
  _Block = _SBlock;                                                                 \
  memset ( &_SBlock[0], 0, sizeof(T)*_SBSize  ) ;                                   \
}                                                                                   \
							                            \
/*-------------------------------------------*/                                     \
/* Constructor                               */					    \
/*-------------------------------------------*/                                     \
CATListVal##N::CATListVal##N (int iInitAlloc) : _Size (0)                           \
{                                                                                   \
 /* Initial allocation (if requested) */                                            \
 if ( iInitAlloc > _SBSize ) {                                                      \
    _MaxSize = iInitAlloc;                                                          \
    GetStorage ( _MaxSize, _Block ) ;                                               \
    memset ( &_Block[0], 0, sizeof(T)*_MaxSize  ) ;                                 \
    }                                                                               \
 else {                                                                             \
    _MaxSize = _SBSize;                                                             \
    _Block = _SBlock;                                                               \
    }                                                                               \
 memset ( &_SBlock[0], 0, sizeof(T)*_SBSize  ) ;                                    \
}                                                                                   \
	                                                                            \
/*--------------------------------------------------------*/                        \
/* Constructeur qui ne sert a rien : je suis oblige de le */                        \
/* conserver car il est utilise en tant que tel           */                        \
/*--------------------------------------------------------*/                        \
CATListVal##N::CATListVal##N (int iInitAlloc, int iExtensAlloc) : _Size (0)         \
{                                                                                   \
 /* Initial allocation (if requested) */                                            \
 if ( iInitAlloc > _SBSize ) {                                                      \
    _MaxSize = iInitAlloc;                                                          \
    GetStorage ( _MaxSize, _Block ) ;                                               \
    memset ( &_Block[0], 0, sizeof(T)*_MaxSize  ) ;                                 \
    }                                                                               \
 else {                                                                             \
    _MaxSize = _SBSize;                                                             \
    _Block = _SBlock;                                                               \
    }                                                                               \
 memset ( &_SBlock[0], 0, sizeof(T)*_SBSize  ) ;                                    \
}                                                                                   \
					                                            \
/*-------------------------------------------*/                                     \
/* Copy Constructor                          */		 		            \
/*-------------------------------------------*/                                     \
CATListVal##N::CATListVal##N ( const CATListVal##N&  iCopy )                        \
	       :_Size ( iCopy._Size )                                               \
{                                                                                   \
 /* 1. initial allocation to required size (if necessary) */                        \
 if ( iCopy._Size > _SBSize ) {                                                     \
    _MaxSize = iCopy._Size ;                                                        \
    GetStorage ( _MaxSize, _Block ) ;                                               \
    memset ( &_Block[0], 0, sizeof(T)*_MaxSize  ) ;                                 \
    }                                                                               \
 else {                                                                             \
    _MaxSize = _SBSize ;                                                            \
    _Block = _SBlock ;                                                              \
    }                                                                              \
 memset ( &_SBlock[0], 0, sizeof(T)*_SBSize  ) ;                                   \
					            		                    \
  /* 2. copy elements */                                                            \
  for ( int i = 0 ; i < _Size ; i++ )                                               \
    _Block[i] = iCopy._Block[i];                                                    \
}                                                                                   \
	                                                                                \
/*-------------------------------------------*/                                     \
/* Destructor                                */                                     \
/*-------------------------------------------*/                                     \
CATListVal##N::~CATListVal##N ()                                                    \
{                                                                                   \
  for ( int i = _Size ; i > 0 ; i-- )                                               \
  {                                                                                 \
    CATBaseUnknown* pt = (CATBaseUnknown*)(_Block[i-1]) ;                           \
    if ( pt )                                                                       \
      pt->Release() ;                                                               \
  }                                                                                 \
                                                                                    \
  if (_MaxSize > _SBSize)                                                           \
	FreeStorage ( _Block ) ;                                                    \
                                                                                    \
  /* derniere etape : j'ai pris la responsabilite de l'effacement des handlers */   \
  /* dans la boucle precedente. !!! si _SBSize vaut 1, le destructeur de handler */ \
  /* va etre appele lors de l'effacement de _SBlock. Donc je mets a nul ce buffer*/ \
  memset ( &_SBlock[0], 0, sizeof(T)*_SBSize  ) ;                                   \
}                                                                                   \
                                                                                    \
/*-------------------------------------------*/                                     \
/* operator =                                */                                     \
/*-------------------------------------------*/                                     \
CATListVal##N& CATListVal##N::operator= ( const CATListVal##N& iCopy )           \
{                                                                                   \
 if ( &iCopy == this ) return *this ;                                               \
							                     	\
  /*-----------------------------------*/                                           \
  /* previous data cleaning            */                                           \
  /*-----------------------------------*/                                           \
  int i ;                                                                           \
  for ( i = _Size ; i > 0 ; i-- )                                                   \
    {                                                                               \
      CATBaseUnknown* pt = (CATBaseUnknown*)(_Block[i-1]) ;                         \
      if ( pt )                                                                     \
      {                                                                             \
        pt->Release() ;                                                             \
        memset ( &_Block[i-1], 0, sizeof(T) ) ;                                     \
      }                                                                             \
    }                                                                               \
                                                                                    \
  _Size = iCopy._Size ;                                                             \
  /* 1. if block is too small, reallocate it */                                     \
  if ( _Size > _MaxSize ) {                                                         \
					                                            \
    /* 1.1 get rid of old too little block */                                       \
    if ( _MaxSize > _SBSize ) {                                                     \
       FreeStorage ( _Block ) ;                                                     \
       }                                                                            \
						                                    \
    /* 1.2 allocate a new big enough block */                                       \
    _MaxSize = _Size ;                                                              \
    GetStorage ( _MaxSize, _Block ) ;                                               \
    memset ( &_Block[0], 0, sizeof(T)*_MaxSize ) ;                                  \
						                                    \
    }                                                                               \
					                                            \
  /* 2. copy elements */                                                            \
  for ( i = 0 ; i < _Size ; i++ )                                                   \
    _Block[i] = iCopy._Block[i] ;                                                   \
					                                            \
 return *this ;                                                                     \
}                                                                                   \
                                                                                    \
int CATListVal##N::Size () const                                                    \
{                                                                                   \
  return _Size;                                                                     \
}                                                                                   \
                                                                                    \
void CATListVal##N::Max ( int iSize )                                               \
{                                                                                   \
    if ( iSize > _MaxSize )                                                         \
	{                                                                               \
       /* 1.1 save old data */                                                      \
       int oldMaxSize = _MaxSize ;                                                  \
       T* oldBlock = _Block ;                                                       \
					                                       \
       /* 1.2 allocate a new big enough block */                                    \
       _MaxSize = iSize ;                                                           \
       GetStorage ( _MaxSize, _Block ) ;                                            \
					                                	    \
       /* 1.3 copy elements from initial block to reallocated block */              \
       memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T) * _Size ) ;       \
       memset ( &_Block[_Size], 0, sizeof(T)*(_MaxSize-_Size) ) ;                   \
					                                            \
       /* 1.4 get rid of old too little block */                                    \
       if ( oldMaxSize > _SBSize )                                                  \
	     FreeStorage ( oldBlock ) ;                                             \
	}                                                                           \
}                                                                                   \
                                                                                    \
                                                                                    \
T& CATListVal##N::operator[] (int iPos)                                             \
{                                                                                   \
  CATAssert ( iPos > 0 && iPos <= _Size ) ;                                         \
  return ( _Block[iPos-1] );                                                        \
}                                                                                   \
                                                                                     \
const T& CATListVal##N::operator[] (int iPos)  const                                \
{                                                                                   \
  CATAssert ( iPos > 0 && iPos <= _Size ) ;                                         \
  return ( _Block[iPos-1] );                                                        \
}                                                                                  \
                                                                                   \
void CATListVal##N::GetImplementationList (int iNbElem, IUnknown**& oBlock) const  \
{                                                                                  \
  oBlock = new IUnknown* [iNbElem] ;                                                \
  if ( iNbElem <= _Size )                                                           \
    for ( int i = 0 ; i < iNbElem ; i++ )                                           \
    {                                                                               \
                                                                                    \
      /* current object specific cast */                                            \
      CATBaseUnknown* iHandlerCast = (CATBaseUnknown*)(_Block[i]) ;                 \
      IUnknown *iu2 = NULL ;                                                        \
	                                                                            \
      if ( iHandlerCast )                                                           \
      {                                                                             \
	iHandlerCast->QueryInterface (CLSID_CATBaseUnknown,(void **)&iu2);                  \
	if (iu2) iu2->Release();                                                    \
      }                                                                             \
	oBlock[i] = iu2 ;                                                           \
    }                                                                               \
                                                                                    \
}                                                                                   \
                                                                                    \
                                                                                    \
/*---------------------------------------------------------*/                       \
/* optional functions (append, Insert, Locate, remove)     */                       \
/*---------------------------------------------------------*/                       \
defCATLISTHand_Append( T, N, S )                                                    \
                                                                                    \
defCATLISTHand_AppendList( T, N, S )                                                \
                                                                                    \
defCATLISTHand_InsertAt( T, N, S )                                                  \
                                                                                    \
defCATLISTHand_InsertAfter( T, N, S )                                               \
                                                                                    \
defCATLISTHand_ReSize( T, N, S )                                                    \
                                                                                    \
defCATLISTHand_Locate( T, N, S )                                                    \
                                                                                    \
defCATLISTHand_RemoveValue( T, N, S )                                               \
                                                                                    \
defCATLISTHand_RemoveList( T, N, S )                                                \
                                                                                    \
defCATLISTHand_RemovePosition( T, N, S )                                            \
                                                                                    \
defCATLISTHand_RemoveAll( T, N, S )                                                 \
                                                                                    \
defCATLISTHand_RemoveNulls( T, N, S )                                               \
                                                                                    \
defCATLISTHand_RemoveDuplicatesCount( T, N, S )                                     \
                                                                                    \
defCATLISTHand_RemoveDuplicatesExtract( T, N, S )                                   \
                                                                                    \
/*---------------------------------------------------------------------------*/     \
/* optional functions (equal operator, compare, swap, quicksort)             */     \
/*---------------------------------------------------------------------------*/     \
                                                                                    \
defCATLISTHand_neOP( T, N, S )                                                      \
					                         	            \
defCATLISTHand_eqOP( T, N, S )                                                      \
					                         	            \
/*defCATLISTHand_Compare( T, N, S )*/                                               \
										\
defCATLISTHand_Swap( T, N, S )                                                      \
										\
defCATLISTHand_Replace( T, N, S )                                                   \
					                                       \
defCATLISTHand_QuickSort( T, N, S )                                                 \
					                                    	\
defCATLISTHand_NbOccur( T, N, S )                                                   \
				                                	\
defCATLISTHand_Intersection( T, N, S )                                              \


#endif // CATLISTHand_DEFINE_TN

//---------------------------------------------------------------------------
// Append / AppendList
//-------------------------
#ifndef CATLISTHand_Append

#undef         defCATLISTHand_Append
/** @nodoc */
#define        defCATLISTHand_Append( T, N, S )

#else

#undef         defCATLISTHand_Append
/** @nodoc */
#define        defCATLISTHand_Append( T, N, S )                                    \
void                                                                                \
CATListVal##N::Append ( const T& iAdd )                                            \
{                                                                                   \
 /* 1. if block is too small, reallocate it */                                      \
 if ( _Size == _MaxSize ) {                                                         \
						                  		    \
    /* 1.1 save old data */                                                         \
    int oldMaxSize = _MaxSize ;                                                     \
    T* oldBlock = _Block ;                                                          \
					                          	            \
    /* 1.2 compute new block max size */                                            \
    _MaxSize = _MaxSize * 2;                                                        \
					                                   	    \
    /* 1.3 allocate a new big enough block */                                       \
    GetStorage ( _MaxSize, _Block ) ;                                               \
					                                      	    \
    /* 1.4 copy initial block to reallocated block */                               \
    memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T) * _Size ) ;          \
    /* !!! si je mets a null sur "_MaxSize" elements, c'est parce que */            \
    /* l'on multiplie par 2 */                                                      \
    memset ( &_Block[_Size], 0, sizeof(T)*oldMaxSize ) ;                            \
				                                	            \
    /* 1.5 get rid of old too little block */                                       \
    if ( oldMaxSize > _SBSize ) {                                                   \
       FreeStorage ( oldBlock ) ;                                                   \
       }                                                                            \
					                               	\
    }                                                                               \
					                                        \
 /* 2. add new element at the end */                                                \
 _Block[_Size++] = iAdd ;                                                    \
}
#endif



#ifndef CATLISTHand_AppendList

#undef         defCATLISTHand_AppendList
/** @nodoc */
#define        defCATLISTHand_AppendList( T, N, S )

#else

#undef         defCATLISTHand_AppendList
/** @nodoc */
#define        defCATLISTHand_AppendList( T, N, S )                                \
void CATListVal##N::Append ( const CATListVal##N& iConcat )                       \
{                                                                                   \
  /* 1. if block is too small, reallocate it */                                     \
  int newSize = _Size + iConcat._Size ;                                             \
  if ( newSize > _MaxSize )                                                         \
  {                                                                                 \
					                                     \
    /* 1.1 save old data */                                                         \
    int oldMaxSize = _MaxSize ;                                                     \
    T* oldBlock = _Block ;                                                          \
					                                            \
    /* 1.2 compute new block max size */                                            \
    _MaxSize = newSize;                                                             \
					                                            \
    /* 1.3 allocate a new big enough block */                                       \
    GetStorage ( _MaxSize, _Block ) ;                                               \
				                                                    \
    /* 1.4 copy initial block to reallocated block */                               \
    memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T) * _Size ) ;          \
    memset ( &_Block[_Size], 0, sizeof(T)*(_MaxSize - _Size) ) ;                    \
					                                     	    \
    /* 1.5 get rid of old too little block */                                       \
    if ( oldMaxSize > _SBSize )                                                     \
       FreeStorage ( oldBlock ) ;                                                   \
  }                                                                                 \
					                             	            \
  /* 2. add new elements at the end */                                              \
  for ( int i = 0 ; i < iConcat._Size ; i ++ )                                      \
    _Block[_Size++] = iConcat._Block[i] ;                                          \
                                                                                    \
}

#endif


//---------------------------------------------------------------------------
// InsertAt
//--------------------------------
#ifndef CATLISTHand_InsertAt

#undef         defCATLISTHand_InsertAt
/** @nodoc */
#define        defCATLISTHand_InsertAt( T, N, S )

#else

#undef         defCATLISTHand_InsertAt
/** @nodoc */
#define        defCATLISTHand_InsertAt( T, N, S )                                  \
void CATListVal##N::InsertAt ( int iPos, const T& iAdd )                            \
{                                                                                   \
 CATAssert ( iPos >= 1  &&  iPos <= _Size+1 ) ;                                        \
					                                            \
 /* 1. if block is too small, reallocate it */                                      \
 if ( _Size == _MaxSize ) {                                                         \
					                                            \
    /* 1.1 save old data */                                                         \
    int oldMaxSize = _MaxSize ;                                                     \
    T* oldBlock = _Block ;                                                          \
					                                            \
    /* 1.2 compute new block max size */                                            \
    _MaxSize = _MaxSize * 2;                                                        \
						                                    \
    /* 1.3 allocate a new big enough block */                                       \
    GetStorage ( _MaxSize, _Block ) ;                                               \
				                               	\
    /* 1.4 copy first elements from initial block to reallocated block */           \
    if ( iPos > 1 )                                                                 \
	  memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T) * (iPos - 1) );\
					                               	            \
    /* 1.5 insert new element */                                                    \
    memset ( &_Block[iPos-1], 0, sizeof(T) ) ;                                      \
    _Block[iPos-1] = iAdd ;                                                         \
				                                 	            \
    /* 1.6 copy last elements from initial block to reallocated block */            \
    if ( iPos <= _Size )                                                            \
       memcpy ( (void*) &_Block[iPos], (const void*) &oldBlock[iPos-1],             \
	             sizeof(T) * (_Size - iPos + 1) ) ;                             \
				                        		            \
    /* 1.7 get rid of old too little block */                                       \
    if ( oldMaxSize > _SBSize )                                                     \
       FreeStorage ( oldBlock ) ;                                                   \
				                        		            \
    /* 1.8 update end of buffer */                                                  \
    memset ( &_Block[_Size+1], 0, sizeof(T)*(oldMaxSize-1) ) ;                      \
    }                                                                               \
				                      	                            \
 /* 2. block is big enough */                                                       \
 else {                                                                             \
						                                 \
    /* 2.1 move last elements to create a hole */                                   \
    if ( iPos <= _Size )                                                            \
      memmove ( &_Block[iPos], &_Block[iPos-1], sizeof(T)*(_Size - iPos + 1) );     \
					                            	            \
    /* 2.2 insert element in hole */                                                \
    memset ( &_Block[iPos-1], 0, sizeof(T) ) ;                                      \
    _Block[iPos-1] = iAdd ;                                                         \
						                                    \
    }                                                                               \
						                   		\
 _Size ++ ;                                                                         \
}

#endif

#ifndef CATLISTHand_InsertAfter

#undef         defCATLISTHand_InsertAfter
/** @nodoc */
#define        defCATLISTHand_InsertAfter( T, N, S )

#else

#undef         defCATLISTHand_InsertAfter
/** @nodoc */
#define        defCATLISTHand_InsertAfter( T, N, S )                                  \
void CATListVal##N::InsertAfter ( int iPos, const T& iAdd )                           \
{                                                                                   \
  InsertAt (iPos+1, iAdd) ;                                                         \
}
#endif


//----------------------------------------------------------------------------
// Resize (optional)
//-------------------------------------------
#ifndef CATLISTHand_ReSize

#undef         defCATLISTHand_ReSize
/** @nodoc */
#define        defCATLISTHand_ReSize( T, N, S )

#else

#undef         defCATLISTHand_ReSize
/** @nodoc */
#define        defCATLISTHand_ReSize( T, N, S )                                    \
void                                                                                \
CATListVal##N::Size ( int iSize, const T* iFiller )                                 \
{                                                                                   \
  if ( iSize == _Size )                                                             \
    return ;                                                                        \
  else if ( iSize < _Size )                                                         \
  {                                                                                 \
	/* upper value cleanings */                                                 \
    for ( int i = iSize+1 ; i <= _Size ; i++ )                                      \
	{                                                                           \
	  CATBaseUnknown* pt = (CATBaseUnknown*)(_Block[i-1]) ;                     \
	  if (  pt )	                                                              \
	  {                                                                         \
	    pt->Release() ;                                                         \
	  }                                                                         \
	}                                                                           \
    memset ( &_Block[iSize], 0, sizeof(T)*(_Size-iSize) ) ;                         \
  }                                                                                 \
  else                                                                              \
  {                                                                                 \
    /* 1. if block is too small, reallocate it */                                   \
    if ( iSize > _MaxSize )                                                         \
    {                                                                               \
       /* 1.1 save old data */                                                      \
       int oldMaxSize = _MaxSize ;                                                  \
       T* oldBlock = _Block ;                                                       \
					                                 	    \
       /* 1.2 allocate a new big enough block */                                    \
       _MaxSize = iSize ;                                                           \
       GetStorage ( _MaxSize, _Block ) ;                                            \
			                                	                    \
       /* 1.3 copy elements from initial block to reallocated block */              \
       memcpy ( (void*) _Block, (const void*) oldBlock, sizeof(T) * _Size ) ;      \
					                                            \
       /* 1.4 set values */                                                         \
       memset ( &_Block[_Size], 0, sizeof(T)*(iSize - _Size) ) ;                    \
			                                                            \
       /* 1.5 get rid of old too little block */                                    \
       if ( oldMaxSize > _SBSize )                                                  \
	     FreeStorage ( oldBlock ) ;                                                 \
	                                                                                \
    }                                                                            \
						                                    \
    if ( iFiller )                                                                  \
      {                                                                             \
        for ( int i = _Size ; i < iSize ; i++ )		                            \
	  _Block[i] = *iFiller ;		                            \
      }                                                                             \
  }                                                                                 \
					                                	    \
  _Size = iSize ;                                                                    \
}

#endif


//----------------------------------------------------------------------------
// Locate (optional)
//-------------------------------------------
#ifndef CATLISTHand_Locate

#undef         defCATLISTHand_Locate
/** @nodoc */
#define        defCATLISTHand_Locate( T, N, S )

#else

#undef         defCATLISTHand_Locate
/** @nodoc */
#define        defCATLISTHand_Locate( T, N, S )                                    \
int CATListVal##N::Locate ( const T& iLocate, int iFrom ) const                    \
{                                                                                   \
  if ( iFrom <= _Size )                                                             \
  {                                                                                 \
    CATBaseUnknown* iLocateCast = (CATBaseUnknown*)iLocate ;                        \
    IUnknown *iu1 = NULL;                                                           \
    if ( iLocateCast )                                                              \
    {                                                                               \
      iLocateCast->QueryInterface (CLSID_CATBaseUnknown,(void **)&iu1);                     \
      if (iu1) iu1->Release();                                                      \
    }                                                                               \
                                                                                    \
    for ( int i = iFrom ; i <= _Size ; i++ )                                        \
	{                                                                               \
	  /* current object specific cast */                                            \
	  CATBaseUnknown* iHandlerCast = (CATBaseUnknown*)(_Block[i-1]) ;           \
	  IUnknown *iu2 = NULL ;                                                        \
	                                                                                \
	  if ( iHandlerCast )                                                           \
	  {                                                                             \
	    iHandlerCast->QueryInterface (CLSID_CATBaseUnknown,(void **)&iu2);                  \
	    if (iu2) iu2->Release();                                                    \
	  }                                                                             \
                                                                                    \
	  if ( iu1 == iu2 )                                                         \
            return i ;                                                              \
	}                                                                           \
  }                                                                                 \
                                                                                    \
  return  0 ;                                                                       \
                                                                                    \
}

#endif


//-------------------------------------------------------------------------
// RemoveValue
//--------------------------------------
#ifndef CATLISTHand_RemoveValue

#undef         defCATLISTHand_RemoveValue
/** @nodoc */
#define        defCATLISTHand_RemoveValue( T, N, S )

#else

#undef         defCATLISTHand_RemoveValue
/** @nodoc */
#define        defCATLISTHand_RemoveValue( T, N, S )                               \
int                                                                                 \
CATListVal##N::RemoveValue ( const T& iRemove )                                    \
{                                                                                   \
 int pos = Locate ( iRemove ) ;                                                     \
 if ( pos )                                                                         \
    RemovePosition ( pos ) ;                                                        \
					                                      \
 return pos ;                                                                       \
}

#endif


//-------------------------------------------------------------------------
// RemoveList
//-------------------------------------
#ifndef CATLISTHand_RemoveList

#undef         defCATLISTHand_RemoveList
/** @nodoc */
#define        defCATLISTHand_RemoveList( T, N, S )

#else

#undef         defCATLISTHand_RemoveList
/** @nodoc */
#define        defCATLISTHand_RemoveList( T, N, S )                                \
int CATListVal##N::Remove ( const CATListVal##N& iSubstract )                     \
{                                                                                   \
 /* crazy case */                                                                   \
 if ( &iSubstract == this ) {                                                       \
    int size = _Size ;                                                              \
    RemoveAll() ;                                                                   \
    return size ;                                                                   \
    }                                                                               \
					                                        \
 int count = 0 ;                                                                    \
 for ( int i = 0 ; i < iSubstract._Size ; i++ )                                     \
    if ( RemoveValue (iSubstract._Block[i]) )                                    \
       count ++ ;                                                                   \
					                              	\
 return count ;                                                                     \
}

#endif


//------------------------------------------------------------------------
//  RemovePosition
//-----------------------------------------
#ifndef CATLISTHand_RemovePosition

#undef         defCATLISTHand_RemovePosition
/** @nodoc */
#define        defCATLISTHand_RemovePosition( T, N, S )

#else

#undef         defCATLISTHand_RemovePosition
/** @nodoc */
#define        defCATLISTHand_RemovePosition( T, N, S )                            \
void                                                                                \
CATListVal##N::RemovePosition ( int iPos )                                         \
{                                                                                   \
/*#ifndef	CNEXT_DEBUG */                                                          \
/*#else  */                                                                         \
  CATAssert ( iPos > 0  &&  iPos <= _Size ) ;                                          \
/*#endif   */                                                                       \
						                           	    \
 /* call to destructor */                                                           \
 CATBaseUnknown* pt = (CATBaseUnknown*)(_Block[iPos - 1]);                          \
 if ( pt )                                                                          \
   pt->Release();                                                                   \
						                          	    \
 /* fill the gap (if necessary) */                                                  \
 if ( iPos < _Size )                                                                \
    memmove ( &_Block[iPos-1], &_Block[iPos], sizeof(T) * (_Size - iPos) ) ;        \
					                                            \
 _Size -- ;                                                                         \
 memset (&_Block[_Size], 0, sizeof(T)) ;                                            \
}

#endif


//--------------------------------------------------------------------------
// RemoveAll
//--------------------
#ifndef CATLISTHand_RemoveAll

#undef         defCATLISTHand_RemoveAll
/** @nodoc */
#define        defCATLISTHand_RemoveAll( T, N, S )

#else

#undef         defCATLISTHand_RemoveAll
/** @nodoc */
#define        defCATLISTHand_RemoveAll( T, N, S )                                 \
void                                                                                \
CATListVal##N::RemoveAll ( CATCollec::MemoryHandling iMH )                          \
{                                                                                   \
  for ( int i = 0 ; i < _Size ; i++ )                                               \
  {                                                                                 \
    CATBaseUnknown* pt =  (CATBaseUnknown*)(_Block[i]) ;                            \
    if ( pt )                                                                       \
      pt->Release();                                                                \
  }                                                                                 \
					                           	            \
  if ( iMH == CATCollec::ReleaseAllocation ) {                                      \
    if ( _MaxSize > _SBSize )                                                       \
      {                                                                             \
        FreeStorage ( _Block ) ;                                                    \
        _Block = _SBlock ;                                                          \
        _MaxSize = _SBSize ;                                                        \
      }                                                                             \
       memset (&_SBlock[0], 0, sizeof(T)*_SBSize) ;                                 \
    }                                                                               \
  else                                                                              \
    memset (&_Block[0], 0, sizeof(T)*_Size) ;                                       \
					                           	            \
 _Size = 0 ;                                                                        \
}

#endif


//
//                      RemoveNulls
//                      ---------
//

#ifndef CATLISTHand_RemoveNulls

#undef         defCATLISTHand_RemoveNulls
/** @nodoc */
#define        defCATLISTHand_RemoveNulls( T, N, S )

#else

#undef         defCATLISTHand_RemoveNulls
/** @nodoc */
#define        defCATLISTHand_RemoveNulls( T, N, S )                               \
int                                                                                 \
CATListVal##N::RemoveNulls ( )                                                     \
{                                                                                   \
 int count = 0 ;                                                                    \
 int pos = 1 ;                                                                      \
 int nullPos = 0 ;                                                                  \
 T nullHand = NULL_var ;                                                            \
 do {                                                                               \
    nullPos = Locate ( nullHand, pos ) ;                                            \
    if ( nullPos ) {                                                                \
       RemovePosition ( nullPos ) ;                                                 \
       count ++ ;                                                                   \
       pos = nullPos ;                                                              \
       }                                                                            \
    } while ( nullPos ) ;                                                           \
				          		\
 return count ;                                                                     \
}

#endif


//-------------------------------------------------------------------------------
// RemoveDuplicates
//---------------------------
#ifndef CATLISTHand_RemoveDuplicatesCount

#undef         defCATLISTHand_RemoveDuplicatesCount
/** @nodoc */
#define        defCATLISTHand_RemoveDuplicatesCount( T, N, S )

#else

#undef         defCATLISTHand_RemoveDuplicatesCount
/** @nodoc */
#define        defCATLISTHand_RemoveDuplicatesCount( T, N, S )                     \
int CATListVal##N::RemoveDuplicates ()                                              \
{                                                                                   \
  int count = 0 ;                                                                   \
  if ( _Size <= 1 )                                                                 \
    return 0 ;                                                                      \
    				                                                    \
  CATHashTable MyTempHashtable (ComputeHashListCollec##N, CompareHashListCollec##N,_Size);\
				                                                    \
  for ( int i = 0 ; i < _Size ; i++ )                                               \
    {                                                                               \
                                                                                    \
      /* current object specific cast */                                            \
      CATBaseUnknown* iHandlerCast = (CATBaseUnknown*)(_Block[i]) ;                 \
      IUnknown *iu = NULL ;                                                         \
	                                                                            \
      if ( iHandlerCast )                                                           \
      {                                                                             \
	iHandlerCast->QueryInterface (CLSID_CATBaseUnknown,(void **)&iu);                   \
	if (iu) iu->Release();                                                      \
      }                                                                             \
      if ( iu && !MyTempHashtable.Insert(iu) )                                      \
      {                                                                             \
	/* call to destructor */                                                    \
	CATBaseUnknown* pt = (CATBaseUnknown*)(_Block[i]);                          \
	if ( pt )                                                                   \
	  pt->Release();                                                            \
                                                                                    \
	count++;                                                                    \
      }                                                                             \
      else                                                                          \
      {                                                                             \
	if ( count )                                                                \
	  memcpy ( &_Block[i-count], &_Block[i], sizeof(T) ) ;                      \
      }                                                                             \
    }                                                                               \
                                                                                    \
  if ( count )                                                                      \
  {                                                                                 \
    _Size -= count ;                                                                \
    memset (&_Block[_Size], 0, sizeof(T)*count);                                    \
  }                                                                                 \
                                                                                    \
  return count ;                                                                    \
}

#endif


#ifndef CATLISTHand_RemoveDuplicatesExtract

#undef         defCATLISTHand_RemoveDuplicatesExtract
/** @nodoc */
#define        defCATLISTHand_RemoveDuplicatesExtract( T, N, S )

#else

#undef         defCATLISTHand_RemoveDuplicatesExtract
/** @nodoc */
#define        defCATLISTHand_RemoveDuplicatesExtract( T, N, S )                   \
void CATListVal##N::RemoveDuplicates (CATListVal##N& ioExtract)                     \
{                                                                                   \
  if ( _Size <= 1 )                                                                 \
    return ;                                                                        \
    				                                                    \
  int count = 0 ;			                                            \
  CATHashTable MyTempHashtable (ComputeHashListCollec##N, CompareHashListCollec##N,_Size);\
				                                                    \
  for ( int i = 0 ; i < _Size ; i++ )                                               \
    {                                                                               \
                                                                                    \
      /* current object specific cast */                                            \
      CATBaseUnknown* iHandlerCast = (CATBaseUnknown*)(_Block[i]) ;                 \
      IUnknown *iu = NULL ;                                                         \
	                                                                            \
      if ( iHandlerCast )                                                           \
      {                                                                             \
	iHandlerCast->QueryInterface (CLSID_CATBaseUnknown,(void **)&iu);                   \
	if (iu) iu->Release();                                                      \
      }                                                                             \
      if ( iu && !MyTempHashtable.Insert(iu) )                                      \
      {                                                                             \
        ioExtract.Append (_Block[i]);                                               \
	/* call to destructor */                                                    \
	CATBaseUnknown* pt = (CATBaseUnknown*)(_Block[i]);                          \
	if ( pt )                                                                   \
	  pt->Release();                                                            \
                                                                                    \
	count++;                                                                    \
      }                                                                             \
      else                                                                          \
      {                                                                             \
	if ( count )                                                                \
	  memcpy ( &_Block[i-count], &_Block[i], sizeof(T) ) ;                      \
      }                                                                             \
    }                                                                               \
                                                                                    \
  if ( count )                                                                      \
  {                                                                                 \
    _Size -= count ;                                                                \
    memset (&_Block[_Size], 0, sizeof(T)*count);                                    \
  }                                                                                 \
}
#endif


//-----------------------------------------------------------------------------------
// Non Equal operator
//---------------------------------
#ifndef CATLISTHand_neOP

#undef         defCATLISTHand_neOP
/** @nodoc */
#define        defCATLISTHand_neOP( T, N, S )

#else

#undef         defCATLISTHand_neOP
/** @nodoc */
#define        defCATLISTHand_neOP( T, N, S )                                      \
int                                                                                 \
CATListVal##N::operator != ( const CATListVal##N& iL ) const                      \
{                                                                                   \
 /* check if same object */                                                         \
 if ( &iL == this ) return  0 ;                                                     \
					                                     \
 /* check if size is different */                                                   \
 if ( _Size != iL._Size ) return  1 ;                                               \
				                 		\
 /* if one element is different from its corresponding element, */                  \
 /* the lists are different */                                                      \
 /* Rem : je conserve l'algo precedent, algo qui ne tient pas   */                  \
 /* compte d'un eventuel reordonnement des  elements            */                  \
 for ( int i = 0  ;  i < _Size  ;  i++ )                                            \
    if ( _Block[i] != iL._Block[i] ) return 1 ;                                     \
				                                                    \
 /* no difference found */                                                          \
 return 0 ;                                                                         \
}
#endif

//-----------------------------------------------------------------------------------
// Equal operator
//---------------------------------
#ifndef CATLISTHand_eqOP

#undef         defCATLISTHand_eqOP
/** @nodoc */
#define        defCATLISTHand_eqOP( T, N, S )

#else

#undef         defCATLISTHand_eqOP
/** @nodoc */
#define        defCATLISTHand_eqOP( T, N, S )                                      \
int                                                                                 \
CATListVal##N::operator == ( const CATListVal##N& iL ) const                        \
{   return ( *this != iL  ?  0  :  1 );  }
#endif

//----------------------------------------------------------------------------------
//  Swap
//------------------------------------
#ifndef CATLISTHand_Swap

#undef         defCATLISTHand_Swap
/** @nodoc */
#define        defCATLISTHand_Swap( T, N, S )

#else

#undef         defCATLISTHand_Swap
/** @nodoc */
#define        defCATLISTHand_Swap( T, N, S )                                      \
void CATListVal##N::Swap ( int iPos1, int iPos2 )                                   \
{                                                                                   \
 CATAssert ( iPos1 > 0  &&  iPos1 <= _Size  &&  iPos2 > 0  &&  iPos2 <= _Size );       \
 if ( iPos1 == iPos2 ) return ;                                                     \
						                                    \
 CATBaseUnknown* tempo = (CATBaseUnknown*)(_Block[iPos1-1]) ;                       \
 memcpy ( &_Block[iPos1-1], &_Block[iPos2-1], sizeof(T) ) ;                         \
 memcpy ( &_Block[iPos2-1], (void*)&tempo, sizeof(T) ) ;                             \
}

#endif


//-----------------------------------------------------------------------------------
// Replace
//------------------------------------
#ifndef CATLISTHand_Replace

#undef         defCATLISTHand_Replace
/** @nodoc */
#define        defCATLISTHand_Replace( T, N, S )

#else

#undef         defCATLISTHand_Replace

/** @nodoc */
#define        defCATLISTHand_Replace( T, N, S )                               \
void CATListVal##N::Replace ( int iPos, const T& iReplace )                     \
{                                                                               \
  CATAssert ( iPos > 0  &&  iPos <= _Size );                                       \
  CATBaseUnknown* pt =  (CATBaseUnknown*)(_Block[iPos-1]) ;                     \
  if ( pt )                                                                     \
  {                                                                             \
    pt->Release();                                                              \
    memset (&_Block[iPos-1], 0, sizeof(T));                                     \
  }                                                                             \
                                                                                \
  _Block[iPos-1] = iReplace ;					        	\
}

#endif

//-----------------------------------------------------------------------------------
// QuickSort
//------------------------------------
#ifndef CATLISTHand_QuickSort

#undef         defCATLISTHand_QuickSort
/** @nodoc */
#define        defCATLISTHand_QuickSort( T, N, S )

#else

#undef         defCATLISTHand_QuickSort

/** @nodoc */
#define        defCATLISTHand_QuickSort( T, N, S )                                 \
void CATListVal##N::QuickSort ( int (*iPFCompare)( T*, T* ) )                       \
{                                                                                   \
  CATVecCxxSort(_Block, _Size, iPFCompare) ;\
}

#endif


//----------------------------------------------------------------------------------
// NbOccur
//---------------------------------
#ifndef CATLISTHand_NbOccur

#undef         defCATLISTHand_NbOccur
/** @nodoc */
#define        defCATLISTHand_NbOccur( T, N, S )

#else

#undef         defCATLISTHand_NbOccur
/** @nodoc */
#define        defCATLISTHand_NbOccur( T, N, S )                                   \
int CATListVal##N::NbOccur ( const T& iTest )                                      \
{                                                                                   \
  int nb_occ = 0 ;                                                                  \
  CATBaseUnknown* iTestCast = (CATBaseUnknown*)iTest ;                              \
  IUnknown *iu1 = NULL;                                                             \
  if ( iTestCast )                                                                 \
  {                                                                                 \
    iTestCast->QueryInterface(CLSID_CATBaseUnknown,(void **)&iu1);                          \
    if (iu1) iu1->Release();                                                        \
  }                                                                                 \
                                                                                    \
  for ( int i = 0 ; i < _Size ; i++ )                                               \
  {                                                                                 \
    CATBaseUnknown* iHandlerCast = (CATBaseUnknown*)(_Block[i]) ;                   \
    IUnknown *iu2 = NULL;                                                           \
    if ( iHandlerCast )                                                             \
	{                                                                           \
	  iHandlerCast->QueryInterface(CLSID_CATBaseUnknown,(void **)&iu2);                 \
	  if (iu2) iu2->Release();                                                  \
	}					                                    \
  			                                                            \
    if ( iu1 == iu2 )                                                               \
      nb_occ ++ ;                                                                   \
  }                                                                                 \
                                                                                    \
  return  nb_occ ;                                                                  \
}

#endif


//----------------------------------------------------------------------------------
// Intersection
//-------------------------------
#ifndef CATLISTHand_Intersection

#undef         defCATLISTHand_Intersection
/** @nodoc */
#define        defCATLISTHand_Intersection( T, N, S )

#else

#undef         defCATLISTHand_Intersection
/** @nodoc */
#define        defCATLISTHand_Intersection( T, N, S )                              \
void                                                                                \
CATListVal##N::Intersection ( const CATListVal##N& iL1,                           \
			      const CATListVal##N& iL2,                                        \
			      CATListVal##N& ioResult )                                        \
{                                                                                   \
  /* crazy case */                                                                  \
  if ( &iL1 == &iL2 )                                                               \
  {                                                                                 \
    ioResult.Append ( iL1 );                                                        \
    return ;                                                                        \
  }                                                                                 \
                                                                                    \
  /* easy case */                                                                   \
  if ( ( iL1._Size == 0 ) || ( iL2._Size == 0 ) )                                   \
    return ;                                                                        \
                                                                                    \
  int size1 = iL1._Size ; /* avoid mistakes if ( &iL1 == &ioResult ) */             \
  int size2 = iL2._Size ; /* avoid mistakes if ( &iL2 == &ioResult ) */             \
                                                                                    \
  /*-------------------------------------------------*/                             \
  /* implementation list recovery                    */                             \
  /*-------------------------------------------------*/                             \
  IUnknown** MyImplBlock1 = NULL ;                                                  \
  iL1.GetImplementationList (size1, MyImplBlock1) ;                                 \
  IUnknown** MyImplBlock2 = NULL ;                                                  \
  iL2.GetImplementationList (size2, MyImplBlock2) ;                                 \
                                                                                    \
  for ( int k = size1 -1 ; k >= 0 ; k-- )                                           \
  {                                                                                 \
	IUnknown* CurrentImpl = MyImplBlock1[k] ;                                   \
    for ( int l = 0 ; l < size2 ; l++ )                                             \
    {                                                                               \
	  if ( CurrentImpl == MyImplBlock2[l] )                                     \
	  {                                                                         \
	    ioResult.Append (iL1._Block[k]);                                        \
	    break ;                                                                 \
	  }                                                                         \
    }                                                                               \
  }							                            \
                                                                                    \
  delete [] MyImplBlock1 ;                                                          \
  delete [] MyImplBlock2 ;                                                          \
}

#endif


#include "CATBaseUnknown.h"
#include "CATHashTable.h"
#include "CATVecCxxSort.h"
#include <string.h>
