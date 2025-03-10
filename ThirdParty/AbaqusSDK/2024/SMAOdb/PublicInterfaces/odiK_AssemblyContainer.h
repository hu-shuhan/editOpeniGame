/*   -*- mode: c++ -*-   */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: odiK_AssemblyContainer.h
//
// Description: Container for all assemblies within a Model
//

#ifndef odiK_AssemblyContainer_h
#define odiK_AssemblyContainer_h

//
// SECTION: System includes
//

//
// Begin local includes
//
#include <cls_Map2Obj.h>
#include <cow_ArrayCOW.h>
#include <cow_String.h>

class odiK_Assembly;
class cow_String;
COW_ARRAYCOW2_DECL(odiK_Assembly, cow_Virtual);

CLS_MAP2OBJ_ITER_DECL(cow_String, odiK_Assembly, odiK_AssemblyContainer);

#endif /* odiK_AssemblyContainer_h */
