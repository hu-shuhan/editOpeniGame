//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_Annotation_h
#define annC_Annotation_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <omn_FixedString.h>
#include <typ_typeTag.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>

//
// Forward declarations
//
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

//
// Class definition
//
class annC_Annotation  : public mem_AllocationOperators
{
public:

    // Constructor.
    annC_Annotation();

    // Destructor.
    virtual ~annC_Annotation();

    // Establishes a new identity in DDB.
    virtual void NewUid();

    // Virtual copy.
    virtual annC_Annotation *Copy() const = 0;

    // Services
    virtual void Translate( double x, double y ) = 0;

    // Static members
    static omn_FixedString nameCmd();
    static omn_FixedString copyCmd();
    static omn_FixedString annotationCmd();
    static omn_FixedString annotationRepositoryCmd();
    static omn_FixedString setParametersCmd();
    static omn_FixedString translateCmd();
    static omn_FixedString xCmd();
    static omn_FixedString yCmd();

    static omn_FixedString bringToFrontCmd();
    static omn_FixedString sendToBackCmd();
    static omn_FixedString bringForwardCmd();
    static omn_FixedString sendBackwardCmd();
    static omn_FixedString moveBeforeCmd();
    static omn_FixedString moveAfterCmd();


public: // Messaging/database interface
    
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // DDB persistence
    annC_Annotation( const cls_ReadVisitor& rv );
    virtual void DBWrite( const cls_WriteVisitor& wv ) const;
    static void InitializeMetadata( cls_ClassRegistrar& reg );
    static void CowDtor( cls_IntCOW* cow );
    
private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(annC_Annotation, cow_Virtual);


#endif

