//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceFieldOutput_h
#define odb_SequenceFieldOutput_h

// Begin local includes
#include <odb_FieldOutput.h>
#include <odb_Sequence.h>
#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_FieldOutput, odb_ListFieldOutput);
ODB_SEQUENCE_DECL(odb_FieldOutput, odb_ListFieldOutput, odb_SequenceFieldOutput);

#endif // #ifndef odb_SequenceFieldOutput_h
