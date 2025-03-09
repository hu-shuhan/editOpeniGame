//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef res_FieldDescriptionShortcut_h
#define res_FieldDescriptionShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <cow_ArrayCOW.h>

// Forward declarations
class cow_String;
class res_Field;
class res_FieldDescription;
template <class Val> struct cow_Virtual;
typedef cow_Virtual<res_FieldDescription> FieldDescCopy;
template <class Value, class Copy> class cow_COW;
typedef cow_ArrayCOW<res_FieldDescription, FieldDescCopy> res_FieldDescriptionCOW;
template <class A, class B, class C> class cls_Map2Obj;
typedef cls_Map2Obj<int, res_FieldDescription, res_FieldDescriptionCOW> res_FieldDescriptionContainer;

ddr_SHORTCUT_DECL(res_FieldDescription, res_FieldDescriptionShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(res_FieldDescriptionContainer, res_FieldDescription, res_FieldDescriptionInFieldDescriptionContainerShortcut);
ddr_MEMBER_SHORTCUT_DECL(res_Field, res_FieldDescription, res_FieldDescriptionInFieldShortcut);

#endif
