//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SequenceBiaxialTestData_h
#define kmaO_SequenceBiaxialTestData_h

// Begin local includes
#include <kmaO_SequenceMaterialOption.h>
#include <kmaO_BiaxialTestData.h>
// Forward declarations

// class
class kmaO_SequenceBiaxialTestData  : private kmaO_SequenceMaterialOption
{
public:
    kmaO_SequenceBiaxialTestData();
    virtual ~kmaO_SequenceBiaxialTestData();
   
    kmaO_BiaxialTestData& get( int index );
    const kmaO_BiaxialTestData& constGet( int index ) const;
    const kmaO_BiaxialTestData& operator[]( int index ) const;
    void append( const kmaO_BiaxialTestData& sl );
    void append( const kmaO_SequenceBiaxialTestData& sll );
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    kmaO_SequenceBiaxialTestData( const kmaC_MaterialOptionListShortcut& sc );
    kmaO_SequenceBiaxialTestData& operator=( const kmaO_SequenceBiaxialTestData& rhs );
    kmaO_SequenceBiaxialTestData( const kmaC_MaterialOptionList& sll );

private:
    mutable kmaO_SequenceMaterialOption m_impl;
};

#endif
