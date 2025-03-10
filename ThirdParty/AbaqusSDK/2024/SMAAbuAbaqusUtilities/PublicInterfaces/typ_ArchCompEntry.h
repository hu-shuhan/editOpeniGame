//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef typ_ArchCompEntry_h
#define typ_ArchCompEntry_h

/* -*- mode: c++ -*- */
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

#ifdef InterfacePragma
#pragma interface
#endif

//
// Forward Declarations
//

class ipc_buf_iFilter;
class ipc_buf_oFilter;


// Begin local includes
#include <mem_AllocationOperators.h>

#include <omi_types.h>
#include <typ_typeTag.h>
#include <typ_Entry.h>
class aio_ostream;

class typ_ArchCompEntry  : public mem_AllocationOperators
{
    friend class typ_ArchCompTable;
    const typ_Entry* entries;
    uint           size;

public:
    typ_ArchCompEntry();
    typ_ArchCompEntry(const typ_Entry* en, uint sz);
    typ_ArchCompEntry(const typ_ArchCompEntry& ace);

    typ_ArchCompEntry&
    operator=(const typ_ArchCompEntry&);
};

aio_ostream& operator<<(aio_ostream&, const typ_ArchCompEntry&);


#endif  // #ifdef typ_ArchCompEntry_h



