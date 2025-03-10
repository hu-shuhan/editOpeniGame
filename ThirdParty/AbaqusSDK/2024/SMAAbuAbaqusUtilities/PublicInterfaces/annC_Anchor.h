//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_Anchor_h
#define annC_Anchor_h
//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <typ_typeTag.h>
#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <cow_ArrayCOW.h>

//
// Forward declarations
//
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

//
// Class definition
//
// This class is an abstraction of an origin of some coordinate sytem.
// 
class annC_Anchor  : public mem_AllocationOperators
{
public:
    
    annC_Anchor();

    // Destructor
    virtual ~annC_Anchor();

    // Virtual copy.
    virtual annC_Anchor* Copy() const = 0;

public: // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // DDB persistence
    annC_Anchor( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    static void CowDtor( cls_IntCOW* cow );
    
private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(annC_Anchor, cow_Virtual);

#endif

