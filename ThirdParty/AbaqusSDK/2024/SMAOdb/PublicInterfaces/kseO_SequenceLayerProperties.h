//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kseO_SequenceLayerProperties_h
#define kseO_SequenceLayerProperties_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kseC_LayerPropListShortcut.h>
#include <cow_List.h>
#include <cls_xo1DArrayList.h>
#include <kseO_LayerProperties.h>
// Forward declarations
class kseO_SequenceLayerProperties;
class kseC_LayerProperties;
CLS_xo1DARRAYLIST_DECL(kseC_LayerProperties, kseC_LayerPropertiesList);

COW_LIST_DECL(kseO_LayerPropertiesCOW, kseO_LayerPropertiesListImpl);

class kseO_SequenceLayerProperties  : public mem_AllocationOperators
{
public:
    kseO_SequenceLayerProperties();
    virtual ~kseO_SequenceLayerProperties();
    
    kseO_LayerProperties& get( int index );
    const kseO_LayerProperties& constGet( int index ) const;
    const kseO_LayerProperties& operator[]( int index ) const;
    void append( const kseO_LayerProperties& lp );
    void append( const kseO_SequenceLayerProperties& lpl );
    int size() const;
    void clearAll(); 

    // Undocumented and unsupported
    const kseO_LayerProperties& GetByReference( int i ) const;
    kseO_SequenceLayerProperties( const kseC_LayerPropertiesListShortcut& sc );
    kseO_SequenceLayerProperties& operator=( const kseO_SequenceLayerProperties& rhs );
    kseO_SequenceLayerProperties( const kseC_LayerPropertiesList& lpl );

private:
    mutable kseO_LayerPropertiesListImpl m_array;
    kseC_LayerPropertiesListShortcut m_shortcut;
    bool m_standalone;
};

#endif
