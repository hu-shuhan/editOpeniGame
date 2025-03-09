
// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#ifndef CATLISTP_DEFINE
/**
 * Macros to define classes which implement lists of pointers 
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
#define CATLISTP_DEFINE( T )  CATLISTP_DEFINE_TN( T, T )
#endif	
	
#ifndef CATLISTP_DEFINE_TN
/** @nodoc 
 * Macro to define a list class with a user-defined name.
 */
#define CATLISTP_DEFINE_TN( T, N )                                                  \
                                                                                    \
CATListPtr##N::CATListPtr##N ( int iInitAlloc ) : CATListPV ( iInitAlloc )              \
{}                                                                                  \
                                                                                    \
CATListPtr##N::CATListPtr##N ( const CATListPtr##N& iCopy ) : CATListPV ( iCopy ) \
{}                                                                                  \
                                                                                    \
defCATLISTP_CtorFromArrayPtrs( T, N )                                               \
                                                                                    \
CATListPtr##N::~CATListPtr##N ()                                                    \
{}                                                                                  \
                                                                                    \
CATListPtr##N& CATListPtr##N::operator= ( const CATListPtr##N& iCopy )              \
{                                                                                   \
  if ( &iCopy == this ) return *this ;                                              \
  CATListPV::operator=(iCopy);                                                             \
  return  *this ;                                                                   \
}                                                                                   \
                                                                                    \
defCATLISTP_RemoveDuplicates( T, N )                                                \
                                                                                    \
defCATLISTP_Compare( T, N )

#endif  // CATLISTP_DEFINE_TN


#undef  defCATLISTP_CtorFromArrayVals
#undef  defCATLISTP_CtorFromArrayPtrs
#undef  defCATLISTP_RemoveDuplicates
#undef  defCATLISTP_Compare


#ifndef CATLISTP_CtorFromArrayVals
/** @nodoc
 * Macros to define optional member functions
 * in the list class.
 */
#define defCATLISTP_CtorFromArrayVals( T, N )
#else
/** @nodoc */
#define defCATLISTP_CtorFromArrayVals( T, N )                                       \
CATListPtr##N::CATListPtr##N ( T* iArray, int iSize ) : CATListPV ( iSize )             \
{                                                                                   \
  for ( int i = 0; i < iSize; i++ )                                                 \
    CATListPV::Append ( (void*)&iArray[i] );                                             \
}
#endif



#ifndef CATLISTP_CtorFromArrayPtrs
/** @nodoc */
#define defCATLISTP_CtorFromArrayPtrs( T, N )
#else
/** @nodoc */
#define defCATLISTP_CtorFromArrayPtrs( T, N )                                        \
CATListPtr##N::CATListPtr##N ( T** iArray, int iSize )                               \
                               : CATListPV ( (void**)iArray, iSize )                     \
{}
#endif




#if !defined(CATLISTP_RemoveDuplicatesCount) && !defined(CATLISTP_RemoveDuplicates)
/** @nodoc */
#define defCATLISTP_RemoveDuplicates( T, N )
#else
/** @nodoc */
#define defCATLISTP_RemoveDuplicates( T, N )                                         \
int CATListPtr##N::RemoveDuplicates ( CATListPtr##N* ioExtract )                     \
{                                                                                    \
  if ( ioExtract ) return CATListPV::RemoveDuplicates( ioExtract );               \
  return CATListPV::RemoveDuplicates();                                                   \
}
#endif




#ifndef CATLISTP_Compare
/** @nodoc */
#define defCATLISTP_Compare( T, N )
#else
/** @nodoc */
#define defCATLISTP_Compare( T, N )                                                    \
int CATListPtr##N::Compare ( const CATListPtr##N& iL1,                                 \
                             const CATListPtr##N& iL2,                                 \
                             int (*iPFCompare) ( T*, T* ) )                            \
{                                                                                      \
  return CATListPV::Compare( iL1, iL2, (CATCollec::PFCompare)iPFCompare ); \
}
#endif

