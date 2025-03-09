//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceSubField_h
#define odb_SequenceSubField_h

// Begin local includes
#include <odb_SubField.h>
#include <odb_Sequence.h>
#include <cow_List.h>

COW_LIST_DECL(odb_SubField, odb_ListSubField);
ODB_SEQUENCE_DECL(odb_SubField, odb_ListSubField, odb_SequenceSubField);

#endif // #ifndef 
