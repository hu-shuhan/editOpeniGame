//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef mdl_Repository_h
#define mdl_Repository_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: mdl_Repository.h
// 
// Author: Uttam Narsu
// 
// Creation date: Fri Nov 15 1996
//
// Purpose:
//
// This file contains the definition of the mdl_Repository class. This is an
// abstract base class which specifies the interface for a repository such
// as the material repository. This base is needed because the Model is defined
// in the Basis, and the Material in the Material module; hence the Basis can
// not know about a repository of Materials.
//

//
// Forward Declarations
//

class mdl_Repository;
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Begin local includes
#include <cow_COW.h>
#include <typ_typeTag.h>
#include <mdl_Extension.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>

// typedefs
COW_COW2_DECL(mdl_Repository,cow_Virtual)

//
// Class definition
//

class mdl_Repository : public mdl_Extension
{
public:

//
// Constructors, Destructor
//
    mdl_Repository();
    virtual ~mdl_Repository();

//
// Member functions
//
    
    virtual mdl_Extension* Copy() const;

    bool IsEmpty() const;

    virtual int Size() const { return 0; }

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

public:

    // Messaging/Database interface
    static void CowDtor( cls_IntCOW * );
    mdl_Repository( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );
    virtual void NewUid();

private:

    cls_Uid m_ClsUid;
};


#endif  // #ifdef mdl_Repository_h
