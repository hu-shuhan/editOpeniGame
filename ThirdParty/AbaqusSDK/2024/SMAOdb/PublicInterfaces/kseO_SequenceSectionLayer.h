//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_SequenceSectionLayer_h
#define kseO_SequenceSectionLayer_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kseC_SectionLayerListShortcut.h>
#include <cow_List.h>
#include <cls_xo1DArrayList.h>
#include <kseO_SectionLayer.h>
// Forward declarations
class kseO_SectionLayer;
class kseO_SequenceSectionLayer;
class kseC_Layer;
CLS_xo1DARRAYLIST_DECL(kseC_Layer, kseC_SectionLayerList);

COW_LIST_DECL(kseO_SectionLayerCOW, kseO_SectionLayerListImpl);

class kseO_SequenceSectionLayer  : public mem_AllocationOperators
{
public:
    kseO_SequenceSectionLayer();
    virtual ~kseO_SequenceSectionLayer();
   
    kseO_SectionLayer& get( int index );
    const kseO_SectionLayer& constGet( int index ) const;
    const kseO_SectionLayer& operator[]( int index ) const;
    void append( const kseO_SectionLayer& sl );
    void append( const kseO_SequenceSectionLayer& sll );
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    const kseO_SectionLayer& GetByReference( int i ) const;
    kseO_SequenceSectionLayer( const kseC_SectionLayerListShortcut& sc );
    kseO_SequenceSectionLayer& operator=( const kseO_SequenceSectionLayer& rhs );
    kseO_SequenceSectionLayer( const kseC_SectionLayerList& sll );

private:
    mutable kseO_SectionLayerListImpl m_array;
    kseC_SectionLayerListShortcut m_shortcut;
    bool m_standalone;
};

#endif
