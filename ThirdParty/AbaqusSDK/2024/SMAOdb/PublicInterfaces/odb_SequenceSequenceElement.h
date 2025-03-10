//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SequenceSequenceElement_h
#define odb_SequenceSequenceElement_h

// Begin local includes
#include <odb_Sequence.h>
#include <odb_SequenceElement.h>
#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_SequenceElement, odb_ListSequenceElement);
ODB_SEQUENCE_DECL(odb_SequenceElement, odb_ListSequenceElement, odb_SequenceSequenceElement);

#endif // odb_SequenceSequenceElement_h
