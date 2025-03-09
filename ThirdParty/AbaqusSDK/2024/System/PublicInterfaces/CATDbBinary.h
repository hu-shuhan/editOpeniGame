#ifndef _CAT_DB_BINARY_H
#define _CAT_DB_BINARY_H

// COPYRIGHT DASSAULT SYSTEMES 2001 

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include <limits.h>
#include "CO0LSTPV.h"

class CATUnicodeString;
class CATString;

/**
 * @nodoc
 */
extern int ExportedByCO0LSTPV CATDbCharToHexa ( unsigned char * InCh 
					       , int inLg 
					       , unsigned char *OutCh 
					       , int OutLg );

/**
 * Class to use to manipulate Binary type of data.
 * <b>Role</b>:
 * A Binary is made of a length followed by an area containing     
 * the data:
 *
 *         LLDDDDDDDDDDDD
 *           <--- LL --->
 * 
 * As it doesn't end with '\0', it must always be used with its length.
 *
 * To ensure the compatibility with DBMSs, the length is a short.
 *
 * It is possible to set the maximal length of the Binary. If during
 * an affectation or a concatenation, one tries to exceed this length, a 
 * truncation will occur.
 * If no length is given, the Binary will not have any size limit.
 */
class ExportedByCO0LSTPV CATDbBinary  
{

public:
/**
 * Default constructor. 
 * Creation of an empty Binary.
 * @param iMaxLength : maximal length 
 */
   CATDbBinary(const short iMaxLength = 0);                     

/**
 * Copy constructor.    
 * The new Binary has the same length limit as the copied Binary
 * @param iBinaryToCopy : The binary to copy
 */
   CATDbBinary(const CATDbBinary & iBinaryToCopy);

/**
 * Creation of a Binary using length and data.
 * @param iLengthToCopy : The number of characters to take into account
 *   <b>Legal values</b>: If length is <=  0, an empty Binary will be created, 
 *                        If length is >  SHRT_MAX, a Binary of size SHRT_MAX will be created
 * @param iAreaToCopy   : The pointer to the binary value
 * If iAreaToCopy is empty (=NULL), an empty Binary will be created
 * @param iMaxLength    : maximal length 
 */
  CATDbBinary(const short &iLengthToCopy, const void *iAreaToCopy, const short iMaxLength = 0);

/**
 * Creation of a Binary using an area having a Binary structure.    
 * (ie: length + data)
 * The rules explained into the preceeding constructor apply.   
 * @param iAreaToCopy   : The pointer to the length & binary value
 * @param iMaxLength    : maximal length 
 */
  CATDbBinary(const void *iAreaToCopy, const short iMaxLength = 0);

/**
 * Destructor.
 */
	~CATDbBinary();

/**
 * Initialisation of a binary.
 * @param iLengthToCopy : The number of characters to take into account
 *   <b>Legal values</b>: If length is <=  0, an empty Binary will be created, 
 *                        If length is >  SHRT_MAX, a Binary of size SHRT_MAX will be created
 * @param iAreaToCopy   : The pointer to the binary value
 * If iAreaToCopy is empty (=NULL), an empty Binary will be created
 * @param iMaxLength    : maximal length 
 */
   int Build (const short iLengthToCopy
            , const void *iZoneToCopy
	    , const short iMaxLength);

// --------------------------------------------------------------
// Operators
//
//     In order to use these operators with a Binary like  
//   area (llddddd..), you just have to cast this area.
//   Example:       
//
//        CATDbBinary Binary1 = Binary2 + (CATDbBinary)ptr_char;
// --------------------------------------------------------------

/**
 * Assignment from a CATDbBinary instance operator.
 * @param iBinaryToAffect : the binary to assign
 */
   CATDbBinary & operator= (const CATDbBinary & iBinaryToAffect);

/**
 * Concatenation from a Binary operator.
 * @param iBinaryToConcatenate : the binary to concatenate.
 */
  CATDbBinary & operator<< (const CATDbBinary & iBinaryToConcatenate);

/**
 * Addition from a Binary operator.
 * @param iBinaryToAdd : the binary to add.
 */
  CATDbBinary operator+ (const CATDbBinary & iBinaryToAdd) const;

/**
 * Equality operator.
 * @param iBinaryToCompare : the binary to compare.
 */
  int operator== (const CATDbBinary & iBinaryToCompare) const;

/**
 * Inequality operator.
 * @param iBinaryToCompare : the binary to compare.
 */
  int operator!= (const CATDbBinary & iBinaryToCompare) const;

/**
 * Cast to get the actual length of the Binary.
 */
  operator short() const;

/**
 * Cast to a Binary like area.
 * ie : length + data
 */
  operator const char *() const;

     
/**
 * To get the contents of the Binary (Only the data).
 */
  const void *GetBinaryData() const;

/**
 * Converts the contents of the Binary to a hexadecimal character string.
 */
  CATUnicodeString GetHexaValue() const;

/**
 * Resets a binary.
 * Destroy its contents and sets its size to 0.
 */
  void Reset();

/**
 * @nodoc
 */
  void Print(int  & OutputStream) const;

/**
 * @nodoc
 */
  unsigned int ComputeHashKey();

// --------------------------------------------------------------
// dedicated methods don't work if binary length GREAT THAN 16
// --------------------------------------------------------------

/**
 * @nodoc
 */
friend int ExportedByCO0LSTPV ComputeHexa ( const CATDbBinary & inBin
					   , CATUnicodeString & ouHexa);
/**
 * @nodoc
 */
friend int ExportedByCO0LSTPV ComputeHexaDCE ( const CATDbBinary & inBin
					      , CATString & ouHexa);
/**
 * @nodoc
 */
friend int ExportedByCO0LSTPV BuildFromHexa ( const CATUnicodeString & inHexa
					     , CATDbBinary & outHexa);
/**
 * @nodoc
 */
friend int ExportedByCO0LSTPV BuildFromHexaDCE ( const CATString & inHexa
					     , CATDbBinary & outHexa);
private:

	/**
	 * @nodoc
	 */
  CATDbBinary(const CATDbBinary * iBinaryToCopy);

  short _BinaryLength;
  short _BinaryMaxLength;
  char  *_BinaryData;

};

/**
 * Type to manipulate pointers to Binary data.
 */
typedef class CATDbBinary  *CATDbBinaryH;  

/**
 * @nodoc
 */
extern int ExportedByCO0LSTPV CompareCATDbBinary (CATDbBinary *B1
						 ,CATDbBinary *B2
						 );

/**
 * @nodoc
 */
extern unsigned int ExportedByCO0LSTPV HashCATDbBinary	(CATDbBinary *B1);

#endif



