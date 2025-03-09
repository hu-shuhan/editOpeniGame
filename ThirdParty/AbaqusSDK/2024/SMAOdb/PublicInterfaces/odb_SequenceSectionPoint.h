//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceSectionPoint_h
#define odb_SequenceSectionPoint_h

// Begin local includes
#include <odb_SectionPoint.h>
#include <odb_Sequence.h>
#include <cow_List.h>

COW_LIST_DECL(odb_SectionPoint, odb_ListSectionPoint);
ODB_SEQUENCE_DECL(odb_SectionPoint, odb_ListSectionPoint, odb_SequenceSectionPoint)

#endif // #ifndef odb_SequenceSectionPoint_h
