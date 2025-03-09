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
// File Name: odiK_RigidBody.h
//
// Description:
//

#ifndef odiK_RigidBody_h
#define odiK_RigidBody_h

// Begin Local includes
#include <mem_AllocationOperators.h>
#include <rgnC_Region.h>
#include <rgnC_SetData.h>
#include <omu_PrimEnum.h>
// SECTION: Forward declarations

// SECTION: Class definition for odiK_RigidBody

class odiK_RigidBody : public mem_AllocationOperators
{
public:
    odiK_RigidBody(const rgnC_SetData& rn);// required argument..

    odiK_RigidBody();//needed for 1-D ArrayList
    odiK_RigidBody(const odiK_RigidBody& rhs);
    odiK_RigidBody& operator=(const odiK_RigidBody& rhs);

    bool operator==(const odiK_RigidBody& ) const;//for cow_ListCmp
    bool operator!=(const odiK_RigidBody& ) const;//for cow_ListCmp
    void AssertValid() const;//consistency check
    odiK_RigidBody* Copy() const;
    enum PositionEnm {
        INPUT,
        CENTER_OF_MASS
    };

    // Data members
    rgnC_SetData referenceNodeSet;// input file permits entry of a node_label or node_set
    rgnC_Region elements;
    rgnC_Region tieNodes;
    rgnC_Region pinNodes;
    rgnC_SetDataCOW analyticSurfaceSet;//rigid body *MAY* have an analyticSurface
    PositionEnm position;
    bool isothermal;
    
    // Default methods
    static inline PositionEnm positionDef();
    static inline bool isothermalDef();
    
    // command string methods
    static omn_FixedString positionCmd();
    static omn_FixedString isothermalCmd();
    static omn_FixedString referenceNodeCmd();
    static omn_FixedString elsetCmd();
    static omn_FixedString pinNodeSetCmd();
    static omn_FixedString tieNodeSetCmd();


    // ODB database interface
    odiK_RigidBody(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();


private:
    rgnC_SetData copyNodeSet(const odiK_RigidBody& rhs); // for copy and assignment

    cls_Uid m_ClsUid;
};

COW_COW_DECL(odiK_RigidBody);

typedef omu_PrimEnum<odiK_RigidBody::PositionEnm,1> odiK_RigidBodyPositionConst;

// Default methods implementation

inline odiK_RigidBody::PositionEnm
odiK_RigidBody::positionDef()
{
    return odiK_RigidBody::INPUT;
}

inline bool
odiK_RigidBody::isothermalDef()
{
    return false;
}

#endif // #ifndef odiK_RigidBody_h
