
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
#include "CxxSupport.h"	// DS_CXX11_SUPPORT_MOVE_SEMANTIC

#ifndef CATHTAB_DECLARE
/** 
 * @nodoc 
 * Declares classe which implement hash table of pointers to a user-defined type.
 * <br><b>Role:</b> The functionalities supported by the hash table class
 * can be turned on and off depending on your needs, using #define
 * symbols (if the proper #defined symbol is present, the
 * functionality is added to the class, otherwise it is omitted).
 * There is a trade-off between code-size and functionalities:
 * the more functionalities, the larger the code size.
 * The list of available functionalities is defined in 
 * <tt>CATHTAB_AllFunct.h</tt>
 * @param T The user-defined type.
 */
#define CATHTAB_DECLARE( T ) class T; CATHTAB_DECLARE_TN( T, T )
#endif

#ifndef CATCOLLEC_ExportedBy
/**
 * CATCOLLEC_ExportedBy is used for WindowsNT import/export DDL specifications.
 * <b>Role:</b> Should be defined to ExportedByXX0MODUL where XX0MODUL is the module containing the hash table.
 */
#define CATCOLLEC_ExportedBy
#endif

#ifndef CATHTAB_DECLARE_TN
/**
 * @nodoc
 */
#define CATHTAB_DECLARE_TN( T, N )                                             \
                                                                               \
class CATCOLLEC_ExportedBy CATHashTab##N : public CATCollecRoot                \
{                                                                              \
  public :                                                                     \
    typedef unsigned int (*PFHash)    ( T* );                                  \
    typedef int          (*PFCompare) ( T*, T* );                              \
    CATHashTab##N ( PFHash iPFH, PFCompare iPFC, int iDimension = 10 )         \
        : CATCollecRoot(), _HT ( (CATCollec::PFHash) iPFH,                     \
                (CATCollec::PFCompare) iPFC,                                   \
                iDimension )                                                   \
    {}                                                                         \
	CATHashTab##N ( PFHash iPFH, PFCompare iPFC, int iDimension, void * iOptions ) \
        : CATCollecRoot(), _HT ( (CATCollec::PFHash) iPFH,                     \
                (CATCollec::PFCompare) iPFC,                                   \
                iDimension, iOptions )                                         \
    {}                                                                         \
    CATHashTab##N ( const CATHashTab##N& iCopy )                               \
        : CATCollecRoot(), _HT ( iCopy._HT )                                   \
    {}                                                                         \
    dclCATHTAB_Dtor( T, N )                                                    \
                                                                               \
    CATHashTab##N& operator= ( const CATHashTab##N& iCopy );                   \
                                                                               \
    dclCATHTAB_Move( T, N )                                                    \
                                                                               \
    inline int Insert ( T* iAdd )                                              \
    { return _HT.Insert ( (void*)iAdd ); }                                     \
                                                                               \
    dclCATHTAB_Dimension( T, N )                                               \
    inline int Size () const { return _HT.Size(); }                            \
    dclCATHTAB_Rebuild( T, N )                                                 \
                                                                               \
    dclCATHTAB_Collisions( T, N )                                              \
    dclCATHTAB_Unused( T, N )                                                  \
                                                                               \
    inline CATHashCodeIter CreateIterator() const                              \
        { CATHashCodeIter ITR ( _HT ); return ITR ; }                          \
                                                                               \
    inline T* operator[] ( const CATHashCodeIter& iPos ) const                 \
    { return  ( (T*) _HT[iPos] ); }                                            \
                                                                               \
    dclCATHTAB_KeyLocate( T, N )                                               \
    dclCATHTAB_KeyLocatePosition( T, N )                                       \
    dclCATHTAB_Locate( T, N )                                                  \
    dclCATHTAB_LocatePosition( T, N )                                          \
    dclCATHTAB_Retrieve( T, N )                                                \
    dclCATHTAB_Next( T, N )                                                    \
    dclCATHTAB_NextPosition( T, N )                                            \
                                                                               \
    dclCATHTAB_Remove( T, N )                                                  \
    dclCATHTAB_RemovePosition( T, N )                                          \
    dclCATHTAB_RemoveAll( T, N )                                               \
                                                                               \
    inline int operator == ( const CATHashTab##N& iH ) const                   \
                { return _HT == iH._HT; }                                      \
    inline int operator != ( const CATHashTab##N& iH ) const                   \
                { return _HT != iH._HT; }                                      \
                                                                               \
    dclCATHTAB_ApplyMemberFunct( T, N )                                        \
    dclCATHTAB_ApplyMemberFunctConst( T, N )                                   \
    dclCATHTAB_ApplyGlobalFunct( T, N )                                        \
    dclCATHTAB_ApplyDelete( T, N )                                             \
                                                                               \
  private :                                                                    \
    CATHashTable _HT;                                                          \
};

#endif

#if defined(CATHTAB_ApplyMemberFunct) || defined(CATHTAB_ApplyMemberFunctConst) || defined(CATHTAB_ApplyGlobalFunct)
#ifndef CATHTAB_LocatePosition
/**
 * @nodoc
 */
#define CATHTAB_LocatePosition
#endif	
#ifndef CATHTAB_NextPosition
/**
 * @nodoc
 */
#define CATHTAB_NextPosition
#endif	
#endif	
	
#ifdef  CATHTAB_ApplyDelete
#ifndef CATHTAB_NextPosition
/**
 * @nodoc
 */
#define CATHTAB_NextPosition
#endif	
#ifndef CATHTAB_RemoveAll
/**
 * @nodoc
 */
#define CATHTAB_RemoveAll
#endif
#endif

#undef  dclCATHTAB_Dtor
#undef  dclCATHTAB_Move
#undef  dclCATHTAB_Dimension
#undef  dclCATHTAB_Collisions
#undef  dclCATHTAB_Unused
#undef  dclCATHTAB_KeyLocate
#undef  dclCATHTAB_KeyLocatePosition
#undef  dclCATHTAB_Locate
#undef  dclCATHTAB_LocatePosition
#undef  dclCATHTAB_Retrieve
#undef  dclCATHTAB_NextPosition
#undef  dclCATHTAB_Next
#undef  dclCATHTAB_Remove
#undef  dclCATHTAB_RemovePosition
#undef  dclCATHTAB_RemoveAll
#undef  dclCATHTAB_ApplyMemberFunct
#undef  dclCATHTAB_ApplyMemberFunctConst
#undef  dclCATHTAB_ApplyGlobalFunct
#undef  dclCATHTAB_ApplyDelete
#undef  dclCATHTAB_Rebuild

#ifdef __hpux
/**
 * @nodoc
 */
#define dclCATHTAB_Dtor( T, N )                               \
virtual ~CATHashTab##N ();
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Dtor( T, N )                               \
virtual ~CATHashTab##N () { }
#endif

#if defined(DS_CXX11_SUPPORT_MOVE_SEMANTIC)
/** @nodoc */
#define dclCATHTAB_Move( T, N )                               \
CATHashTab##N ( CATHashTab##N && ) = default;                 \
CATHashTab##N & operator= ( CATHashTab##N && ) = default;
#else
/** @nodoc */
#define dclCATHTAB_Move( T, N ) 
#endif

#ifndef CATHTAB_Dimension
/**
 * @nodoc
 */
#define dclCATHTAB_Dimension( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Dimension( T, N )                           \
int Dimension () const                                         \
{ return _HT.Dimension(); }
#endif


#ifndef CATHTAB_Rebuild
/**
 * @nodoc
 */
#define dclCATHTAB_Rebuild( T, N ) 
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Rebuild( T, N )                                     \
void Rebuild (int iNewDimension) { _HT.Rebuild(iNewDimension); }
#endif


#ifndef CATHTAB_Collisions
/**
 * @nodoc
 */
#define dclCATHTAB_Collisions( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Collisions( T, N )                          \
int Collisions () const { return _HT.Collisions(); }
#endif

#ifndef CATHTAB_Unused
/**
 * @nodoc
 */
#define dclCATHTAB_Unused( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Unused( T, N )                              \
int Unused () const { return _HT.Unused(); }
#endif

#ifndef CATHTAB_KeyLocate
/**
 * @nodoc
 */
#define dclCATHTAB_KeyLocate( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_KeyLocate( T, N )                                           \
T* KeyLocate ( int iKey ) const { return (T*) _HT.KeyLocate ( iKey ); }
#endif

#ifndef CATHTAB_KeyLocatePosition
/**
 * @nodoc
 */
#define dclCATHTAB_KeyLocatePosition( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_KeyLocatePosition( T, N )                                   \
T* KeyLocatePosition ( int iKey, int& oBucket, int& oPosition ) const          \
{ return (T*) _HT.KeyLocatePosition ( iKey, oBucket, oPosition ); }
#endif

#ifndef CATHTAB_Locate
/**
 * @nodoc
 */
#define dclCATHTAB_Locate( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Locate( T, N )                                              \
T* Locate ( T* iLocate ) const { return (T*) _HT.Locate ( (void*)iLocate ); }

#endif

#ifndef CATHTAB_LocatePosition
/**
 * @nodoc
 */
#define dclCATHTAB_LocatePosition( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_LocatePosition( T, N )                                      \
T* LocatePosition ( T* iLocate, int& oBucket, int& oPosition ) const           \
{ return (T*) _HT.LocatePosition ( (void*)iLocate, oBucket, oPosition ); }
#endif

#ifndef CATHTAB_Retrieve
/**
 * @nodoc
 */
#define dclCATHTAB_Retrieve( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Retrieve( T, N )                                            \
T* Retrieve ( int iBucket, int iPosition ) const                               \
{ return (T*) _HT.Retrieve ( iBucket, iPosition ); }
#endif

#ifndef CATHTAB_NextPosition
/**
 * @nodoc
 */
#define dclCATHTAB_NextPosition( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_NextPosition( T, N )                                        \
T* Next ( int& ioBucket, int& ioPosition ) const                               \
{ return (T*) _HT.Next( ioBucket, ioPosition ); }
#endif

#ifndef CATHTAB_Next
/**
 * @nodoc
 */
#define dclCATHTAB_Next( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Next( T, N )                                                \
T* Next ( T* iFrom ) const { return (T*) _HT.Next( (void*)iFrom ); }
#endif

#ifndef CATHTAB_Remove
/**
 * @nodoc
 */
#define dclCATHTAB_Remove( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_Remove( T, N )                                              \
T* Remove ( T* iRemove ) { return (T*) _HT.Remove( (void*)iRemove ); }
#endif

#ifndef CATHTAB_RemovePosition
/**
 * @nodoc
 */
#define dclCATHTAB_RemovePosition( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_RemovePosition( T, N )                                      \
T* RemovePosition ( int iBucket, int iPosition )                               \
{ return (T*) _HT.RemovePosition ( iBucket, iPosition ); }
#endif

#ifndef CATHTAB_RemoveAll
/**
 * @nodoc
 */
#define dclCATHTAB_RemoveAll( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_RemoveAll( T, N )                                           \
void RemoveAll () { _HT.RemoveAll(); }
#endif

#ifndef CATHTAB_ApplyMemberFunct
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyMemberFunct( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyMemberFunct( T, N )                                    \
typedef int  (T::*PtrMbrFunct) () ;                                            \
int ApplyMemberFunct ( PtrMbrFunct iPF ,                                       \
                       T*   iFrom  = 0 ,                                       \
                       T**  iPLast = 0 ,                                       \
                       int* iPRC   = 0 ) const ;
#endif

#ifndef CATHTAB_ApplyMemberFunctConst
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyMemberFunctConst( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyMemberFunctConst( T, N )                               \
typedef int (T::*PtrMbrFunctConst) () const ;                                  \
int ApplyMemberFunctConst ( PtrMbrFunctConst iPF,                              \
                            T*   iFrom  = 0     ,                              \
                            T**  iPLast = 0     ,                              \
                            int* iPRC   = 0     ) const ;
#endif

#ifndef CATHTAB_ApplyGlobalFunct
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyGlobalFunct( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyGlobalFunct( T, N )                                    \
typedef int  (*PtrGlbFunct) (T*) ;                                             \
int ApplyGlobalFunct ( PtrGlbFunct iPF ,                                       \
                       T*   iFrom  = 0 ,                                       \
                       T**  iPLast = 0 ,                                       \
                       int* iPRC   = 0 ) const                                 \
{ return _HT.ApplyGlobalFunct ( (CATHashTable::PtrGlbFunct)iPF,                \
                                (void*)iFrom, (void**)iPLast, iPRC ); }
#endif

#ifndef CATHTAB_ApplyDelete
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyDelete( T, N )
#else
/**
 * @nodoc
 */
#define dclCATHTAB_ApplyDelete( T, N )                                         \
void ApplyDelete () ;
#endif

#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CATHashTable.h"
