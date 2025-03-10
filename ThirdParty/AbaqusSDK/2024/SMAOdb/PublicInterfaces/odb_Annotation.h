//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Annotation_h
#define odb_Annotation_h
//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <odb_Enum.h>
#include <odb_String.h>
#include <odb_Types.h>
#include <cow_ArrayCOW.h>

// Forward declarations
class ddb_Annotation;
COW_ARRAYCOW2_DECL(ddb_Annotation,cow_Virtual);

//  
// Class: odb_Annotation
// 
class odb_Annotation  : public mem_AllocationOperators
{
public:
    // Access methods
    odb_String name() const;

    // Services
    void translate( double x, double y );

public:  // Undocumented and unsupported

    odb_Annotation( const ddb_Annotation &annotation );
    odb_Annotation();
    virtual ~odb_Annotation();

    const ddb_Annotation& constAnnotation() const { return m_Annotation.ConstGet(); }
    ddb_Annotation& Annotation() { return m_Annotation.Get(); }

    static unsigned int typeIdentifier();
    unsigned int subTypeIdentifier() const;

protected:
    ddb_AnnotationCOW m_Annotation;
};

#endif
