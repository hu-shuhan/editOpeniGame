//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_RigidBody_h
#define odb_RigidBody_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odiK_RigidBodyShortcut.h>
#include <ddr_Shortcut.h>

// Forward declarations
class odb_Set;
class odiK_Model;
class rgnC_SetData;
typedef ddr_Shortcut<odiK_Model> odiK_ModelShortcut;
typedef ddr_Shortcut<rgnC_SetData> rgnC_SetDataShortcut;
class odb_AnalyticSurface;
class odb_String;

class odb_RigidBody : public mem_AllocationOperators
{
public:
    ~odb_RigidBody();
    void setPinNodes(odb_Set& nodes);
    void setTieNodes(odb_Set& nodes);
    void setElements(odb_Set& elements);
    void setAnalyticSurface( const odb_AnalyticSurface& ars);
    bool hasPinNodes() const;
    bool hasTieNodes() const;
    bool hasElements() const;
    bool hasAnalyticSurface() const;
    odb_Set referenceNodeSet() const;
    odb_Set tieNodes() const;
    odb_Set pinNodes() const;
    odb_Set elements() const;
    bool isothermal() const;
    odb_String position() const;
    odb_AnalyticSurface analyticSurface() const;
    // Undocumented and unsupported
    odb_RigidBody( const odiK_RigidBodyShortcut& sc,
                   const odiK_ModelShortcut& msc );    
private:
    enum parentEnm {
        PART_LEVEL,
        INSTANCE_LEVEL,
        ASSEMBLY_LEVEL
    };
    parentEnm getParentType() const;
    parentEnm getSetSourceFromShortcut( const rgnC_SetDataShortcut& ) const;
    odb_Set getSetFromShortcut( const rgnC_SetDataShortcut& ) const;
    odiK_RigidBodyShortcut m_shortcut;   
    odiK_ModelShortcut m_modelShortcut;
};

#endif // odb_RigidBody_h
