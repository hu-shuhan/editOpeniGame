//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef typ_typeTag_h
#define typ_typeTag_h

// Begin local includes
#include <omi_types.h>


// an index into the type tables is a 32-bit quantity
typedef uint typ_typeTag;

//                                                          
// Constants
//


// mask off lower 16 bits;
const uint typ_FacilityMask = 0xFFFF0000;
const uint typ_ClassMask    = 0x0000FFFF;

// enumerated type used to distinguish between a type of
// typ_NONE, typ_ANY, or a type of typ_OBJ for, respectively, a 
// primitive type and any mem_ObjectPtr or obj_ObjectPtr.
enum typ_typeKind {
    typ_none = 1,
    typ_any = 2,
    typ_obj = 3,
    typ_ado = 4
};


// This will be used primarily to indicate that no component is passed
// in a message.
const typ_typeTag typ_NONE = 0x00000000 | typ_none;


// NO_PARENT should be replaced with typ_OBJ. If a class has
// typ_ObjectPtr as its base class, then typ_OBJ should be the
// appropriate entry in the cpp_parent field in the appropriate
// type table.  

const typ_typeTag typ_ANY = 0x00000000 | typ_any;
const typ_typeTag typ_OBJ = 0x00000000 | typ_obj;
const typ_typeTag typ_ADO = 0x00000000 | typ_ado;

#endif









