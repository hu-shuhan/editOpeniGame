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
// File Name: gsl_Dot.h
// 
// Description:
//
//    This file represents the recipe of a point

#ifndef gsl_Dot_h
#define gsl_Dot_h

//
// Includes
//


// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_types.h>
#include <typ_typeTag.h>
#include <idb_Utils.h>
#include <gsl_Point.h>
#include <cow_ArrayCOW.h>
#include <cow_MapUint2Uint.h>
#include <cls_Uid.h>
#include <omu_PrimEnum.h>
#include <omu_xoObjVarray.h>

class ftr_FeatureList;

//
// Constants
//
// Method types
#ifndef DotMethodDefined
#define DotMethodDefined
    enum gsl_DotMethod {gsl_FROM_POINT, 
			gsl_FROM_VERTEX, 
			gsl_FROM_DATUM_POINT,
			gsl_FROM_EDGE_PARAM, 
			gsl_FROM_FACE_DIM, 
			gsl_FROM_EDGE_VTX, 
			gsl_FROM_FACE_VTX, 
			gsl_FROM_IP_VTX_ONTO_EDGE,
			gsl_FROM_IP_CENTER_OF_EDGE,
			gsl_FROM_IP_MIDPT_OF_EDGE,
			gsl_FROM_NODE,
			gsl_DOT_INVALID,
			gsl_FROM_DATUM_CSYS_ORIGIN // has to be added after 
			          // invalid because of database compatibility
    };
#endif

OMU_PRIMENUM_DECL(gsl_DotMethod,1,gsl_DotMethodConst);

//
// Class definition
//

class gsl_Dot  : public mem_AllocationOperators
{
public:
// Constructors, Destructor

    gsl_Dot(const gsl_Point&);     // FROM_POINT method
    gsl_Dot(double, double, double);
    gsl_Dot();			// Default 
    gsl_Dot(int meshIndex, int nodeLabel);

    virtual ~gsl_Dot();
    virtual gsl_Dot* Copy() const;

// Member functions

    virtual bool Regenerate(const ftr_FeatureList*);
    int OwnerFeatID() const;

    int GetParentInstId() const{return _instanceId;}
    virtual int GetEntityId() const;
    void SetParentInstId(int id) { _instanceId = id; }
    void SetEntityId(int id) { _entityId = id; }
    
    // returns a gdy_HitRecord*
    virtual void* CreateHitRecord(const ftr_FeatureList* fl)const{return 0;} 

    virtual int NmrParents() const{return 0;}
    virtual int GetNthParentId(int i) const{return 0;}

    // Get and Set for actual point coordinates
    const gsl_Point& GetPoint() const;
    void SetValues(const double,const double,const double);

    gsl_DotMethod GetMethod() const;

    // Hack!
    virtual void SetGeometryData(double param, double d1, double d2){};
    virtual void GetGeometryData(double& param, double& d1, double& d2)const{};

    // CAE database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();
    bool                    IsA(typ_typeTag) const;
    gsl_Dot *             DeepCopy() const {
        return (gsl_Dot*)DDB_DeepCopyPtr(this, DynTypeId());}

    virtual int GetUserNodeLabel() const { return 0; }

// Ddb interface    
public: 
    gsl_Dot(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;
private:
    cls_Uid m_ClsUid;
    
protected:

    gsl_Point             _point;
    gsl_DotMethod         _method;
    int                 _instanceId;
    int                 _entityId;
    
};

//
// Define a COW that can be used to hold gsl_Dot. Since this is a base
// class it must be a cow_Virtual.
//
COW_ARRAYCOW2_DECL(gsl_Dot,cow_Virtual);
OMU_xoOBJVARRAY_DECL(gsl_Dot, gsl_DotArray);

omu_Primitive* omu_ReturnPrim(const gsl_Dot&);


#endif  // #ifdef gsl_Dot_h


