//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef shp_AnalyticSurfaceSegment_h
#define shp_AnalyticSurfaceSegment_h

// Begin Local includes
#include <g3d_PointList.h>
#include <cls_Uid.h>
#include <omu_PrimEnum.h>
#include <cow_ArrayCOW.h>

// Forward Declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class  shp_AnalyticSurfaceSegment  : public mem_AllocationOperators
{

public:
    enum TypeEnm {
        START,
        LINE,
        CIRCLE,
        PARABOLA
    };
    shp_AnalyticSurfaceSegment();

    // Methods
    void AssertValid() const;
    bool operator==(const shp_AnalyticSurfaceSegment& ) const;
    bool operator!=(const shp_AnalyticSurfaceSegment& ) const;
    virtual shp_AnalyticSurfaceSegment* Copy() const;
    // command string methods
    static omn_FixedString dataCmd();
    static omn_FixedString typeCmd();

    // ODB Database interface
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    static void CowDtor(cls_IntCOW* cow);
    shp_AnalyticSurfaceSegment( const cls_ReadVisitor& rv);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    // data members
    TypeEnm type;
    g3d_PointList data;

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW_DECL(shp_AnalyticSurfaceSegment);
    
typedef omu_PrimEnum<shp_AnalyticSurfaceSegment::TypeEnm,1> shp_AnalyticSurfaceSegmentTypeConst;

#endif // shp_AnalyticSurfaceSegment_h
