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
// File Name: gsl_ModelSegment.h
// 
// Description:
//
//    This file contains the defintion of the gsl_ModelSegment class.
//
//


#ifndef gsl_ModelSegment_h
#define gsl_ModelSegment_h

//
// Includes
//


// Begin local includes
#include <mem_AllocationOperators.h>

#include <omi_types.h>
#include <gsl_Dot.h>
#include <typ_typeTag.h>
#include <cls_xo1DArrayList.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <idb_Utils.h>



//
// Constants
//

//
// Class definition
//
class gsl_ModelSegment  : public mem_AllocationOperators
{
public:
// Constructors, Destructor

    gsl_ModelSegment();
    gsl_ModelSegment(const gsl_Dot& p1, const gsl_Dot& p2);
    ~gsl_ModelSegment(){};

// Member functions
    const gsl_Dot& Point1() const;
    const gsl_Dot& Point2() const;
    gsl_Vector Direction() const;
    double Length() const;
    void Regenerate(const ftr_FeatureList*);

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    gsl_ModelSegment *             DeepCopy() const {
        return (gsl_ModelSegment*)DDB_DeepCopyPtr(this, DynTypeId());
    }

    gsl_ModelSegment(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    gsl_DotCOW point1;
    gsl_DotCOW point2;

private:
    cls_Uid m_ClsUid;

};

COW_COW_DECL(gsl_ModelSegment);
CLS_xo1DARRAYLIST_DECL(gsl_ModelSegment, gsl_ModelSegmentArray);


omu_Primitive* omu_ReturnPrim(const gsl_ModelSegment&);

#endif  
