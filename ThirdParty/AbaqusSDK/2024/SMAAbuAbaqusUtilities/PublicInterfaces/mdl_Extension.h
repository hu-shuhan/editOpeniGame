//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef mdl_Extension_h
#define mdl_Extension_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_ArrayCOW.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>

// Forward declaration
class mdl_Extension;
class cls_WriteVisitor;
class cls_ReadVisitor;
class cls_ClassRegistrar;

// typedefs
COW_ARRAYCOW2_DECL(mdl_Extension,cow_Virtual); 


//
// Class definition
//
class mdl_Extension  : public mem_AllocationOperators
{
  public:
    /// Constructors:
    mdl_Extension() {};
    virtual ~mdl_Extension();
    
    /// Interface:
    virtual mdl_Extension *Copy()const =  0;
    virtual mdl_Extension& GetValue();
    virtual const mdl_Extension& ConstGetValue() const;

  public:

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    mdl_Extension( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    static void CowDtor( cls_IntCOW* cow );
    virtual void NewUid();

private:
    cls_Uid m_ClsUid;
};

#endif
