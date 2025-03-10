//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SequenceRigidBody_h
#define odb_SequenceRigidBody_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odiK_RigidBodyListShortcut.h>
#include <odb_RigidBody.h>
#include <ddr_Shortcut.h>
// Forward declarations
class odiK_Model;
typedef ddr_Shortcut <odiK_Model> odiK_ModelShortcut;

class odb_SequenceRigidBody : public mem_AllocationOperators
{
public:
    ~odb_SequenceRigidBody();
    
    odb_RigidBody constGet( int index ) const;
    odb_RigidBody operator[]( int index ) const;
    int size() const;

    // Undocumented and unsupported
    odb_SequenceRigidBody( const odiK_RigidBodyListShortcut& rsc, 
                           const odiK_ModelShortcut& msc );

private:
    odiK_RigidBodyListShortcut m_shortcut;
    odiK_ModelShortcut m_modelShortcut;
};

#endif
