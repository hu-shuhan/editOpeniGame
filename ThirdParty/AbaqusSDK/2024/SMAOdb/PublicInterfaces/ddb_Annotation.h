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
// File Name: ddb_Annotation.h

#ifndef ddb_Annotation_h
#define ddb_Annotation_h

//
// SECTION: System includes
//

//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <BasicConstants.h>
#include <udd_AnnotationShortcut.h>
#include <annC_Annotation.h>
#include <cow_ListListFloat.h>
#include <cow_ArrayCOW.h>
#include <ddb_StateVariableEnums.h>

//  
// Class: ddb_Annotation
// 

class ddb_Annotation  : public mem_AllocationOperators
{
public:
    ddb_Annotation( const udd_AnnotationOdbShortcut&, unsigned id);
    virtual ~ddb_Annotation();
    virtual ddb_Annotation *Copy() const { return new ddb_Annotation(*this); }
    ddb_AnnotationTypeEnm getAnnotationType() const;

    // Access methods

    // Helper function
    const annC_Annotation& constAnnotation() const; 
    annC_Annotation& Annotation(); 

    // Helper function
    void  setAnnotation( const annC_Annotation& annotation );

    udd_AnnotationOdbShortcut Shortcut() const { return shortcut; }
    unsigned Id() const { return id; }

protected:
    udd_AnnotationOdbShortcut shortcut;
    unsigned id;

private:
    bool operator== ( ddb_Annotation &rhs );
    bool operator!= ( ddb_Annotation &rhs );
};

// Define class handler
COW_ARRAYCOW2_DECL(ddb_Annotation, cow_Virtual);

#endif
