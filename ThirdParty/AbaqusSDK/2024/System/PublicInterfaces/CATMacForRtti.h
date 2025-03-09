#include "CATMacForRtti_required.h"
#ifndef __CATMacForRtti
#define __CATMacForRtti     42600

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2012

#include "CATMacForIUnknown.h"


/**
 * @nodoc
 * Declares a class.
 * Use this macro in the class header file in conjunction with @href CATImplementClass_Deprec 
 * in the class source (.cpp) file.
 * End it with a semicolon.
 */
#define CATDeclareClass_Deprec      CATDeclareClass


/**
 * @nodoc
 * Defines an implementation class.
 * Use this macro in the class source (.cpp) file in conjunction with @href CATDeclareClass_Deprec
 * in the class header file
 * End it with a semicolon.
 */
#define CATImplementClass_Deprec(Class,Basemeta)                                \
CATMetaClass * __stdcall Class::GetMetaObject() const                           \
{                                                                               \
   return(MetaObject());                                                        \
}                                                                               \
                                                                                \
const CLSID & __stdcall Class::ClassId()                                        \
{                                                                               \
   return(MetaObject()->GetClassId());                                          \
}                                                                               \
const char * __stdcall Class::ClassName()                                       \
{                                                                               \
   return(MetaObject()->IsA());                                                 \
}                                                                               \
const char *Class::IsA() const                                                  \
{                                                                               \
   return(MetaObject()->IsA());                                                 \
}                                                                               \
int Class::IsAKindOf(const char *ident) const                                   \
{                                                                               \
   return(MetaObject()->IsAKindOf(ident));                                      \
}                                                                               \
                                                                                \
CATBaseUnknown *Class::CreateItself()                                           \
{                                                                               \
   return(nullptr);                                                             \
}                                                                               \
                                                                                \
CATMacMetaObjectMethodProlog(Class)                                             \
                                                                                \
  /* Check that "CNext inheritance" is related to "C++ inheritance" for proper RTTI support */ \
  static_assert((dsy::internal::is_accessible_strict_base_of<Basemeta, Class>() || std::is_same<Basemeta, CATNull>::value), \
        "[CATImplementClass_Deprec] 2nd argument \"" #Basemeta "\" is not a valid \"BaseMeta\" because it is not a parent C++ class of \"" #Class "\" or \"CATNull\"");                                        \
  meta_object = dsy::internal::fct_RetrieveMetaObjectDeprec(#Class,Implementation,\
            Basemeta::MetaObject(),"CATNull",CATLicenseOptionId,sizeof(Class)); \
                                                                                \
CATMacMetaObjectMethodEpilog(Class)                                             \
                                                                                \
static_assert(true, ""/*CATImplementClass_Deprec requires an ending semicolon*/)


namespace dsy
{
    namespace internal  // Internal use only
    {
        /** @nodoc Function that creates a meta-object */
        ExportedByJS0CORBA CATMetaClass* fct_RetrieveMetaObjectDeprec(char const*, TypeOfClass, 
            CATMetaClass*, char const*, char const*, size_t);
    }   // namespace internal
}   // namespace dsy

#endif // __CATMacForRtti
