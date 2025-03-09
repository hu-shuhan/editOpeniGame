//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef shp_AnalyticSurface_h
#define shp_AnalyticSurface_h

// Begin Local includes
#include <cls_Uid.h>
#include <cow_ArrayCOW.h>
#include <shp_AnalyticSurfaceSegmentList.h>
#include <g3d_PointList.h>

// Forward Declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;
class gsl_Matrix;

class  shp_AnalyticSurface  : public mem_AllocationOperators
{

public:
    enum TypeEnm {
        SEGMENTS,
        CYLINDER,
        REVOLUTION,
        USER
    };

    // Methods
    shp_AnalyticSurface();
    void AssertValid() const;
    virtual shp_AnalyticSurface* Copy() const;
    void SetTransform( const gsl_Matrix& );
    // Default methods
    static inline double filletRadiusDef();

    // command string methods
    static omn_FixedString filletRadiusCmd();
    static omn_FixedString typeCmd();
    static omn_FixedString profileCmd();
    static omn_FixedString nameCmd();
    static omn_FixedString localCoordDataCmd();

    // ODB Database interface
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();
    static void CowDtor(cls_IntCOW* cow);
    shp_AnalyticSurface( const cls_ReadVisitor& rv);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    // data members
    TypeEnm type;
    double filletRadius;
    shp_AnalyticSurfaceSegmentList profile;
    g3d_PointList localCoordData;
    cow_String name;

private:
    cls_Uid m_ClsUid;
};
    
COW_ARRAYCOW_DECL(shp_AnalyticSurface);

typedef omu_PrimEnum<shp_AnalyticSurface::TypeEnm,1> shp_AnalyticSurfaceTypeConst;

inline double
shp_AnalyticSurface::filletRadiusDef()
{
    return 0.0;
}


#endif // shp_AnalyticSurface_h
