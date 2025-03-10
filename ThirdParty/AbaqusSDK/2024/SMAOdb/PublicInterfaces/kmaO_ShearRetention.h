//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_ShearRetention_h
#define kmaO_ShearRetention_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_ShearRetention
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_ShearRetention;

class kmaO_ShearRetention: public kmaO_MaterialOption
{
  public:
    kmaO_ShearRetention( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_ShearRetention();
    kmaO_ShearRetention( const odb_SequenceSequenceDouble& table,
                         bool temperatureDependency = false,
                         int dependencies = 0 );
    kmaO_ShearRetention( const kmaO_ShearRetention& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_ShearRetention& operator=( const kmaO_ShearRetention& rhs );

    virtual ~kmaO_ShearRetention();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_ShearRetention* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_ShearRetention_h
