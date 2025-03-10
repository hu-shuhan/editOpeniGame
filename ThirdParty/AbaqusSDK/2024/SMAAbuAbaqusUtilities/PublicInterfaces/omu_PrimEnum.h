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
// File Name: omu_PrimEnum.h
// 
// Description:
//
//     
//


#ifndef omu_PrimEnum_h
#define omu_PrimEnum_h

//
// Begin Local Includes
//
#include <omu_Primitive.h>
#include <cow_COW.h>
#include <cow_MapString2Uint.h>
#include <cow_ListString.h>
#include <cow_MapUtils.h>
#include <cls_IntCOW.h>

//
// Forward Declarations
//

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

//
// Class definition
//
class omu_PrimEnumBase : public omu_Primitive
{
  public:
    omu_PrimEnumBase();

    omu_PrimEnumBase(const cow_String &value)
	   : omu_Primitive('c')
    {
	    SetValue(value.CStr());
    }

    virtual ~omu_PrimEnumBase();
    virtual bool SetValue(const char *);   // set value from name
    const char * GetString() const;

    virtual cow_String AsString(int depth=0) const;
    virtual bool operator==(const omu_Primitive &) const;
    virtual void Accept(omu_PrimVisitor *);
    virtual omu_Primitive  * Copy() const;
    virtual cow_String TypeString() const; // used in TypeErrorExceptions.
    
    // virtual cow_ListString PermissibleValues() const;
    virtual cow_MapString2Uint PermissibleValues() const;
    // For error message 
    cow_String PermissibleValuesAsString() const;

    struct PrimEnumTableEl
    {
	   const char *name;
	   int value;
    };

    static void CowDtor(cls_IntCOW* cow);

  protected:
    cow_String     stringValue;

  public:

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

  public:

    // Messaging/Database interface
    omu_PrimEnumBase( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

  private:

    cls_Uid m_ClsUid;
};

COW_COW2_DECL(omu_PrimEnumBase,cow_Virtual);


//
// Template implementation which derives from this the base class
// this template it used to implement the the symbolic constant objects
//
template <typename ENUMTYPE,int Inst>
class omu_PrimEnum : public  omu_PrimEnumBase
{
  public:
    omu_PrimEnum() 
      : omu_PrimEnumBase()
    {};

    omu_PrimEnum(ENUMTYPE val);

    omu_PrimEnum(const omu_PrimEnumBase &val);

    omu_PrimEnum (const cow_String& str);

    omu_PrimEnum (const cls_ReadVisitor& rv);

    virtual ~omu_PrimEnum();

    virtual bool SetValue(const char *);   // set value from name
    bool         SetValue(ENUMTYPE);
    ENUMTYPE     GetValue() { return value; }

    operator ENUMTYPE() const { return value; }

#if ( __cplusplus >= 202000L ) 

    bool operator==( ENUMTYPE const& rhs) const { return ((const ENUMTYPE)*this == rhs); }
    bool operator!=( ENUMTYPE const& rhs) const { return ((const ENUMTYPE)*this != rhs); }

#else

    bool operator==(ENUMTYPE rhs) const { return ((ENUMTYPE)*this == rhs); }
    bool operator!=(ENUMTYPE rhs) const { return ((ENUMTYPE)*this != rhs); }

    virtual bool operator==(const omu_Primitive &p) const;

#endif

    virtual omu_Primitive  *Copy () const;

    virtual cow_MapString2Uint PermissibleValues() const;

    static const char* Name(int index);
    static ENUMTYPE    Value(int index);

  private:
    // the table pointer is borrowed, it is assumed that it is
    // a static table. The end of the table is indicated by a 
    // sentinal element where the name is a null pointer.
    static const omu_PrimEnumBase::PrimEnumTableEl table[];

    ENUMTYPE value;
};

// Macros

#define OMU_PRIMENUM_DECL(TYPE,INST,NAME)\
typedef omu_PrimEnum<TYPE,INST> NAME;\

#define OMU_PRIMENUM_IMPL(TYPE,INST,NAME)\
template class omu_PrimEnum<TYPE,INST>;

#endif  // #ifdef omu_PrimEnum_h
