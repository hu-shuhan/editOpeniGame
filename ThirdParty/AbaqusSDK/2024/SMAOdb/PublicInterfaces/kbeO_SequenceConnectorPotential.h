//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_SequenceConnectorPotential_h
#define kbeO_SequenceConnectorPotential_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kbeC_ConnPotentialListShortcut.h>
#include <cow_List.h>
#include <cls_xo1DArrayList.h>
#include <kbeO_ConnectorPotential.h>
// Forward declarations
class kbeC_ConnectorPotential;
CLS_xo1DARRAYLIST_DECL(kbeC_ConnectorPotential, kbeC_ConnectorPotentialList);
COW_LIST_DECL(kbeO_ConnectorPotentialCOW, kbeO_ConnectorPotentialListImpl);

class kbeO_SequenceConnectorPotential  : public mem_AllocationOperators
{
public:
    kbeO_SequenceConnectorPotential();
    virtual ~kbeO_SequenceConnectorPotential();
    
    kbeO_ConnectorPotential& get( int index );
    const kbeO_ConnectorPotential& constGet( int index ) const;
    const kbeO_ConnectorPotential& operator[]( int index ) const;
    void append( const kbeO_ConnectorPotential& cp );
    void append( const kbeO_SequenceConnectorPotential& cpList );
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    const kbeO_ConnectorPotential& GetByReference( int i ) const;
    kbeO_SequenceConnectorPotential( const kbeC_ConnectorPotentialListShortcut& cps );
    kbeO_SequenceConnectorPotential& operator=( const kbeO_SequenceConnectorPotential& rhs );
    kbeO_SequenceConnectorPotential( const kbeC_ConnectorPotentialList& l );
    void CopyDerivedComponent(kbeO_ConnectorPotential, const kbeC_ConnectorPotential&);

private:
    mutable kbeO_ConnectorPotentialListImpl m_array;    
    kbeC_ConnectorPotentialListShortcut m_shortcut;
    bool m_standalone;
};

#endif
