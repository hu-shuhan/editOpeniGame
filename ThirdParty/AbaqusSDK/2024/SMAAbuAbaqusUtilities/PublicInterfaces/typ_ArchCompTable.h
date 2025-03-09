//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef typ_ArchCompTable_h
#define typ_ArchCompTable_h

/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: typ_ArchCompEntry.h
// 
// Author: Uttam Narsu
// 
// Creation date: Tue Sep 27 1994
//
// Purpose:
//
// This file contains the definition of the typ_ArchCompEntry struct.
//

//
// Forward Declarations
//

class typ_ArchCompTable;
//
// Includes
//

#include <stdlib.h>
#include <assert.h>

// Begin local includes
#include <mem_AllocationOperators.h>

#include <omi_types.h>
#include <typ_typeTag.h>
#include <typ_Entry.h>
#include <typ_ArchCompEntry.h>

class typ_ArchCompTable  : public mem_AllocationOperators
{
public:
    // Dispatch to appropriate type table
    static const char*		GetName(typ_typeTag);
    static typ_typeTag	        GetParent(typ_typeTag);

    static bool        		IsParent(typ_typeTag child, 
					 typ_typeTag suspect);

    // Table Enquiry
    static typ_TableTypes	TableType(typ_typeTag);

    // Table mutators
    static void			InsertEntry(ushort, typ_ArchCompEntry);

    static void                 initialize(int);
    static void                 finalize();
protected:
    static const typ_Entry&	GetEntry(typ_typeTag);
private:
    
    static typ_ArchCompEntry	*tbl;
    static uint		tbsize;
};


inline typ_TableTypes
typ_ArchCompTable::TableType(typ_typeTag tt)
{
    if (tt == typ_OBJ || tt == typ_ANY )
        return typ_VirtComponentTT;
    else
        return GetEntry(tt).kind;
}

#endif  // #ifdef typ_ArchCompTable_h
