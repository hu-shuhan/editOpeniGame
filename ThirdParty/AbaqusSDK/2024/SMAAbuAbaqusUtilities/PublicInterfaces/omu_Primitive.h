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
// File Name: omu_Primitive.h (abstract class)
// 
// Description:
//     This is a base class used as the root of a special class hierarchy.
//     the purpose of this hierarchy is to mimic the class hierarchy of python.
//     The object in this hierarchy are used to arguments and return values 
//     from ABAQUS commands and queries. By maintaining an interface similar to
//     python we are able to translate to and from python objects with ease.
//     
//


#ifndef omu_Primitive_h
#define omu_Primitive_h

//
// Begin Local Includes
//
#include <omi_types.h>
#include <typ_typeTag.h>
#include <idb_Utils.h>
#include <cow_String.h>
#include <cls_Uid.h>

//
// Forward Declarations
//
class omu_Primitive;
class omu_PrimVisitor;
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;
//
// Class definition
//

class omu_Primitive  : public mem_AllocationOperators
{
  public:
    // Constructors, Destructor
    virtual ~omu_Primitive();

    // Is this primitive a particular type?
    // A char is used to discriminate the type.
    // A list of the type chars is in omu_Primitive.C
    bool IsType(char ) const;
    char Type() const {return type;};
    virtual cow_String TypeString() const; // used in TypeErrorExceptions.

    // Return a String object in python syntax for 
    // this object. This interface is used for printing
    // and journaling.
    virtual cow_String AsString(int depth=0) const = 0;
    virtual cow_String AsRepr() const;

    //
    // Equality operator.
    //
    virtual bool operator==(const omu_Primitive &) const = 0;
    bool operator!=(const omu_Primitive &) const;

    //
    // Visitor Accept method
    //
    virtual void Accept(omu_PrimVisitor *) = 0;

    //  needed to support cow_List of omu primitive cows
    virtual omu_Primitive  *Copy () const  = 0;


    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    omu_Primitive *             DeepCopy() const {
        return (omu_Primitive*)DDB_DeepCopyPtr(this, DynTypeId());
    }

    bool                    IsA(typ_typeTag) const;

  protected:
    omu_Primitive(char _type)
	:type(_type) {}

    char type;

  public:

    // Messaging/Database interface
    omu_Primitive( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

  private:

    cls_Uid m_ClsUid;
};


#endif  // #ifdef omu_Primitive_h
