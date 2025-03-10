#ifndef		CATCollec_h
#define		CATCollec_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


/**
 * @nodoc
 * Define used as type for standard collection.
 * @param N The type of the collection element.
 */
#define	CATRCOLL( N )	CATRawColl##N
/**
 * @nodoc
 * Define used as type for collection of pointers.
 * @param N The type of the collection element.
 */
#define CATLISTP( N )   CATListPtr##N   
/**
 * @nodoc
 * Define used as type for collection of objects.
 * @param N The type of the collection object.
 */
#define	CATLISTV( N )	CATListVal##N
/**
 * @nodoc
 * Define used as type for a set of objects.
 * @param N The type of the set object.
 */
#define	CATSETV( N )	CATSetVal##N
/**
 * @nodoc
 * Define used as hash table of objects.
 * @param N The type of the hash table element.
 */
#define	CATHTAB( N )	CATHashTab##N
/**
 * @nodoc
 * Define used as type for dictionary of objects.
 * @param N The type of the dictionary element.
 */
#define	CATHDICO( N )	CATHashDic##N
/**
 * @nodoc
 * Define used as type for dictionary of objects.
 * @param N The type of the dictionary element.
 */
#define	CATHDICOS( N )	CATHashDicS##N

class CATString ;
class CATUnicodeString ;

/**
 * Type for collection of int.
 * <b>Role</b>:Use this type in place of @href CATRawCollint
 */
typedef	class CATRCOLL(int)			CATListOfInt ;
/**
 * Type for collection of double.
 * <b>Role</b>:Use this type in place of @href CATRawColldouble
 */
typedef	class CATRCOLL(double)			CATListOfDouble ;
/**
 * Type for collection of float.
 * <b>Role</b>:Use this type in place of @href CATRawCollfloat
 */
typedef class CATRCOLL(float)                   CATListOfFloat ;
/**
 * Type for collection of pointer.
 * <b>Role</b>:Use this type in place of @href CATRawCollPV
 */
typedef class CATRCOLL(PV)                      CATListPV ;  
/**
 * Type for collection of CATString.
 * <b>Role</b>:Use this type in place of @href CATListValCATString
 */
typedef	class CATLISTV(CATString)		CATListOfCATString ;
/**
 * Type for collection of CATUnicodeString.
 * <b>Role</b>:Use this type in place of @href CATListValCATUnicodeString
 */
typedef	class CATLISTV(CATUnicodeString)	CATListOfCATUnicodeString ;
/**
 * Type for set of CATString.
 * <b>Role</b>:Use this type in place of @href CATSetValCATString
 */
typedef	class CATSETV(CATString)		CATSetOfCATString ;
/**
 * Type for set of CATUnicodeString.
 * <b>Role</b>:Use this type in place of @href CATSetValCATUnicodeString
 */
typedef	class CATSETV(CATUnicodeString)		CATSetOfCATUnicodeString ;



/**
 * Scope level for collection definitions.
 * <b>Role</b>: This class is can not be instanciated. It is only used for scope of definitions.
 */
class CATCollec {
	public:
		/**
		 * Prototype for the comparison function.
		 */
		typedef		int	(*PFCompare)	( const void*
							, const void* );

		/**
		 * Prototype for the hashing function.
		 */
		typedef		unsigned int	(*PFHash)	( const void* );

		/**
		 * Memory management mode.
		 * @param KeepAllocation No action is taken while removing element.
		 * @param ReleaseAllocation Memory is released while removing element.
		 */
		enum		MemoryHandling	{ KeepAllocation
						, ReleaseAllocation };

		/**
		 * Displacement direction mode.
		 * @param FromFirstToLast Displacement is from first to last.
		 * @param FromLastToFirst Displacement is form last to first.
		 */
		enum		Direction	{ FromFirstToLast
						, FromLastToFirst };


	private :
		/** @nodoc */
		CATCollec ()	{}
};

#endif		/* CATCollec_h */
