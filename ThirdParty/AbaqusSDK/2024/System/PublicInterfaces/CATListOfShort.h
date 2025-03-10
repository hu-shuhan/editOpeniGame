#ifndef	CATListOfShort_h
#define	CATListOfShort_h

//  COPYRIGHT  DASSAULT  SYSTEMES  2010  

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */ 

// To undefine all possible methods of the list of values
#include "CATLISTV_Clean.h" 
#include "JS0CTYP.h"

// Define the methods supported by the list of values
#define CATLISTV_RemoveAll
#define CATLISTV_AppendList
#define CATLISTV_Locate
#define CATLISTV_InsertAt
#define CATLISTV_RemovePosition
#define CATLISTV_RemoveValue
#define CATLISTV_Append
#define CATLISTV_ArrayVals

#ifdef CATCOLLEC_ExportedBy
#undef CATCOLLEC_ExportedBy
#define CATCOLLEC_ExportedBy ExportedByJS0CTYP
#endif // CATCOLLEC_ExportedBy

// Declare the class CATListValShort : list of values of <short>
#include "CATLISTV_Declare.h"
CATLISTV_DECLARE_TN(short, Short)
typedef CATListValShort CATListOfShort;

#endif // CATListOfShort_h


