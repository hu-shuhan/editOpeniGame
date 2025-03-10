//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kmaO_Shear_h
#define kmaO_Shear_h
///////////////////////////////////////////////////////////////////////////////
// kmaO_Shear
//

// Begin local includes
#include <odb_Types.h>
#include <kmaO_MaterialOption.h>
// Forward declarations
class kmaC_Shear;

class kmaO_Shear: public kmaO_MaterialOption
{
  public:
    kmaO_Shear( const kmaC_MaterialOptionShortcut& shortcut );
    kmaO_Shear();
    kmaO_Shear( const odb_SequenceSequenceDouble& table,
                bool temperatureDependency = false,
                int dependencies = 0 );
    kmaO_Shear( const kmaO_Shear& copy );
    virtual kmaO_MaterialOption* Copy() const;
    kmaO_Shear& operator=( const kmaO_Shear& rhs );

    virtual ~kmaO_Shear();
    bool temperatureDependency() const;
    int dependencies() const;
    odb_SequenceSequenceDouble table() const;
    const kmaC_Shear* GetPointer() const;
    static unsigned int typeIdentifier();
};

#endif // kmaO_Shear_h
