
// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//
//      Macros :        1- CATLISTHand_DECLARE( T )
//      ======
//                      2- CATLISTHand_DECLARE_TS( T, S )
//
//                      3- CATLISTHand_DECLARE_TN( T, N )
//
//                      4- CATLISTHand_DECLARE_TNS( T, N, S )
//
//                      where T stands for a Type name
//                            N stands for a Name appended to "CATListHand"
//                      (in the first two cases, T and N are the same)
//                            S stands for a Static buffer length
//                      (in cases 1 and 3, S is set to 1)
//
//
//      Purpose :       Generate body of a non-template class CATListHand"N"
//      =======         (list of handlers of type "T")
//
//
//      Usage :         // 1- To generate the body of a class CATListHandMyClass,
//      =====           //    create a file CATListHandMyClass.h containing those lines :
//                              #ifndef CATListHandMyClass_h
//                              #define CATListHandMyClass_h
//                              //
//                              // declare base class :
//                              //
//                              class CATMyClass;
//                              //
//                              // clean previous requests for extra functionalities :
//                              //
//                              #include  <CATLISTHand_Clean.h>
//                              //
//                              // define symbols for extra functionalities :
//                              //
//                              #define CATLISTHand_Append
//                              #define CATLISTHand_QuickSort
//                              #define CATLISTHand_...
//                              //
//                              // define macros CATLISTHand_DECLARE :
//                              //
//                              #include  <CATLISTHand_Declare.h>
//                              //
//                              // DECLARE functions for class CATListHandCATMyClass :
//                              // (generate body of class)
//                              //
//                              CATLISTHand_DECLARE( CATMyClass )
//                              //
//                              // DECLARE functions for class CATListHandConstMyClass :
//                              // (generate body of class)
//                              //
//                              CATLISTHand_DECLARE_TNS( const CATMyClass, ConstMyClass, 10 )
//                              //
//                              #endif  // CATListHandCATMyClass_h
//
//                      // 2- Consult documentation "COLLECTIONS" 
//
//
//      Note :          The static buffer prevents from dynamic allocations.
//      ====            A big value will increase the code size,
//                      A low value will increase the number of allocations:
//                      So this value must be tuned to fit the application.
//
//
//      Authors :       Lionel Buffier
//      =======
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//      1- To produce declaration of functions for class CATListHand...
//         ============================================================
//
////////////////////////////////////////////////////////////////////////////////

// Macro CATCOLLEC_ExportedBy is used for WindowsNT import/export DLL specifications
// 1- it should be defined to ExportedByXX0MODUL
// 2- but if not defined, it should not be kept as indesirable text :
#ifndef CATCOLLEC_ExportedBy
/** @nodoc */
#define CATCOLLEC_ExportedBy
#endif

#ifndef CATLISTHand_DECLARE
/** @nodoc */
#define CATLISTHand_DECLARE(T) CATLISTHand_DECLARE_TNS(T, T, 1)
#endif

#ifndef CATLISTHand_DECLARE_TS
/** @nodoc */
#define CATLISTHand_DECLARE_TS(T, S) CATLISTHand_DECLARE_TNS(T, T, S)
#endif

#ifndef CATLISTHand_DECLARE_TN
/** @nodoc */
#define CATLISTHand_DECLARE_TN(T, N) CATLISTHand_DECLARE_TNS(T, N, 1)
#endif

#ifndef CATLISTHand_DECLARE_TNS
/** @nodoc */
#define CATLISTHand_DECLARE_TNS(T, N, S)                                    \
class T;                                                                    \
                                                                            \
class CATCOLLEC_ExportedBy CATListVal##N : public CATCollecRoot             \
{                         /************/                                    \
	public :                                                                  \
									                                                          \
		/*--------------------------*/                                          \
		/* constructor              */                                          \
		/*--------------------------*/                                          \
		CATListVal##N ();                                                       \
		CATListVal##N (int iInitAlloc);                                         \
		CATListVal##N (int iInitAlloc, int iExtensAlloc);                       \
		CATListVal##N (const CATListVal##N& iCopy);                             \
                                                                            \
		/*--------------------------*/                                          \
		/* destructor               */                                          \
		/*--------------------------*/                                          \
		virtual ~CATListVal##N ();                                              \
								                                                            \
		/*--------------------------*/                                          \
		/* copy operator            */                                          \
		/*--------------------------*/                                          \
		CATListVal##N& operator= (const CATListVal##N& iCopy);                  \
					                                                                  \
		/*--------------------------*/                                          \
		/* size, max                */                                          \
		/*--------------------------*/                                          \
		int Size() const ;                                                      \
		void Max(int iSize);                                                    \
			                                                                      \
		/*--------------------------*/                                          \
		/* operateur []             */                                          \
		/*--------------------------*/                                          \
		dclCATLISTHand_IndexOP(T, N, S)                                         \
		dclCATLISTHand_IndexOPconst(T, N, S)                                    \
                                                                            \
		/*---------------------------------------------------------*/           \
		/* optional functions (append, Insert, Locate, remove)     */           \
		/*---------------------------------------------------------*/           \
		dclCATLISTHand_Append(T, N, S)                                          \
		dclCATLISTHand_AppendList(T, N, S)                                      \
		dclCATLISTHand_InsertAt(T, N, S)                                        \
		dclCATLISTHand_InsertAfter(T, N, S)                                     \
		dclCATLISTHand_ReSize(T, N, S)                                          \
		dclCATLISTHand_Locate(T, N, S)                                          \
		dclCATLISTHand_RemoveValue(T, N, S)                                     \
		dclCATLISTHand_RemoveList(T, N, S)                                      \
		dclCATLISTHand_RemovePosition(T, N, S)                                  \
		dclCATLISTHand_RemoveAll(T, N, S)                                       \
		dclCATLISTHand_RemoveNulls(T, N, S)                                     \
		dclCATLISTHand_RemoveDuplicatesCount(T, N, S)                           \
		dclCATLISTHand_RemoveDuplicatesExtract(T, N, S)                         \
                                                                            \
		/*---------------------------------------*/                             \
		/* optional functions (operateur eq, ne) */                             \
		/*---------------------------------------*/                             \
		dclCATLISTHand_eqOP(T, N, S)                                            \
		dclCATLISTHand_neOP(T, N, S)                                            \
                                                                            \
		/*-----------------------------------------------------*/               \
		/* optional functions (compare, swap, quicksort,...)   */               \
		/*-----------------------------------------------------*/               \
		/*dclCATLISTHand_Compare( T, N, S ) */                                  \
		dclCATLISTHand_Swap(T, N, S)                                            \
		dclCATLISTHand_Replace(T, N, S)                                         \
		dclCATLISTHand_QuickSort(T, N, S)                                       \
		dclCATLISTHand_NbOccur(T, N, S)                                         \
		dclCATLISTHand_Intersection(T, N, S)                                    \
		dclCATLISTHand_Storage(T, N, S)                                         \
                                                                            \
                                                                            \
	protected :                                                               \
							                                                              \
		inline void GetStorage(int iNbElem, T*& oBlock)                         \
			{ oBlock = (T*)malloc (sizeof(T)*iNbElem); }                          \
		inline void FreeStorage(T*& ioBlock)                                    \
			{ if ( ioBlock ) { free (ioBlock) ; ioBlock = NULL ;} }               \
		void GetImplementationList(int iNbElem, IUnknown**& oBlock) const;      \
			                                                                      \
		/*------------------------*/                                            \
		/* data menbers           */                                            \
		/*------------------------*/                                            \
		static const int _SBSize ;                                              \
		int _Size ;                                                             \
		int _MaxSize ;                                                          \
		T _SBlock [S] ;                                                         \
		T* _Block ;                                                             \
};  

#endif // CATLISTHand_DECLARE_TNS


//---------------------------------------------------------------------------
// [] operator
//-------------------------
#undef dclCATLISTHand_IndexOP
/** @nodoc */
#define dclCATLISTHand_IndexOP(T, N, S)                                         \
		T& operator[] (int iPos);               

#undef dclCATLISTHand_IndexOPconst
/** @nodoc */
#define dclCATLISTHand_IndexOPconst(T, N, S)                                    \
		const T& operator[] (int iPos) const;             
                
//---------------------------------------------------------------------------
// Append / AppendList
//-------------------------
#ifndef CATLISTHand_Append

#undef dclCATLISTHand_Append
/** @nodoc */
#define dclCATLISTHand_Append(T, N, S)

#else

#undef dclCATLISTHand_Append
/** @nodoc */
#define dclCATLISTHand_Append(T, N, S)                            \
		void Append (const T& iAdd);

#endif

#ifndef CATLISTHand_AppendList

#undef dclCATLISTHand_AppendList
/** @nodoc */
#define dclCATLISTHand_AppendList(T, N, S)

#else

#undef dclCATLISTHand_AppendList
/** @nodoc */
#define dclCATLISTHand_AppendList(T, N, S)                        \
		void Append(const CATListVal##N& iConcat);

#endif


//---------------------------------------------------------------------------
// Insert At, After
//--------------------------------
#ifndef CATLISTHand_InsertAt

#undef dclCATLISTHand_InsertAt
/** @nodoc */
#define dclCATLISTHand_InsertAt(T, N, S)

#else

#undef dclCATLISTHand_InsertAt
/** @nodoc */
#define dclCATLISTHand_InsertAt(T, N, S)                          \
		void InsertAt ( int iPos, const T& iAdd );

#endif

#ifndef CATLISTHand_InsertAfter

#undef dclCATLISTHand_InsertAfter
/** @nodoc */
#define dclCATLISTHand_InsertAfter(T, N, S)

#else

#undef dclCATLISTHand_InsertAfter
/** @nodoc */
#define dclCATLISTHand_InsertAfter(T, N, S)                          \
		void InsertAfter(int iPos, const T& iAdd);

#endif


//----------------------------------------------------------------------------
// Resize (optional)
//-------------------------------------------
#ifndef CATLISTHand_ReSize

#undef dclCATLISTHand_ReSize
/** @nodoc */
#define dclCATLISTHand_ReSize(T, N, S)

#else

#undef dclCATLISTHand_ReSize
/** @nodoc */
#define dclCATLISTHand_ReSize(T, N, S)                \
                void Size(int iSize, const T*	iFiller = 0); 

#endif


//------------------------------------------------------------------------------
// Locate

//----------------------------------------------
#ifndef CATLISTHand_Locate

#undef dclCATLISTHand_Locate
/** @nodoc */
#define dclCATLISTHand_Locate(T, N, S)

#else

#undef dclCATLISTHand_Locate
/** @nodoc */
#define dclCATLISTHand_Locate(T, N, S)                                \
		int Locate (const T& iLocate,                                          \
			     int iFrom = 1) const;

#endif


//------------------------------------------------------------------------------
// Remove, 
//----------------------------------------------
#ifndef CATLISTHand_RemoveValue

#undef dclCATLISTHand_RemoveValue
/** @nodoc */
#define dclCATLISTHand_RemoveValue(T, N, S)

#else

#undef dclCATLISTHand_RemoveValue
/** @nodoc */
#define dclCATLISTHand_RemoveValue(T, N, S)                           \
		int RemoveValue(const T& iRemove);

#endif


#ifndef CATLISTHand_RemoveList

#undef dclCATLISTHand_RemoveList
/** @nodoc */
#define dclCATLISTHand_RemoveList(T, N, S)

#else

#undef dclCATLISTHand_RemoveList
/** @nodoc */
#define dclCATLISTHand_RemoveList(T, N, S)                            \
		int Remove(const CATListVal##N& iSubstract);

#endif


#ifndef CATLISTHand_RemovePosition

#undef dclCATLISTHand_RemovePosition
/** @nodoc */
#define dclCATLISTHand_RemovePosition(T, N, S)

#else

#undef dclCATLISTHand_RemovePosition
/** @nodoc */
#define dclCATLISTHand_RemovePosition(T, N, S)                        \
		void RemovePosition(int iPos);

#endif


#ifndef CATLISTHand_RemoveAll

#undef dclCATLISTHand_RemoveAll
/** @nodoc */
#define dclCATLISTHand_RemoveAll(T, N, S)

#else

#undef dclCATLISTHand_RemoveAll
/** @nodoc */
#define dclCATLISTHand_RemoveAll(T, N, S)                             \
		void RemoveAll(CATCollec::MemoryHandling iMH = CATCollec::ReleaseAllocation);

#endif


#ifndef CATLISTHand_RemoveNulls

#undef dclCATLISTHand_RemoveNulls
/** @nodoc */
#define dclCATLISTHand_RemoveNulls(T, N, S)

#else

#undef dclCATLISTHand_RemoveNulls
/** @nodoc */
#define dclCATLISTHand_RemoveNulls(T, N, S)                           \
		int RemoveNulls();

#endif


#ifndef CATLISTHand_RemoveDuplicatesCount

#undef dclCATLISTHand_RemoveDuplicatesCount
/** @nodoc */
#define dclCATLISTHand_RemoveDuplicatesCount(T, N, S)

#else

#undef dclCATLISTHand_RemoveDuplicatesCount

/** @nodoc */
#define dclCATLISTHand_RemoveDuplicatesCount(T, N, S)                     \
		int RemoveDuplicates();

#endif


#ifndef CATLISTHand_RemoveDuplicatesExtract

#undef dclCATLISTHand_RemoveDuplicatesExtract
/** @nodoc */
#define dclCATLISTHand_RemoveDuplicatesExtract(T, N, S)

#else

#undef dclCATLISTHand_RemoveDuplicatesExtract
/** @nodoc */
#define dclCATLISTHand_RemoveDuplicatesExtract(T, N, S)                     \
		void RemoveDuplicates(CATListVal##N&	ioExtract);

#endif


//------------------------------------------------------------------------------
// operateur equal, non-equal
//---------------------------------------------
#ifndef CATLISTHand_eqOP

#undef dclCATLISTHand_eqOP
/** @nodoc */
#define dclCATLISTHand_eqOP(T, N, S)      

#else

#undef dclCATLISTHand_eqOP
/** @nodoc */
#define dclCATLISTHand_eqOP(T, N, S)                                  \
                int operator == (const CATListVal##N& iL) const;

#endif


#ifndef CATLISTHand_neOP

#undef dclCATLISTHand_neOP
/** @nodoc */
#define dclCATLISTHand_neOP(T, N, S)      

#else

#undef dclCATLISTHand_neOP
/** @nodoc */
#define dclCATLISTHand_neOP(T, N, S)                                  \
		int operator != (const CATListVal##N& iL) const;

#endif


//------------------------------------------------------------------------------
// swap, replace, quicksort
//----------------------------------------------
#ifndef CATLISTHand_Swap

#undef dclCATLISTHand_Swap
/** @nodoc */
#define dclCATLISTHand_Swap(T, N, S)

#else

#undef dclCATLISTHand_Swap
/** @nodoc */
#define dclCATLISTHand_Swap(T, N, S)  void Swap(int iPos1, int iPos2);

#endif


#ifndef CATLISTHand_Replace

#undef dclCATLISTHand_Replace
/** @nodoc */
#define dclCATLISTHand_Replace(T, N, S)

#else

#undef dclCATLISTHand_Replace
/** @nodoc */
#define dclCATLISTHand_Replace(T, N, S)                              \
                void Replace(int iPos1, const T& iReplace);

#endif


#ifndef CATLISTHand_QuickSort

#undef dclCATLISTHand_QuickSort
/** @nodoc */
#define dclCATLISTHand_QuickSort(T, N, S)

#else

#undef dclCATLISTHand_QuickSort
/** @nodoc */
#define dclCATLISTHand_QuickSort(T, N, S)                              \
		void QuickSort(int (*iPFCompare)( T*, T* ));

#endif


//-------------------------------------------------------------------------------

// NbOccur
//--------------------------------------
#ifndef CATLISTHand_NbOccur

#undef dclCATLISTHand_NbOccur
/** @nodoc */
#define dclCATLISTHand_NbOccur(T, N, S)

#else

#undef dclCATLISTHand_NbOccur
/** @nodoc */
#define dclCATLISTHand_NbOccur(T, N, S)                                \
		int NbOccur(const T& iTest);

#endif


//-------------------------------------------------------------------------------
// Intersection
//--------------------------------------
#ifndef CATLISTHand_Intersection

#undef dclCATLISTHand_Intersection
/** @nodoc */
#define dclCATLISTHand_Intersection(T, N, S)

#else

#undef dclCATLISTHand_Intersection
/** @nodoc */
#define dclCATLISTHand_Intersection(T, N, S)                           \
		static void Intersection ( const CATListVal##N& iL1,                    \
					   const CATListVal##N& iL2,                                \
					   CATListVal##N& ioResult );

#endif


//--------------------------------------------------------------------------------
// Storage
//----------------------------
#ifndef CATLISTHand_Storage

#undef dclCATLISTHand_Storage
/** @nodoc */
#define dclCATLISTHand_Storage(T, N, S)

#else

#undef dclCATLISTHand_Storage
/** @nodoc */
#define dclCATLISTHand_Storage(T, N, S)                                \
		T* Storage () const { return _Block ; }

#endif


//------------------------------------------------
// includes
//--------------
#include "CATCollec.h"
#include "CATCollecRoot.h"
#include "IUnknown.h"
#include  <stdlib.h>
