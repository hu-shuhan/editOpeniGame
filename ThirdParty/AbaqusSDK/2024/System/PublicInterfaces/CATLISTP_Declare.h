
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#ifndef CATLISTP_DECLARE
/** @nodoc 
 * Macros to declare classes which implement lists of pointers 
 * to a user-defined type X.
 * <br><b>Role:</b> The functionalities supported by the list class
 * can be turned on and off depending on your needs, using #define
 * symbols (if the proper #defined symbol is present, the
 * functionality is added to the class, otherwise it is omitted).
 * There is a trade-off between code-size and functionalities:
 * the more functionalities, the larger the code size.
 * The list of available functionalities is defined in 
 * <tt>CATLISTP_AllFunct.h</tt>
 */
#define CATLISTP_DECLARE( T )  CATLISTP_DECLARE_TN( T, T )
#endif

#ifndef CATCOLLEC_ExportedBy
/** @nodoc 
 * Macro for Windows NT to trigger the export of the list class
 * defined in a DLL.
 */
#define CATCOLLEC_ExportedBy
#endif

#ifndef CATLISTP_DECLARE_TN
/** @nodoc 
 * Macro to declare a list class with a user-defined name.
 */
#define CATLISTP_DECLARE_TN( T, N )                                        \
class CATCOLLEC_ExportedBy CATListPtr##N : private CATListPV               \
{                                                                          \
  public :                                                                 \
    CATListPtr##N ( int iInitAlloc = 0 );                                  \
    CATListPtr##N ( const CATListPtr##N& iCopy );                          \
    dclCATLISTP_CtorFromArrayPtrs( T, N )                                  \
    virtual ~CATListPtr##N ();                                             \
                                                                           \
    CATListPtr##N& operator = ( const CATListPtr##N& iCopy );              \
                                                                           \
    dclCATLISTP_Append( T, N )						   \
    dclCATLISTP_AppendList( T, N )					   \
                                                                           \
    dclCATLISTP_InsertAt( T, N )                                           \
                                                                           \
    inline int Size () const { return CATListPV::Size(); }                 \
    dclCATLISTP_ReSize( T, N )                                             \
                                                                           \
    inline T*& operator[] (int iPos) { return ((T*&)(CATListPV::operator[](iPos))); }      \
    inline T*  operator[] (int iPos) const { return ((T*)(CATListPV::operator[](iPos))); } \
                                                                           \
    dclCATLISTP_Locate( T, N )                                             \
                                                                           \
    dclCATLISTP_RemoveValue( T, N )                                        \
    dclCATLISTP_RemoveList( T, N )                                         \
    dclCATLISTP_RemovePosition( T, N )                                     \
    dclCATLISTP_RemoveAll( T, N )                                          \
    dclCATLISTP_RemoveNulls( T, N )                                        \
    dclCATLISTP_RemoveDuplicates( T, N )                                   \
                                                                           \
    inline int operator == ( const CATListPtr##N& iL ) const               \
                    { return CATListPV::operator == (iL); }                      \
    inline int operator != ( const CATListPtr##N& iL ) const               \
                    { return CATListPV::operator != (iL); }                      \
                                                                           \
    dclCATLISTP_Compare( T, N )                                            \
                                                                           \
    dclCATLISTP_Swap( T, N )                                               \
    dclCATLISTP_QuickSort( T, N )                                          \
                                                                           \
    dclCATLISTP_FillArrayPtrs( T, N )                                      \
    dclCATLISTP_NbOccur( T, N )                                            \
    dclCATLISTP_Intersection( T, N )                                       \
                                                                           \
};

#endif                // CATLISTP_DECLARE_TN


/** @nodoc 
 * Macro to iterate over all the elements from a pointer list.
 */
#define CATLISTP_FORWARD_LOOP( LP, ITR )                            \
{                                                                   \
  int const CATLISTP_LoopLimit##ITR = (LP).Size() + 1 ;             \
  int ITR ;                                                         \
  for ( ITR = 1 ; ITR < CATLISTP_LoopLimit##ITR ; ITR++ )


/** @nodoc 
 * Macro to iterate over all the elements from a pointer list,
 * in reverse order.
 */
#define CATLISTP_BACKWARD_LOOP( LP, ITR )                           \
{                                                                   \
  int ITR ;                                                         \
  for ( ITR = (LP).Size(); ITR > 0; ITR-- )


/** @nodoc 
 * Macro to specify the end of an iteration loop.
 */
#define CATLISTP_END_LOOP  }

	
#ifndef CATLISTP_APPLY_DELETE
/** @nodoc 
 * Macro to apply the <tt>delete</tt> C++ operator
 * on all the elements of a pointer list.
 */
#define CATLISTP_APPLY_DELETE( LP )                                \
{                                                                  \
  CATLISTP_BACKWARD_LOOP( LP, i )                                  \
  {                                                                \
    void*& PV = (void*&) (LP)[i] ;                                 \
    delete (LP)[i] ;                                               \
    PV = (void*) 0 ;                                               \
  }                                                                \
  CATLISTP_END_LOOP                                                \
}
#endif


#undef  dclCATLISTP_CtorFromArrayPtrs
#undef  dclCATLISTP_Append
#undef  dclCATLISTP_AppendList
#undef  dclCATLISTP_InsertAt
#undef  dclCATLISTP_ReSize
#undef  dclCATLISTP_Locate
#undef  dclCATLISTP_RemoveValue
#undef  dclCATLISTP_RemoveList
#undef  dclCATLISTP_RemovePosition
#undef  dclCATLISTP_RemoveAll
#undef  dclCATLISTP_RemoveNulls
#undef  dclCATLISTP_RemoveDuplicates
#undef  dclCATLISTP_Compare
#undef  dclCATLISTP_Swap
#undef  dclCATLISTP_QuickSort
#undef  dclCATLISTP_FillArrayPtrs
#undef  dclCATLISTP_NbOccur
#undef  dclCATLISTP_Intersection


#ifndef CATLISTP_CtorFromArrayPtrs
/** @nodoc
 * Macros to declare optional member functions
 * in the list class.
 */
#define dclCATLISTP_CtorFromArrayPtrs( T, N )
#else
/** @nodoc */
#define dclCATLISTP_CtorFromArrayPtrs( T, N )                      \
CATListPtr##N ( T** iArray, int iSize );
#endif

#ifndef CATLISTP_Append
/** @nodoc */
#define dclCATLISTP_Append( T, N )
#else
/** @nodoc */
#define dclCATLISTP_Append( T, N )                                 \
int Append ( T* iAdd ) { return CATListPV::Append ( (void*)iAdd ); }
#endif


#ifndef CATLISTP_AppendList
/** @nodoc */
#define dclCATLISTP_AppendList( T, N )
#else
/** @nodoc */
#define dclCATLISTP_AppendList( T, N )                               \
int Append ( const CATListPtr##N& iConcat ) { return CATListPV::Append( iConcat ); }
#endif


#ifndef CATLISTP_InsertAt
/** @nodoc */
#define dclCATLISTP_InsertAt( T, N )
#else
/** @nodoc */
#define dclCATLISTP_InsertAt( T, N )                                \
int InsertAt ( int iPos, T* iAdd )                                 \
{ return CATListPV::InsertAt ( iPos, (void*)iAdd ); }
#endif


#ifndef CATLISTP_ReSize
/** @nodoc */
#define dclCATLISTP_ReSize( T, N )
#else
/** @nodoc */
#define dclCATLISTP_ReSize( T, N )                                    \
void Size ( int iSize ) { CATListPV::Size( iSize, NULL ); }
#endif


#ifndef CATLISTP_Locate
/** @nodoc */
#define dclCATLISTP_Locate( T, N )
#else
/** @nodoc */
#define dclCATLISTP_Locate( T, N )                                    \
int Locate ( T* iLocate, int iFrom = 1 ) const                        \
{ return CATListPV::Locate( (void*)iLocate, iFrom ); }
#endif


#ifndef CATLISTP_RemoveValue
/** @nodoc */
#define dclCATLISTP_RemoveValue( T, N )
#else
/** @nodoc */
#define dclCATLISTP_RemoveValue( T, N )                             \
int RemoveValue ( T* iRemove )                                      \
{ return CATListPV::RemoveValue( (void*)iRemove ); }
#endif



#ifndef CATLISTP_RemoveList
/** @nodoc */
#define dclCATLISTP_RemoveList( T, N )
#else
/** @nodoc */
#define dclCATLISTP_RemoveList( T, N )                             \
int Remove ( const CATListPtr##N& iSubstract )                     \
{ return CATListPV::Remove( iSubstract ); }
#endif



#ifndef CATLISTP_RemovePosition
/** @nodoc */
#define dclCATLISTP_RemovePosition( T, N )
#else
/** @nodoc */
#define dclCATLISTP_RemovePosition( T, N )                        \
void RemovePosition ( int iPos ) { CATListPV::RemovePosition ( iPos ); } \
int RemovePosition(int iIndex, unsigned int iNbElem) { return CATListPV::RemovePosition(iIndex, iNbElem); }
#endif



#ifndef CATLISTP_RemoveAll
/** @nodoc */
#define dclCATLISTP_RemoveAll( T, N )
#else
/** @nodoc */
#define dclCATLISTP_RemoveAll( T, N )                                            \
void RemoveAll ( CATCollec::MemoryHandling iMH = CATCollec::ReleaseAllocation )  \
{ CATListPV::RemoveAll( iMH ); }
#endif



#ifndef CATLISTP_RemoveNulls
/** @nodoc */
#define dclCATLISTP_RemoveNulls( T, N )
#else
#undef  dclCATLISTP_RemoveNulls
/** @nodoc */
#define dclCATLISTP_RemoveNulls( T, N )                        \
int RemoveNulls () { return CATListPV::RemoveNulls(); }
#endif



#if !defined(CATLISTP_RemoveDuplicatesCount) && !defined(CATLISTP_RemoveDuplicates)
/** @nodoc */
#define dclCATLISTP_RemoveDuplicates( T, N )
#else
/** @nodoc */
#define dclCATLISTP_RemoveDuplicates( T, N )                \
int RemoveDuplicates ( CATListPtr##N* ioExtract = NULL ); 
#endif



#ifndef CATLISTP_Compare
/** @nodoc */
#define dclCATLISTP_Compare( T, N )
#else
/** @nodoc */
#define dclCATLISTP_Compare( T, N )                           \
static int Compare ( const CATListPtr##N& iL1,                \
                     const CATListPtr##N& iL2,                \
                     int (*iPFCompare) ( T*, T* ) );
#endif



#ifndef CATLISTP_Swap
/** @nodoc */
#define dclCATLISTP_Swap( T, N )
#else
#undef  dclCATLISTP_Swap
/** @nodoc */
#define dclCATLISTP_Swap( T, N )                             \
void Swap ( int iPos1, int iPos2 )                           \
{ CATListPV::Swap ( iPos1, iPos2 ); }
#endif

#ifndef CATLISTP_QuickSort
/** @nodoc */
#define dclCATLISTP_QuickSort( T, N )
#else
/** @nodoc */
#define dclCATLISTP_QuickSort( T, N )                          \
void QuickSort ( int (*iPFCompare) ( T*, T* ) )                \
{ CATListPV::QuickSort ( (CATCollec::PFCompare) iPFCompare ); }
#endif



#ifndef CATLISTP_FillArrayPtrs
/** @nodoc */
#define dclCATLISTP_FillArrayPtrs( T, N )
#else
/** @nodoc */
#define dclCATLISTP_FillArrayPtrs( T, N )                \
void FillArray ( T** ioArray, int iMaxSize ) const       \
{ CATListPV::FillArray( (void**)ioArray, iMaxSize ); }
#endif



#ifndef CATLISTP_NbOccur
/** @nodoc */
#define dclCATLISTP_NbOccur( T, N )
#else
/** @nodoc */
#define dclCATLISTP_NbOccur( T, N )                        \
int NbOccur ( T* iTest )                                   \
{ return CATListPV::NbOccur( (void*)iTest ); }
#endif



#ifndef CATLISTP_Intersection
/** @nodoc */
#define dclCATLISTP_Intersection( T, N )
#else
/** @nodoc */
#define dclCATLISTP_Intersection( T, N )                    \
static void Intersection ( const CATListPtr##N& iL1,        \
                           const CATListPtr##N& iL2,        \
                           CATListPtr##N& ioResult )        \
{ CATListPV::Intersection ( iL1, iL2, ioResult ); }
#endif



#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "CATListPV.h"
