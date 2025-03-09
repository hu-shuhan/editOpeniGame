//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
//
// File Name: nex_dynamicCast.h
// 
// Author: Uttam Narsu
// 
// Creation date: Tue Nov  5 1996
//
// Purpose: 
//
// This file contains the definition of macros to support ISO C++
// style dynamic_cast operator. Damn. Wish I had been able to keep
// this in typ. I had a ton of bad jokes about typ_cast.
//

#ifndef nex_dynamicCast_h
#define nex_dynamicCast_h

//
// Includes
//

// Begin local includes
#include <omi_types.h>
#include <typ_typeTag.h>

// in the .C file. Returns true if cast is allowable,
//                         false otherwise
extern bool nex_canDynamicCastPtr(typ_typeTag, typ_typeTag);


// in the .C file. Returns true if cast is allowable,
//                         throws nex_BadCastException otherwise
extern bool nex_canDynamicCastRef(typ_typeTag, typ_typeTag,
				  const char [], int);

//
// Macros to handle dynamic_cast. We don't quite use the bizarre ISO C++
// mutant template syntax, because these are implemented as macros, since
// for Phase0 objects, there is no convenient base type.
//
// dynamic_cast is hard to support because we have a few cases: 1. pointers
// 2. references 3. Phase0 (no base class) objects, 4. Phase1 objects. So
// we punt on the last two, since we have an existing style for Phase1
// objects (the equally byzantine mem_DownCast style).
//
// WARNING1 !!!: Do not use dynamic_castPtr or dynamic_castRef with Phase1
//             objects (you will get a comple error).
//
// WARNING2 !!!: dynamic_cast only works with classes which define the
//               virtual method DynTypeId() and the static method TypeId()!
//
// The pointer semantics are that if it is possible to do the cast
// (checking is done via typ_typeTags), then it is done. If the two
// pointers are not compatible, then 0 is returned.
//
// When called 'type' should be a class name, and 'expr' a pointer:
//     Foo *fp = dynamic_castPtr(Foo, f);

#define dynamic_castPtr(type,expr) \
    ((expr) ? (nex_canDynamicCastPtr(type::TypeId(),expr->DynTypeId()) ? \
	       (type*)(expr) : 0) : (type*) 0)


// The reference semantics are that if it is possible to do the cast
// (checking is done via typ_typeTags), then it is done. If the two
// references are not compatible, then the nex_BadCastException is thrown.
//
// When called 'type' should be a class name, and 'expr' an object or reference:
//     Foo &fp = dynamic_castRef(Foo, f);

#define dynamic_castRef(type,expr) \
    (nex_canDynamicCastRef(type::TypeId(), expr.DynTypeId(), \
                         __FILE__, __LINE__), \
	      (type&)(expr))


#endif  // #ifdef nex_dynamicCast_h


