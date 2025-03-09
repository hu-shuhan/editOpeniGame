//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_TypeIdentifier_h
#define odb_TypeIdentifier_h

// Begin local includes
#include <nex_Exception.h>
#include <nex_dynamicCast.h>

#define odb_dynamicCast(type,expr)\
    (nex_canDynamicCastRef(type::typeIdentifier(), expr.subTypeIdentifier(),\
                           __FILE__, __LINE__),\
               (type&)(expr))

#define odb_isA(type,expr)\
    ((type::typeIdentifier() == expr.subTypeIdentifier()) ? true : false)

#endif // odb_TypeIdentifier_h
