//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SequencePlanarTestData_h
#define kmaO_SequencePlanarTestData_h

// Begin local includes
#include <kmaO_SequenceMaterialOption.h>
#include <kmaO_PlanarTestData.h>
// Forward declarations

// class
class kmaO_SequencePlanarTestData  : private kmaO_SequenceMaterialOption
{
public:
    kmaO_SequencePlanarTestData();
    virtual ~kmaO_SequencePlanarTestData();
   
    kmaO_PlanarTestData& get( int index );
    const kmaO_PlanarTestData& constGet( int index ) const;
    const kmaO_PlanarTestData& operator[]( int index ) const;
    void append( const kmaO_PlanarTestData& sl );
    void append( const kmaO_SequencePlanarTestData& sll );
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    kmaO_SequencePlanarTestData( const kmaC_MaterialOptionListShortcut& sc );
    kmaO_SequencePlanarTestData& operator=( const kmaO_SequencePlanarTestData& rhs );
    kmaO_SequencePlanarTestData( const kmaC_MaterialOptionList& sll );

private:
    mutable kmaO_SequenceMaterialOption m_impl;
};

#endif
