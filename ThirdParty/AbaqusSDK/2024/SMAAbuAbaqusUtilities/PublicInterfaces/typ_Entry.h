//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef typ_Entry_h
#define typ_Entry_h

/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: typ_Entry.h
// 
// Author: George Beshers
// 
// Creation date: Tue Feb 20 1996
//
// Purpose:
//
// This file contains the definition of the typ_Entry struct.
//

//
// Forward Declarations
//

// Begin local includes

#include <omi_types.h>
#include <typ_typeTag.h>

enum typ_TableTypes {
    typ_ErrorTT,
    typ_VirtComponentTT,
    typ_ComponentTT,
    typ_AdoTT,
    typ_ExceptionTT,
    typ_ArrayTT,
    typ_SimpleTT,
    typ_Phase0TT
};

struct typ_Entry {
    typ_TableTypes   kind;	   // The category of this class
    typ_typeTag      cppParent;	   // TypeConstructor; parent in C++ sense
    const char*      name;	   // Ado name
};


#endif  // #ifdef typ_Entry_h
