//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef omn_FixedString_h
#define omn_FixedString_h

// The omn_FixedString struct is meant to hold a constant C-string at a
// fixed address. All instances of this struct should be defined as:
//
// static const omn_FixedString name = { "string" };
//
// Since there is no way to enforce this rule within the C++ language,
// it is enforced by our checkFileInc script.
//
// A omn_FixedString can be used anywhere a const char* is required,
// including type-unsafe interfaces such as (...) and void*. However,
// beware that this is subject to the C++ rule of only one implicit,
// user-defined conversion. So, omn_FixedString cannot be passed to
// an interface which requires a type that converted from a const char*,
// unless an explicit cast is invoked.
//
// There is an optimized conversion constructor from omn_FixedString to
// cow_String, which avoids copying the C-string. As a result, it is
// very cheap to pass a omn_FixedString to an interface requiring a
// cow_String. Beware that this may lead to an ambiguity for interfaces
// that take either const char* or cow_String, which would trigger a
// compilation error. The interface should be restructured to take only 
// cow_String (or const cow_String&).

struct omn_FixedString {
    const char* str;
    operator const char*() const { return str; }
};

#endif // omn_FixedString_h
