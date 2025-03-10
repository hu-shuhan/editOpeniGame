//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SequenceSequenceNode_h
#define odb_SequenceSequenceNode_h

// Begin local includes
#include <odb_Sequence.h>
#include <odb_SequenceNode.h>
#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_SequenceNode, odb_ListSequenceNode);
ODB_SEQUENCE_DECL(odb_SequenceNode, odb_ListSequenceNode, odb_SequenceSequenceNode);

#endif // odb_SequenceSequenceNode_h
