//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef mdl_OdbExtensionTable_h
#define mdl_OdbExtensionTable_h

// Begin Local Includes
#include <mem_AllocationOperators.h>
#include <mdl_Extension.h>
#include <typ_typeTag.h>
#include <cow_String.h>
#include <cow_ListString.h>
#include <cls_xcMap.h>
#include <cls_Uid.h>

// Forward declarations
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

template <class Value>
class mdl_OdbExtensionTable : public mem_AllocationOperators
{
  public:
    mdl_OdbExtensionTable(){}

    /// Interface:
    inline mdl_Extension&       Get(const cow_String &name);
    inline const mdl_Extension& ConstGet(const cow_String &name) const;

    Value& GetValue(const cow_String& name){ return map.Get(name).Get();};
    const Value& ConstGetValue(const cow_String& name) const {return map.ConstGet(name).ConstGet();};

    /// These methods use a prototype table to copy a prototype instance
    /// of the extension if the instance map cannot satisfy the request
    /// for the extension name.

    mdl_Extension&  Get(const cow_String &name,
	 const mdl_OdbExtensionTable<Value> &prototypeTable);

    const mdl_Extension& ConstGet(const cow_String &name,
          const mdl_OdbExtensionTable<Value> &prototypeTable) const;

    inline void Insert(const cow_String &name, const mdl_Extension &);
    inline void Remove(const cow_String &name);

    cow_ListString Keys();

    static void* Ctor( cls_ReadVisitor& rv );
    mdl_OdbExtensionTable( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    
  private:
    cls_xcMap<cow_String,cow_ArrayCOW<Value, cow_Virtual<Value> >, Value>  map;
    cls_Uid m_ClsUid;
    
  public:
    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();
};

template <class Value>
inline
mdl_Extension &
mdl_OdbExtensionTable<Value>::Get(const cow_String &name)
{
    return map.Get(name).Get();
}

template <class Value>
inline
const mdl_Extension &
mdl_OdbExtensionTable<Value>::ConstGet(const cow_String &name)const
{
    return map.ConstGet(name).ConstGet();
}

template <class Value>
inline void 
mdl_OdbExtensionTable<Value>::Insert(const cow_String &name, const mdl_Extension &ext)
{
    cow_ArrayCOW<Value, cow_Virtual<Value> > obj(ext.Copy());
    map.Insert(name,obj);
}

template <class Value>
inline void 
mdl_OdbExtensionTable<Value>::Remove(const cow_String &name)
{
    map.Remove(name);
}


#define MDL_ODBEXTENSIONTABLE_DECL(TYPE,NAME)\
typedef mdl_OdbExtensionTable<TYPE> NAME;

#define MDL_ODBEXTENSIONTABLE_IMPL(TYPE,NAME)\
template <> typ_typeTag NAME::TypeId(void) { return TYP_TIX_ ## NAME; }\
template class mdl_OdbExtensionTable<TYPE>;\

//
// This macro goes in the <fac>_TypeTable.C file.
//
//
#define MDL_ODBEXTENSIONTABLE_TT(NAME)\
    {\
        typ_Phase0TT,\
        typ_NONE,\
        #NAME\
    }



#endif
