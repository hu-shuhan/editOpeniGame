//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbeO_SequenceCDCTerm_h
#define kbeO_SequenceCDCTerm_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kbeC_CDCTermListShortcut.h>
#include <cow_List.h>
#include <cls_xo1DArrayList.h>
#include <kbeO_CDCTerm.h>
// Forward declarations
class kbeC_CDCTerm;
CLS_xo1DARRAYLIST_DECL(kbeC_CDCTerm, kbeC_CDCTermList);
COW_LIST_DECL(kbeO_CDCTermCOW, kbeO_CDCTermListImpl);

class kbeO_SequenceCDCTerm  : public mem_AllocationOperators
{
public:
    kbeO_SequenceCDCTerm();
    virtual ~kbeO_SequenceCDCTerm();
    
    kbeO_CDCTerm& get( int index );
    const kbeO_CDCTerm& constGet( int index ) const;
    const kbeO_CDCTerm& operator[]( int index ) const;
    void append( const kbeO_CDCTerm& cdct );
    void append( const kbeO_SequenceCDCTerm& cdctList );
    int size() const;
    void clearAll();
    
    // Undocumented and unsupported
    const kbeO_CDCTerm& GetByReference( int i ) const;
    kbeO_SequenceCDCTerm( const kbeC_CDCTermListShortcut& cps );
    kbeO_SequenceCDCTerm& operator=( const kbeO_SequenceCDCTerm& rhs );
    kbeO_SequenceCDCTerm( const kbeC_CDCTermList& l );

private:
    mutable kbeO_CDCTermListImpl m_array;
    kbeC_CDCTermListShortcut m_shortcut;
    bool m_standalone;
};

#endif
