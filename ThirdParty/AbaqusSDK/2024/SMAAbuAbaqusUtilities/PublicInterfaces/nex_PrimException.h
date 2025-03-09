//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: nex_PrimException.h
// 
// Author: Uttam Narsu
// 
// Creation date: Mon Mar 22 1999
//
// Purpose:
//
// This file contains the definition of the nex_PrimException
// template class.  You can use this template class to instantiate
// an exception over a single valued datum (like a cow_String), an
// enumerated or other primitive type.
//
// Note: if you intantiate this over an eumerated type, you must
// supply the following overloaded operators (preferably in the
// inst.C file)
//
//     bio_Ifilter& operator>>(bio_Ifilter&, enum-type&);
//     bio_Ofilter& operator<<(bio_Ofilter&, const enum-type&);
//
// The Inst member is used to allow multiple instantiations over
// P, but still have the instantiations be different types. So for
// example, the nex_AssertException and nex_BadCastException are
// over the same P (cow_String), but they must be distinct types
// so they can be caught by different catch clauses.
//

#ifndef nex_PrimException_h
#define nex_PrimException_h

//
// Includes
//

// Begin local includes

#include <nex_Exception.h>
#include <cls_Uid.h>
#include <SMABasStringMode.h>

// Forward declarations

class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;


//
// Class definition
//

template <class P, int Inst>
class nex_PrimException : public nex_Exception
{
private:
    P m_Value;

public:

    nex_PrimException(const P &value, STRMODE_DECL);

    virtual ~nex_PrimException();

    // This should be specialized
    virtual atr_String AsString(STRMODE_DECL) const;

    virtual void Propagate() const;
    virtual SMABasException* Clone() const;
    virtual nex_Exception* Copy() const;

    // Accessors
    P             Value(STRMODE_DECL) const;

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

public:

    // Messaging/Database interface
    nex_PrimException( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

private:

    cls_Uid m_ClsUid;
};


// MACRO for instantiation

#define NEX_PRIMEXCEPTION_DECL(TYPE,NAME,INST)          \
    typedef nex_PrimException<TYPE,INST> NAME;          \
    template <class T> class cls_TypeAdapter;           \
    typedef cls_TypeAdapter<NAME> NAME ## Adapter

//
// Use this macro in the .C file.
//
// Remember to include cow_List.T!
// Remember to include cls_TypeAdapter.T!
//
#define NEX_PRIMEXCEPTION_IMPL(TYPE,NAME,INST)                          \
    template <> typ_typeTag NAME::TypeId(void) { return TYP_TIX_ ## NAME; } \
    template class nex_PrimException<TYPE,INST>;                        \
    typedef cls_TypeAdapter<NAME> NAME ## Adapter;                      \
    template class cls_AbsTypeAdapter<NAME>;                            \
    template class cls_TypeAdapter<NAME>

#define NEX_PRIMEXCEPTION_TYPEID_IMPL(TYPE,NAME,INST)                          \
    typedef cls_TypeAdapter<NAME> NAME ## Adapter;                      \
    template <> typ_typeTag NAME::TypeId(void) { return TYP_TIX_ ## NAME; } \

#define NEX_PRIMEXCEPTION_INST(TYPE,NAME,INST)                          \
    template class nex_PrimException<TYPE,INST>;                        \
    typedef cls_TypeAdapter<NAME> NAME ## Adapter;                      \
    template class cls_AbsTypeAdapter<NAME>;                            \
    template class cls_TypeAdapter<NAME>

#endif //nex_PrimException_h
