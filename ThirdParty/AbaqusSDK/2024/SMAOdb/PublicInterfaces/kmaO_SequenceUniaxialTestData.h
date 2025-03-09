//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_SequenceUniaxialTestData_h
#define kmaO_SequenceUniaxialTestData_h

// Begin local includes
#include <kmaO_SequenceMaterialOption.h>
#include <kmaO_UniaxialTestData.h>
// Forward declarations

// class
class kmaO_SequenceUniaxialTestData  : private kmaO_SequenceMaterialOption
{
public:
    kmaO_SequenceUniaxialTestData();
    virtual ~kmaO_SequenceUniaxialTestData();
   
    kmaO_UniaxialTestData& get( int index );
    const kmaO_UniaxialTestData& constGet( int index ) const;
    const kmaO_UniaxialTestData& operator[]( int index ) const;
    void append( const kmaO_UniaxialTestData& sl );
    void append( const kmaO_SequenceUniaxialTestData& sll );
    int size() const;
    void clearAll();

    // Undocumented and unsupported
    kmaO_SequenceUniaxialTestData( const kmaC_MaterialOptionListShortcut& sc );
    kmaO_SequenceUniaxialTestData& operator=( const kmaO_SequenceUniaxialTestData& rhs );
    kmaO_SequenceUniaxialTestData( const kmaC_MaterialOptionList& sll );

private:
    mutable kmaO_SequenceMaterialOption m_impl;
};

#endif
